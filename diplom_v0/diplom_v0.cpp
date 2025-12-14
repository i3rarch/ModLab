#include "diplom_v0.h"
#include <QtSerialPort/QSerialPort>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

diplom_v0::diplom_v0(QWidget *parent)
    : QMainWindow(parent)
    , m_prdPort(new QSerialPort(this))
  , m_prmPort(new QSerialPort(this))
    , m_interferencePort(new QSerialPort(this))
    , m_isConnected(false)
    , m_rxPacketCount(0)
    , m_rxCrcErrorCount(0)
    , m_txPacketCount(0)
    , m_cyclicTimer(new QTimer(this))
  , m_cyclicTransmissionActive(false)
{
  ui.setupUi(this);
    populateComPorts();

    // Подключаем сигналы приёма данных
    connect(m_prdPort, &QSerialPort::readyRead, this, &diplom_v0::onPrdDataReceived);
    connect(m_prmPort, &QSerialPort::readyRead, this, &diplom_v0::onPrmDataReceived);
    connect(m_interferencePort, &QSerialPort::readyRead, this, &diplom_v0::onInterferenceDataReceived);

    // Подключаем обработку ошибок
    connect(m_prdPort, &QSerialPort::errorOccurred, this, &diplom_v0::onPrdErrorOccurred);
    connect(m_prmPort, &QSerialPort::errorOccurred, this, &diplom_v0::onPrmErrorOccurred);
    connect(m_interferencePort, &QSerialPort::errorOccurred, this, &diplom_v0::onInterferenceErrorOccurred);

    // Устанавливаем фильтр событий для перехвата Enter в поле ввода
    ui.dataInputPlainTextEdit->installEventFilter(this);

    // Подключаем таймер циклической передачи
    connect(m_cyclicTimer, &QTimer::timeout, this, &diplom_v0::onCyclicTransmit);

    // === Добавляем элементы для циклической передачи в ПРД ===
    QWidget* cyclicWidget = new QWidget();
    QHBoxLayout* cyclicLayout = new QHBoxLayout(cyclicWidget);
    cyclicLayout->setContentsMargins(0, 0, 0, 0);
    
 QCheckBox* cyclicCheckBox = new QCheckBox(QString::fromUtf8("Циклическая передача"));
    cyclicCheckBox->setObjectName("cyclicCheckBox");
    
    QLabel* periodLabel = new QLabel(QString::fromUtf8("Период (мс):"));
    QSpinBox* periodSpinBox = new QSpinBox();
    periodSpinBox->setObjectName("cyclicPeriodSpinBox");
    periodSpinBox->setMinimum(10);
    periodSpinBox->setMaximum(60000);
    periodSpinBox->setValue(1000);
    periodSpinBox->setSingleStep(100);
    periodSpinBox->setMinimumWidth(80);
    
    cyclicLayout->addWidget(cyclicCheckBox);
    cyclicLayout->addWidget(periodLabel);
    cyclicLayout->addWidget(periodSpinBox);
    cyclicLayout->addStretch();
    
    // Вставляем перед кнопками передачи
QVBoxLayout* txLayout = qobject_cast<QVBoxLayout*>(ui.groupBox_transmitter->layout());
    if (txLayout) {
        // Находим индекс layout_tx_buttons и вставляем перед ним
        for (int i = 0; i < txLayout->count(); i++) {
          QLayoutItem* item = txLayout->itemAt(i);
   if (item->layout() && item->layout()->objectName() == "layout_tx_buttons") {
     txLayout->insertWidget(i, cyclicWidget);
                break;
      }
        }
  }
    
    // Подключаем сигнал чекбокса
    connect(cyclicCheckBox, &QCheckBox::toggled, this, [this, periodSpinBox](bool checked) {
     if (checked && m_isConnected) {
  m_cyclicTransmissionActive = true;
   m_cyclicTimer->start(periodSpinBox->value());
            logMessage(QString::fromUtf8("Циклическая передача включена, период: %1 мс").arg(periodSpinBox->value()));
        } else {
 m_cyclicTransmissionActive = false;
            m_cyclicTimer->stop();
    if (checked && !m_isConnected) {
                QCheckBox* cb = findChild<QCheckBox*>("cyclicCheckBox");
        if (cb) cb->setChecked(false);
    logMessage(QString::fromUtf8("Ошибка: Сначала подключитесь к устройствам"));
    } else {
  logMessage(QString::fromUtf8("Циклическая передача отключена"));
            }
        }
    });
    
    // Обновление периода при изменении
    connect(periodSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
      if (m_cyclicTransmissionActive) {
            m_cyclicTimer->setInterval(value);
            logMessage(QString::fromUtf8("Период циклической передачи изменен на: %1 мс").arg(value));
        }
    });
    
 // === Добавляем кнопку очистки статистики в ПРМ ===
    QPushButton* clearStatsButton = new QPushButton(QString::fromUtf8("Очистить статистику"));
    clearStatsButton->setObjectName("clearStatsButton");
    clearStatsButton->setMinimumHeight(25);
  
    QVBoxLayout* rxLayout = qobject_cast<QVBoxLayout*>(ui.groupBox_receiver->layout());
  if (rxLayout) {
        // Вставляем после perLabel
        for (int i = 0; i < rxLayout->count(); i++) {
 QLayoutItem* item = rxLayout->itemAt(i);
      if (item->widget() == ui.perLabel) {
        rxLayout->insertWidget(i + 1, clearStatsButton);
break;
      }
        }
    }
    
    // Подключаем сигнал кнопки очистки
    connect(clearStatsButton, &QPushButton::clicked, this, [this]() {
  clearRxStats();
        ui.receivedDataPlainTextEdit->clear();
        logMessage(QString::fromUtf8("Статистика приёмника и окно данных очищены"));
    });

    logMessage(QString::fromUtf8("Приложение запущено."));
}

