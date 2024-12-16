#include "QSerialPortTest.h"
#include <QStringList>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

QSerialPortTest::QSerialPortTest(QWidget* parent)
    : QMainWindow(parent), port(nullptr)
{
    ui.setupUi(this);

    // Initialisation des ports disponibles
    const auto availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : availablePorts)
    {
        ui.portComboBox->addItem(info.portName(), QVariant(info.portName()));
    }

    // Initialisation du timer
    clearTimer = new QTimer(this);
    clearTimer->setInterval(2000); // Efface les données reçues toutes les 2 secondes
    connect(clearTimer, &QTimer::timeout, this, [this]() {
        ui.receivedDataText->clear();
        });
    clearTimer->start();

    // Connecter les boutons
    connect(ui.openPortButton, &QPushButton::clicked, this, &QSerialPortTest::onOpenPortButtonClicked);
    connect(ui.sendButton, &QPushButton::clicked, this, &QSerialPortTest::onSendMessageButtonClicked);
}

QSerialPortTest::~QSerialPortTest()
{
    if (port) {
        if (port->isOpen()) {
            port->close();
        }
        delete port;
    }
    delete clearTimer;
}

void QSerialPortTest::onOpenPortButtonClicked()
{
    if (ui.portComboBox->currentIndex() < 0) {
        ui.portStatusLabel->setText("Aucun port sélectionné");
        return;
    }

    if (port) {
        if (port->isOpen()) {
            port->close();
        }
        delete port;
    }

    port = new QSerialPort(ui.portComboBox->currentText(), this);
    connect(port, &QSerialPort::readyRead, this, &QSerialPortTest::onSerialPortReadyRead);

    port->setBaudRate(QSerialPort::Baud9600);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);

    if (port->open(QIODevice::ReadWrite)) {
        ui.portStatusLabel->setText("Port ouvert avec succès");
    }
    else {
        ui.portStatusLabel->setText("Erreur : " + port->errorString());
        delete port;
        port = nullptr;
    }
}

void QSerialPortTest::onSerialPortReadyRead()
{
    if (!port) return;

    static QString buffer; // Stockage temporaire des données reçues
    const QByteArray data = port->readAll();
    buffer.append(data); // Ajouter les nouvelles données reçues

    // Afficher les données brutes dans la zone de texte sans les effacer
    ui.receivedDataText->insertPlainText(QString::fromUtf8(data));

    // Diviser le buffer en lignes complètes
    const QStringList lines = buffer.split('\n');
    buffer = lines.last(); // Garder la dernière ligne incomplète dans le buffer

    // Traiter chaque ligne complète
    for (const QString& line : lines) {
        if (!line.startsWith("$GPGGA")) continue; // Filtrer uniquement les trames GPGGA

        const QStringList parts = line.split(',');
        if (parts.size() > 5) {
            bool latOk = false, lonOk = false;

            // Extraire les données de latitude et longitude
            double rawLatitude = parts[2].toDouble(&latOk) / 100.0;
            double rawLongitude = parts[4].toDouble(&lonOk) / 100.0;
            QString latDir = parts[3];
            QString lonDir = parts[5];

            if (latOk && lonOk) {
                // Appliquer les corrections en fonction de la direction (N/S, E/W)
                double latitude = (latDir == "S") ? -rawLatitude : rawLatitude;
                double longitude = (lonDir == "W") ? -rawLongitude : rawLongitude;

                // Mettre à jour les labels
                ui.latitudeLabel->setText(QString::number(latitude, 'f', 6));
                ui.longitudeLabel->setText(QString::number(longitude, 'f', 6));

                // Envoyer les coordonnées au serveur Node.js
                sendCoordinatesToServer(latitude, longitude);
            }
        }
    }
}

void QSerialPortTest::sendCoordinatesToServer(double latitude, double longitude)
{
    // Remplacez l'URL PHP par celle de votre serveur Node.js
    QUrl url("http://localhost:3000/recevoir_coordinates");  // Assurez-vous que le port correspond à celui de votre serveur Node.js
    QNetworkRequest request(url);

    // Créez un gestionnaire de réseau
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    // Créez la requête POST avec les données à envoyer
    QUrlQuery params;
    params.addQueryItem("latitude", QString::number(latitude, 'f', 6));
    params.addQueryItem("longitude", QString::number(longitude, 'f', 6));

    // Configurez la requête pour envoyer les données en POST
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Envoi de la requête POST
    manager->post(request, params.toString().toUtf8());

    // Connecter la réponse à un gestionnaire
    connect(manager, &QNetworkAccessManager::finished, this, &QSerialPortTest::onRequestFinished);
}

void QSerialPortTest::onRequestFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "Données envoyées avec succès";
    }
    else {
        qDebug() << "Erreur lors de l'envoi des données : " << reply->errorString();
    }

    // Libérer la mémoire
    reply->deleteLater();
}

void QSerialPortTest::onSendMessageButtonClicked()
{
    if (!port || !port->isOpen()) {
        ui.receivedDataText->insertPlainText("Erreur : le port n'est pas ouvert.\n");
        return;
    }

    const QString message = ui.messageLineEdit->text();
    if (message.isEmpty()) {
        ui.receivedDataText->insertPlainText("Message vide, non envoyé.\n");
        return;
    }

    const QByteArray data = message.toUtf8();
    const qint64 bytesWritten = port->write(data);

    if (bytesWritten == -1) {
        ui.receivedDataText->insertPlainText("Erreur lors de l'envoi du message.\n");
    }
    else {
        ui.receivedDataText->insertPlainText("Envoyé : " + message + "\n");
    }
}
