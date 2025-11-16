#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_diplom_v0.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QDateTime>

class diplom_v0 : public QMainWindow
{
    Q_OBJECT

public:
    diplom_v0(QWidget *parent = nullptr);
    ~diplom_v0();

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
    void on_outputPowerSlider_valueChanged(int value);
    void on_fixedLengthRadioButton_toggled(bool checked);
    void on_textFieldRadioButton_toggled(bool checked);
    void on_interferenceTypeComboBox_currentTextChanged(const QString &text);
    void on_interferencePowerSlider_valueChanged(int value);
    void on_baudRateSpinBox_valueChanged(int value);


private:
    void logMessage(const QString& message);
    void populateComPorts();
    Ui::diplom_v0Class ui;
};