diplom_v0::~diplom_v0()
{
    m_cyclicTimer->stop();
    closeAllPorts();
}

void diplom_v0::clearRxStats()
{
    m_rxPacketCount = 0;
    m_rxCrcErrorCount = 0;
    ui.packetCounterLabel->setText(QString::fromUtf8("Счетчик пакетов (Принято / ошибки CRC): 0 / 0"));
  ui.perLabel->setText(QString::fromUtf8("PER: 0%"));
    ui.perLabel->setStyleSheet("font-weight: bold; color: #006400;");
    ui.rssiLabel->setText(QString::fromUtf8("RSSI: - дБм"));
    ui.lqiLabel->setText(QString::fromUtf8("LQI: -"));
}

void diplom_v0::updateTxPacketCount()
{
    m_txPacketCount++;
    ui.sentPacketsLabel->setText(QString::fromUtf8("Отправлено пакетов: %1").arg(m_txPacketCount));
}

void diplom_v0::onCyclicTransmit()
{
    if (!m_prdPort->isOpen()) {
        m_cyclicTimer->stop();
        m_cyclicTransmissionActive = false;
QCheckBox* cb = findChild<QCheckBox*>("cyclicCheckBox");
        if (cb) cb->setChecked(false);
 logMessage(QString::fromUtf8("Циклическая передача остановлена: порт ПРД закрыт"));
        return;
    }
    
    QString data = ui.dataInputPlainTextEdit->toPlainText().trimmed();
    if (data.isEmpty()) {
        data = "PING";
    }
 
    QString cmdWithNewline = QString("tx %1\r\n").arg(data);
    m_prdPort->write(cmdWithNewline.toUtf8());
    updateTxPacketCount();
    // Не логируем каждый пакет при циклической передаче чтобы не засорять лог
}

