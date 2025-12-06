#include "diplom_v0.h"
#include <QtSerialPort/QSerialPort>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>

diplom_v0::diplom_v0(QWidget *parent)
    : QMainWindow(parent)
{
  ui.setupUi(this);
  populateComPorts();

    logMessage(QString::fromUtf8("Приложение запущено."));
}

diplom_v0::~diplom_v0()
{}

void diplom_v0::populateComPorts()
{
 const auto infos = QSerialPortInfo::availablePorts();
  for (const QSerialPortInfo &info : infos) {
   ui.prdComPortComboBox->addItem(info.portName());
        ui.prmComPortComboBox->addItem(info.portName());
 ui.interferenceComPortComboBox->addItem(info.portName());
    }
    logMessage(QString::fromUtf8("Списки COM-портов обновлены."));
}

void diplom_v0::logMessage(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui.logPlainTextEdit->appendPlainText(timestamp + ": " + message);
}

void diplom_v0::on_applyGeneralSettingsButton_clicked()
{
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
    logMessage(QString::fromUtf8("Начата передача."));
    ui.transmitterStatusLabel->setText(QString::fromUtf8("Статус: Передача"));
}

void diplom_v0::on_stopTransmissionButton_clicked()
{
    logMessage(QString::fromUtf8("Передача остановлена."));
    ui.transmitterStatusLabel->setText(QString::fromUtf8("Статус: Ожидание"));
}

void diplom_v0::on_toggleInterferenceButton_clicked()
{
    if (ui.toggleInterferenceButton->text() == QString::fromUtf8("Включить помеху")) {
        ui.toggleInterferenceButton->setText(QString::fromUtf8("Выключить помеху"));
        logMessage(QString::fromUtf8("Помеха включена."));
    } else {
        ui.toggleInterferenceButton->setText(QString::fromUtf8("Включить помеху"));
        logMessage(QString::fromUtf8("Помеха выключена."));
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

void diplom_v0::on_connectButton_clicked()
{
    QString log = QString::fromUtf8("Попытка подключения: ПРД - %1, ПРМ - %2, Генератор - %3")
        .arg(ui.prdComPortComboBox->currentText())
        .arg(ui.prmComPortComboBox->currentText())
        .arg(ui.interferenceComPortComboBox->currentText());
    logMessage(log);
    // Здесь будет логика подключения
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
    // Этот слот нужен для автоматического подключения,
    // но логирование уже обрабатывается on_baudRateSlider_valueChanged,
  // с которым он синхронизирован. Оставляем пустым, чтобы избежать дублирования.
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

