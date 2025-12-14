#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_diplom_v0.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QDateTime>
#include <QKeyEvent>
#include <QTimer>

class diplom_v0 : public QMainWindow
{
    Q_OBJECT

public:
    diplom_v0(QWidget *parent = nullptr);
    ~diplom_v0();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_applyGeneralSettingsButton_clicked();
    void on_startTransmissionButton_clicked();
    void on_stopTransmissionButton_clicked();
    void on_toggleInterferenceButton_clicked();
    void on_clearLogButton_clicked();
    void on_saveLogButton_clicked();
    void on_connectButton_clicked();

    void on_modulationTypeComboBox_currentTextChanged(const QString &text);
    void on_carrierFrequencySlider_valueChanged(int value);
    void on_baudRateSlider_valueChanged(int value);
    void on_outputPowerComboBox_currentTextChanged(const QString &text);
    void on_fixedLengthRadioButton_toggled(bool checked);
    void on_textFieldRadioButton_toggled(bool checked);
    void on_interferenceTypeComboBox_currentTextChanged(const QString &text);
    void on_interferencePowerSlider_valueChanged(int value);
    void on_baudRateSpinBox_valueChanged(int value);
    void on_syncModeTxComboBox_currentTextChanged(const QString &text);
    void on_syncModeRxComboBox_currentTextChanged(const QString &text);
    void on_rxFilterBwSpinBox_valueChanged(int value);
    void on_frequencyDeviationSpinBox_valueChanged(int value);

    // Кастомные регистры
    void on_setCustomRegTxButton_clicked();
    void on_setCustomRegRxButton_clicked();

    // Обработка данных с COM-портов
    void onPrdDataReceived();
    void onPrmDataReceived();
    void onInterferenceDataReceived();

    // Ошибки портов
    void onPrdErrorOccurred(QSerialPort::SerialPortError error);
    void onPrmErrorOccurred(QSerialPort::SerialPortError error);
    void onInterferenceErrorOccurred(QSerialPort::SerialPortError error);

    // Циклическая передача
    void onCyclicTransmit();

private:
    void logMessage(const QString& message);
    void populateComPorts();
    bool parseHexValue(const QString& text, uint8_t& value);
    
    // Работа с портами
    bool openSerialPort(QSerialPort* port, const QString& portName, const QString& deviceName);
    void closeAllPorts();
    void sendCommandToPrd(const QString& command);
    void sendCommandToPrm(const QString& command);
    void sendCommandToInterference(const QString& command);
    void appendToTerminal(QPlainTextEdit* terminal, const QString& text, const QString& prefix = "");
    void parseRxData(const QString& data);
    void clearRxStats();
    void updateTxPacketCount();
    
    Ui::diplom_v0Class ui;
    
    // COM-порты
    QSerialPort* m_prdPort;
    QSerialPort* m_prmPort;
    QSerialPort* m_interferencePort;
    
    // Буферы для накопления данных
    QByteArray m_prdBuffer;
    QByteArray m_prmBuffer;
    QByteArray m_interferenceBuffer;
    
    // Флаги подключения
    bool m_isConnected;
    
    // Счётчики пакетов
    int m_rxPacketCount;
    int m_rxCrcErrorCount;
    int m_txPacketCount;
    
    // Циклическая передача
    QTimer* m_cyclicTimer;
    bool m_cyclicTransmissionActive;
};