bool diplom_v0::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.dataInputPlainTextEdit && event->type() == QEvent::KeyPress) {
  QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
   if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
           // Enter без Shift - отправляем команду
         QString text = ui.dataInputPlainTextEdit->toPlainText().trimmed();
    if (!text.isEmpty() && m_isConnected) {
              // Отправляем на ПРД
       sendCommandToPrd(text);
     ui.dataInputPlainTextEdit->clear();
 }
 return true; // Событие обработано
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void diplom_v0::populateComPorts()
{
    ui.prdComPortComboBox->clear();
    ui.prmComPortComboBox->clear();
    ui.interferenceComPortComboBox->clear();
    
 // Добавляем пустой элемент для генератора помех
    ui.interferenceComPortComboBox->addItem(QString::fromUtf8("-- Не использовать --"), "");
    
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
    QString displayText = info.portName() + " (" + info.description() + ")";
        ui.prdComPortComboBox->addItem(displayText, info.portName());
        ui.prmComPortComboBox->addItem(displayText, info.portName());
 ui.interferenceComPortComboBox->addItem(displayText, info.portName());
  }
    logMessage(QString::fromUtf8("Списки COM-портов обновлены. Найдено портов: %1").arg(infos.size()));
}

void diplom_v0::logMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
  ui.logPlainTextEdit->appendPlainText("[" + timestamp + "] " + message);
 // Прокрутка вниз
    ui.logPlainTextEdit->verticalScrollBar()->setValue(ui.logPlainTextEdit->verticalScrollBar()->maximum());
}

bool diplom_v0::openSerialPort(QSerialPort* port, const QString& portName, const QString& deviceName)
{
    port->setPortName(portName);
    port->setBaudRate(QSerialPort::Baud115200);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);

    if (port->open(QIODevice::ReadWrite)) {
        logMessage(QString::fromUtf8("%1: Порт %2 открыт успешно").arg(deviceName).arg(portName));
        return true;
    } else {
        logMessage(QString::fromUtf8("%1: Ошибка открытия порта %2: %3").arg(deviceName).arg(portName).arg(port->errorString()));
        return false;
    }
}

void diplom_v0::closeAllPorts()
{
    if (m_prdPort->isOpen()) {
        m_prdPort->close();
        logMessage(QString::fromUtf8("ПРД: Порт закрыт"));
    }
    if (m_prmPort->isOpen()) {
     m_prmPort->close();
        logMessage(QString::fromUtf8("ПРМ: Порт закрыт"));
    }
    if (m_interferencePort->isOpen()) {
     m_interferencePort->close();
        logMessage(QString::fromUtf8("Генератор помех: Порт закрыт"));
    }
}

void diplom_v0::sendCommandToPrd(const QString& command)
{
    if (m_prdPort->isOpen()) {
     QString cmdWithNewline = command + "\r\n";
        m_prdPort->write(cmdWithNewline.toUtf8());
        logMessage(QString::fromUtf8("ПРД >> %1").arg(command));
    } else {
        logMessage(QString::fromUtf8("ПРД: Порт не открыт"));
    }
}

void diplom_v0::sendCommandToPrm(const QString& command)
{
    if (m_prmPort->isOpen()) {
        QString cmdWithNewline = command + "\r\n";
        m_prmPort->write(cmdWithNewline.toUtf8());
        logMessage(QString::fromUtf8("ПРМ >> %1").arg(command));
    } else {
        logMessage(QString::fromUtf8("ПРМ: Порт не открыт"));
    }
}

void diplom_v0::sendCommandToInterference(const QString& command)
{
    if (m_interferencePort->isOpen()) {
   QString cmdWithNewline = command + "\r\n";
        m_interferencePort->write(cmdWithNewline.toUtf8());
        logMessage(QString::fromUtf8("Генератор >> %1").arg(command));
    } else {
        logMessage(QString::fromUtf8("Генератор помех: Порт не открыт"));
    }
}

void diplom_v0::appendToTerminal(QPlainTextEdit* terminal, const QString& text, const QString& prefix)
{
  QString displayText = prefix.isEmpty() ? text : prefix + text;
  terminal->moveCursor(QTextCursor::End);
    terminal->insertPlainText(displayText);
    terminal->verticalScrollBar()->setValue(terminal->verticalScrollBar()->maximum());
}

