#ifndef QSERIALPORTTEST_H
#define QSERIALPORTTEST_H

#include <QtWidgets/QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include "ui_QSerialPortTest.h"

class QSerialPortTest : public QMainWindow
{
    Q_OBJECT

public:
    QSerialPortTest(QWidget* parent = nullptr);
    ~QSerialPortTest();

private slots:
    void onOpenPortButtonClicked();
    void onSendMessageButtonClicked();
    void onSerialPortReadyRead();
    void onRequestFinished(QNetworkReply* reply); // Slot pour gérer la réponse du serveur

private:
    void sendCoordinatesToServer(double latitude, double longitude); // Méthode pour envoyer les coordonnées au serveur

    Ui::QSerialPortTestClass ui;
    QSerialPort* port; // Port série utilisé pour la communication
    QTimer* clearTimer; // Timer pour nettoyer les données reçues périodiquement
    QNetworkAccessManager* networkManager; // Gestionnaire de réseau pour envoyer les données
};

#endif // QSERIALPORTTEST_H