void diplom_v0::parseRxData(const QString& data)
{
    // Парсим строки вида: [1386] RX: 50 49 4E 47 38 31 00 00 | PING81.. | RSSI:-52 LQI:7 CRC:OK #1
    QRegularExpression rxRegex(R"(\[(\d+)\]\s*RX:.*\|\s*RSSI:(-?\d+)\s*LQI:(\d+)\s*CRC:(\w+)\s*#(\d+))");
    QRegularExpressionMatch match = rxRegex.match(data);
    
    if (match.hasMatch()) {
        int rssi = match.captured(2).toInt();
        int lqi = match.captured(3).toInt();
        QString crc = match.captured(4);
        int packetNum = match.captured(5).toInt();
        
  // Обновляем UI
ui.rssiLabel->setText(QString::fromUtf8("RSSI: %1 дБм").arg(rssi));
        ui.lqiLabel->setText(QString::fromUtf8("LQI: %1").arg(lqi));
        
        m_rxPacketCount = packetNum;
        if (crc != "OK") {
   m_rxCrcErrorCount++;
        }
        
        ui.packetCounterLabel->setText(QString::fromUtf8("Счетчик пакетов (Принято / ошибки CRC): %1 / %2")
            .arg(m_rxPacketCount).arg(m_rxCrcErrorCount));
        
        // Вычисляем PER
 double per = (m_rxPacketCount > 0) ? (100.0 * m_rxCrcErrorCount / m_rxPacketCount) : 0.0;
      ui.perLabel->setText(QString::fromUtf8("PER: %1%").arg(per, 0, 'f', 2));
        
        // Цвет в зависимости от PER
        if (per < 1.0) {
        ui.perLabel->setStyleSheet("font-weight: bold; color: #006400;"); // Зелёный
        } else if (per < 5.0) {
            ui.perLabel->setStyleSheet("font-weight: bold; color: #FF8C00;"); // Оранжевый
        } else {
            ui.perLabel->setStyleSheet("font-weight: bold; color: #DC143C;"); // Красный
        }
    }
}

void diplom_v0::onPrdDataReceived()
{
    m_prdBuffer.append(m_prdPort->readAll());
    
    // Обрабатываем данные построчно
    while (m_prdBuffer.contains('\n')) {
        int index = m_prdBuffer.indexOf('\n');
        QByteArray line = m_prdBuffer.left(index);
        m_prdBuffer.remove(0, index + 1);
        
        // Удаляем \r если есть
        if (line.endsWith('\r')) {
     line.chop(1);
        }
        
        QString text = QString::fromUtf8(line);
        if (!text.isEmpty()) {
         logMessage(QString::fromUtf8("ПРД << %1").arg(text));
    
         // Парсим ответ TX OK для подсчёта отправленных пакетов
            if (text.startsWith("TX OK:")) {
      // Не вызываем updateTxPacketCount() здесь, т.к. счётчик уже в ответе устройства
    QRegularExpression txRegex(R"(TX OK:.*total:\s*(\d+))");
   QRegularExpressionMatch match = txRegex.match(text);
         if (match.hasMatch()) {
         m_txPacketCount = match.captured(1).toInt();
          ui.sentPacketsLabel->setText(QString::fromUtf8("Отправлено пакетов: %1").arg(m_txPacketCount));
    }
    }
      }
    }
}

void diplom_v0::onPrmDataReceived()
{
    m_prmBuffer.append(m_prmPort->readAll());
    
    // Обрабатываем данные построчно
    while (m_prmBuffer.contains('\n')) {
        int index = m_prmBuffer.indexOf('\n');
        QByteArray line = m_prmBuffer.left(index);
        m_prmBuffer.remove(0, index + 1);
    
        // Удаляем \r если есть
    if (line.endsWith('\r')) {
            line.chop(1);
        }
        
        QString text = QString::fromUtf8(line);
 if (!text.isEmpty()) {
     logMessage(QString::fromUtf8("ПРМ << %1").arg(text));
            
     // Показываем принятые данные в окне приёмника
     ui.receivedDataPlainTextEdit->appendPlainText(text);
            
     // Парсим данные для обновления статистики
            parseRxData(text);
    }
    }
}

void diplom_v0::onInterferenceDataReceived()
{
    m_interferenceBuffer.append(m_interferencePort->readAll());
    
    // Обрабатываем данные построчно
    while (m_interferenceBuffer.contains('\n')) {
        int index = m_interferenceBuffer.indexOf('\n');
        QByteArray line = m_interferenceBuffer.left(index);
        m_interferenceBuffer.remove(0, index + 1);
        
        // Удаляем \r если есть
        if (line.endsWith('\r')) {
  line.chop(1);
        }
   
 QString text = QString::fromUtf8(line);
     if (!text.isEmpty()) {
         logMessage(QString::fromUtf8("Генератор << %1").arg(text));
        }
    }
}

void diplom_v0::onPrdErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
    logMessage(QString::fromUtf8("ПРД: Ошибка порта: %1").arg(m_prdPort->errorString()));
    }
}

void diplom_v0::onPrmErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
logMessage(QString::fromUtf8("ПРМ: Ошибка порта: %1").arg(m_prmPort->errorString()));
    }
}

void diplom_v0::onInterferenceErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        logMessage(QString::fromUtf8("Генератор: Ошибка порта: %1").arg(m_interferencePort->errorString()));
    }
}

void diplom_v0::on_connectButton_clicked()
{
    if (m_isConnected) {
        // Останавливаем циклическую передачу
        m_cyclicTimer->stop();
        m_cyclicTransmissionActive = false;
   QCheckBox* cb = findChild<QCheckBox*>("cyclicCheckBox");
        if (cb) cb->setChecked(false);
        
      // Отключаемся
        closeAllPorts();
        m_isConnected = false;
   ui.connectButton->setText(QString::fromUtf8("Подключить"));
        ui.transmitterStatusLabel->setText(QString::fromUtf8("Статус: Отключено"));
        logMessage(QString::fromUtf8("Отключено от устройств"));
    } else {
        // Подключаемся
        QString prdPortName = ui.prdComPortComboBox->currentData().toString();
        QString prmPortName = ui.prmComPortComboBox->currentData().toString();
    QString intPortName = ui.interferenceComPortComboBox->currentData().toString();
        
        bool prdOk = false;
     bool prmOk = false;
        bool intOk = false;
        
        // Подключаем ПРД
        if (!prdPortName.isEmpty()) {
     prdOk = openSerialPort(m_prdPort, prdPortName, QString::fromUtf8("ПРД"));
     } else {
            logMessage(QString::fromUtf8("ПРД: Порт не выбран"));
        }
        
        // Подключаем ПРМ (если отличается от ПРД)
        if (!prmPortName.isEmpty()) {
            if (prmPortName != prdPortName) {
            prmOk = openSerialPort(m_prmPort, prmPortName, QString::fromUtf8("ПРМ"));
      } else {
     logMessage(QString::fromUtf8("ПРМ: Тот же порт что и ПРД, пропускаем"));
        }
        } else {
            logMessage(QString::fromUtf8("ПРМ: Порт не выбран"));
        }
        
    // Подключаем генератор помех (опционально, только если порт отличается)
    if (!intPortName.isEmpty() && intPortName != prdPortName && intPortName != prmPortName) {
          intOk = openSerialPort(m_interferencePort, intPortName, QString::fromUtf8("Генератор помех"));
         if (!intOk) {
           logMessage(QString::fromUtf8("Генератор помех: Продолжаем без генератора"));
     }
        } else if (intPortName.isEmpty()) {
          logMessage(QString::fromUtf8("Генератор помех: Не используется"));
        }
        
        // Успех если подключился хотя бы один из основных устройств (ПРД или ПРМ)
  if (prdOk || prmOk) {
            m_isConnected = true;
     ui.connectButton->setText(QString::fromUtf8("Отключить"));
     
    QString status = QString::fromUtf8("Статус: Подключено (");
            QStringList connected;
   if (prdOk) connected << QString::fromUtf8("ПРД");
            if (prmOk) connected << QString::fromUtf8("ПРМ");
            if (intOk) connected << QString::fromUtf8("Генератор");
      status += connected.join(", ") + ")";
            
 ui.transmitterStatusLabel->setText(status);
 logMessage(QString::fromUtf8("Подключение выполнено: %1").arg(connected.join(", ")));
 
       // Сбрасываем счётчики
      m_txPacketCount = 0;
            ui.sentPacketsLabel->setText(QString::fromUtf8("Отправлено пакетов: 0"));
  clearRxStats();
 } else {
       closeAllPorts();
        logMessage(QString::fromUtf8("Ошибка: Не удалось подключиться ни к ПРД, ни к ПРМ"));
          QMessageBox::warning(this, QString::fromUtf8("Ошибка подключения"),
      QString::fromUtf8("Не удалось подключиться к устройствам.\n"
      "Проверьте, что порты не заняты другими программами\n"
          "(например, Arduino IDE Serial Monitor)."));
        }
    }
}

void diplom_v0::on_applyGeneralSettingsButton_clicked()
{
    // Формируем команды для устройств
    int freq = ui.carrierFrequencySpinBox->value() * 1000; // МГц -> кГц
    int baud = ui.baudRateSpinBox->value();
    QString mod;
    switch (ui.modulationTypeComboBox->currentIndex()) {
      case 0: mod = "2fsk"; break;
        case 1: mod = "gfsk"; break;
      case 2: mod = "ook"; break;
   case 3: mod = "msk"; break;
        default: mod = "2fsk";
    }
  int dev = ui.frequencyDeviationSpinBox->value();
    int syncTx = ui.syncModeTxComboBox->currentIndex();
    int syncRx = ui.syncModeRxComboBox->currentIndex();
    int bw = ui.rxFilterBwSpinBox->value();
    
    // Отправляем команды на ПРД
    if (m_prdPort->isOpen()) {
   sendCommandToPrd(QString("sf %1").arg(freq));
      QThread::msleep(50);
      sendCommandToPrd(QString("sb %1").arg(baud));
        QThread::msleep(50);
        sendCommandToPrd(QString("sm %1").arg(mod));
   QThread::msleep(50);
     sendCommandToPrd(QString("sd %1").arg(dev));
  QThread::msleep(50);
        sendCommandToPrd(QString("ssm %1").arg(syncTx));
    }
    
  // Отправляем команды на ПРМ
    if (m_prmPort->isOpen()) {
        sendCommandToPrm(QString("sf %1").arg(freq));
    QThread::msleep(50);
        sendCommandToPrm(QString("sb %1").arg(baud));
        QThread::msleep(50);
        sendCommandToPrm(QString("sm %1").arg(mod));
        QThread::msleep(50);
sendCommandToPrm(QString("sd %1").arg(dev));
        QThread::msleep(50);
        sendCommandToPrm(QString("ssm %1").arg(syncRx));
        QThread::msleep(50);
        sendCommandToPrm(QString("sbw %1").arg(bw));
    }
    
    QString log = QString::fromUtf8("Применены общие настройки: Частота: %1 МГц, Модуляция: %2, Скорость: %3 бод/с, Девиация: %4 кГц, Синхр. ПРД: %5, Синхр. ПРМ: %6, Полоса ПРМ: %7 кГц")
        .arg(ui.carrierFrequencySpinBox->value())
        .arg(ui.modulationTypeComboBox->currentText())
        .arg(ui.baudRateSpinBox->value())
        .arg(ui.frequencyDeviationSpinBox->value())
        .arg(ui.syncModeTxComboBox->currentIndex())
      .arg(ui.syncModeRxComboBox->currentIndex())
        .arg(ui.rxFilterBwSpinBox->value());
    logMessage(log);
}

void diplom_v0::on_startTransmissionButton_clicked()
{
    if (!m_prdPort->isOpen()) {
        logMessage(QString::fromUtf8("ПРД: Порт не открыт"));
return;
    }
    
    QString data = ui.dataInputPlainTextEdit->toPlainText().trimmed();
    if (data.isEmpty()) {
 data = "PING"; // По умолчанию
    }
    
    sendCommandToPrd(QString("tx %1").arg(data));
    ui.transmitterStatusLabel->setText(QString::fromUtf8("Статус: Передача"));
}

void diplom_v0::on_stopTransmissionButton_clicked()
{
 // Останавливаем циклическую передачу
    m_cyclicTimer->stop();
    m_cyclicTransmissionActive = false;
    QCheckBox* cb = findChild<QCheckBox*>("cyclicCheckBox");
    if (cb) cb->setChecked(false);
    
    logMessage(QString::fromUtf8("Передача остановлена."));
  ui.transmitterStatusLabel->setText(QString::fromUtf8("Статус: Ожидание"));
}

void diplom_v0::on_toggleInterferenceButton_clicked()
{
if (ui.toggleInterferenceButton->text() == QString::fromUtf8("Включить помеху")) {
      ui.toggleInterferenceButton->setText(QString::fromUtf8("Выключить помеху"));
      logMessage(QString::fromUtf8("Помеха включена."));
        // TODO: отправить команду на генератор помех
    } else {
        ui.toggleInterferenceButton->setText(QString::fromUtf8("Включить помеху"));
 logMessage(QString::fromUtf8("Помеха выключена."));
   // TODO: отправить команду на генератор помех
    }
}

void diplom_v0::on_clearLogButton_clicked()
{
    ui.logPlainTextEdit->clear();
    logMessage(QString::fromUtf8("Лог очищен."));
}

void diplom_v0::on_saveLogButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, QString::fromUtf8("Сохранить лог"), "", QString::fromUtf8("Текстовые файлы (*.txt);;Все файлы (*)"));
    if (filePath.isEmpty()) {
    return;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
   logMessage(QString::fromUtf8("Ошибка сохранения файла лога."));
        return;
    }
  QTextStream out(&file);
    out << ui.logPlainTextEdit->toPlainText();
    file.close();
    logMessage(QString::fromUtf8("Лог сохранен в файл: ") + filePath);
}

void diplom_v0::on_modulationTypeComboBox_currentTextChanged(const QString &text)
{
    bool isFsk = (text == "2-FSK" || text == "GFSK");
    ui.frequencyDeviationSpinBox->setEnabled(isFsk);
    logMessage(QString::fromUtf8("Тип модуляции изменен на: %1").arg(text));
}

void diplom_v0::on_carrierFrequencySlider_valueChanged(int value)
{
    logMessage(QString::fromUtf8("Несущая частота изменена на: %1 МГц").arg(value));
}

void diplom_v0::on_baudRateSlider_valueChanged(int value)
{
    logMessage(QString::fromUtf8("Скорость передачи изменена на: %1 бод/с").arg(value));
}

void diplom_v0::on_outputPowerComboBox_currentTextChanged(const QString &text)
{
    if (m_prdPort->isOpen()) {
        sendCommandToPrd(QString("sp %1").arg(text));
    }
    logMessage(QString::fromUtf8("Выходная мощность изменена на: %1 дБм").arg(text));
}

void diplom_v0::on_fixedLengthRadioButton_toggled(bool checked)
{
    ui.packetLengthLineEdit->setEnabled(checked);
    logMessage(QString::fromUtf8("Режим пакетов изменен на: %1").arg(checked ? QString::fromUtf8("Фиксированная длина") : QString::fromUtf8("Переменная длина")));
}

void diplom_v0::on_textFieldRadioButton_toggled(bool checked)
{
    ui.dataInputPlainTextEdit->setEnabled(checked);
    logMessage(QString::fromUtf8("Источник данных изменен на: %1").arg(checked ? QString::fromUtf8("Текстовое поле") : QString::fromUtf8("Другой")));
}

void diplom_v0::on_interferenceTypeComboBox_currentTextChanged(const QString &text)
{
    logMessage(QString::fromUtf8("Тип помехи изменен на: %1").arg(text));
}

void diplom_v0::on_interferencePowerSlider_valueChanged(int value)
{
    logMessage(QString::fromUtf8("Мощность помехи изменена на: %1").arg(value));
}

void diplom_v0::on_baudRateSpinBox_valueChanged(int value)
{
    // Синхронизировано со слайдером
}

void diplom_v0::on_syncModeTxComboBox_currentTextChanged(const QString &text)
{
    logMessage(QString::fromUtf8("Режим синхронизации ПРД изменен на: %1").arg(text));
}

void diplom_v0::on_syncModeRxComboBox_currentTextChanged(const QString &text)
{
    logMessage(QString::fromUtf8("Режим синхронизации ПРМ изменен на: %1").arg(text));
}

void diplom_v0::on_rxFilterBwSpinBox_valueChanged(int value)
{
    logMessage(QString::fromUtf8("Полоса приёмного фильтра изменена на: %1 кГц").arg(value));
}

void diplom_v0::on_frequencyDeviationSpinBox_valueChanged(int value)
{
    logMessage(QString::fromUtf8("Девиация частоты изменена на: %1 кГц").arg(value));
}

bool diplom_v0::parseHexValue(const QString& text, uint8_t& value)
{
    QString trimmed = text.trimmed();
    bool ok = false;
    
    if (trimmed.startsWith("0x", Qt::CaseInsensitive)) {
        value = trimmed.mid(2).toUInt(&ok, 16);
    } else {
        value = trimmed.toUInt(&ok, 16);
  }
    
    return ok;
}

void diplom_v0::on_setCustomRegTxButton_clicked()
{
    uint8_t addr, val;
    
    if (!parseHexValue(ui.customRegAddrTxLineEdit->text(), addr)) {
        logMessage(QString::fromUtf8("Ошибка: Неверный формат адреса ПРД. Используйте формат 0x00."));
      return;
    }
    
    if (!parseHexValue(ui.customRegValueTxLineEdit->text(), val)) {
        logMessage(QString::fromUtf8("Ошибка: Неверный формат значения ПРД. Используйте формат 0x00."));
  return;
    }
    
    QString addrHex = QString("%1").arg(addr, 2, 16, QChar('0')).toUpper();
    QString valHex = QString("%1").arg(val, 2, 16, QChar('0')).toUpper();
    
    logMessage(QString::fromUtf8("ПРД: Запись в регистр 0x%1 значения 0x%2").arg(addrHex).arg(valHex));
    
    // Отправляем команду на устройство (формат может зависеть от прошивки)
    if (m_prdPort->isOpen()) {
        // Пример команды - адаптируйте под вашу прошивку
        sendCommandToPrd(QString("wr %1 %2").arg(addrHex).arg(valHex));
    }
}

void diplom_v0::on_setCustomRegRxButton_clicked()
{
    uint8_t addr, val;
    
    if (!parseHexValue(ui.customRegAddrRxLineEdit->text(), addr)) {
        logMessage(QString::fromUtf8("Ошибка: Неверный формат адреса ПРМ. Используйте формат 0x00."));
  return;
    }
    
    if (!parseHexValue(ui.customRegValueRxLineEdit->text(), val)) {
        logMessage(QString::fromUtf8("Ошибка: Неверный формат значения ПРМ. Используйте формат 0x00."));
        return;
    }

    QString addrHex = QString("%1").arg(addr, 2, 16, QChar('0')).toUpper();
    QString valHex = QString("%1").arg(val, 2, 16, QChar('0')).toUpper();
    
    logMessage(QString::fromUtf8("ПРМ: Запись в регистр 0x%1 значения 0x%2").arg(addrHex).arg(valHex));
    
    // Отправляем команду на устройство
if (m_prmPort->isOpen()) {
        sendCommandToPrm(QString("wr %1 %2").arg(addrHex).arg(valHex));
    }
}

