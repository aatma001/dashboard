#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include "speedometer.h"
#include "batterygauge.h"
#include <unistd.h>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <iostream>
#include <canframeid.h>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>

using namespace std;

// Global.
QCanBusDevice *device;
QObject *object;
QObject *speedoMeter;

QObject *ptrCompass;
Speedometer *ptrSpeedometer;
Batterygauge *ptrBatteryGauge;


int decodeFrame(const QCanBusFrame &frame)
{
    int value;
    const QByteArray payload = frame.payload();

    if(frame.isValid())
    {
        switch(frame.frameId())
        {
            case VEHICLE_SPEED:
            {
                unsigned short b3, b4;

                b3 = payload[3];
                b4 = payload[4];
                value = b3 + b4;
                break;
            }

        }

        return value;
    }

    value = 1;

    return value;
}

void setSpeedometerColor(int speed)
{
    if(speed < 60) {
        ptrSpeedometer->setProperty("speedColor", QColor(255,0,0));
    } else {
        ptrSpeedometer->setProperty("speedColor", QColor(128,255,0));
    }
}

int decodeBatteryLevel(const QCanBusFrame &frame)
{
    const QByteArray payload = frame.payload();

    if(frame.isValid())
    {
        double value = (payload[5] + payload[6]) / 1000;
        return qRound(value);
    }

    return 0;
}

int dedoceCompass(const QCanBusFrame &frame)
{
    int value;
    const QByteArray payload = frame.payload();
    if(frame.isValid())
    {
        switch(frame.frameId())
        {
            case VEHICLE_SPEED:
            {
            QString valueString = QString::number(payload[0]) + QString::number(payload[1]) + QString::number(payload[2]);


                value = valueString.toInt();
                qDebug() << value;
                return value;
                break;
            }
        }
    }

    value = 1;

    return value;
}

void checkFrames()
{
    while(device->framesAvailable() > 0)
    {
       QCanBusFrame frame = device->readFrame();
        if(frame.isValid())
        {
            switch(frame.frameId())
            {
                case VEHICLE_SPEED:
                    ptrSpeedometer->setProperty("speed", decodeFrame(frame));
                    setSpeedometerColor(decodeFrame(frame));
                    break;

                case BATTERY_INFO:
                    ptrBatteryGauge->setProperty("batteryLevel", decodeBatteryLevel(frame));
                    break;


                default:
                    break;
            }
        }
    }
}

QCanBusDevice::Filter setCanFilter(const unsigned short &id)
{
    QCanBusDevice::Filter filter;

    filter.frameId = id;
    filter.frameIdMask = 0x7FFu; // Compare against all 11-bits of frame id.
    filter.format = QCanBusDevice::Filter::MatchBaseFormat;
    filter.type = QCanBusFrame::DataFrame;

    return filter;
}
void sendCan (Speedometer *speedometer)
{
    qDebug() << speedometer;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);


    qmlRegisterType< Speedometer>("com.dashboard.speedometer",1,0,"Speedometer");
    qmlRegisterType< Batterygauge>("com.dashboard.batterygauge",1,0,"Batterygauge");

    QQmlApplicationEngine engine;

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    QObject *object = engine.rootObjects()[0];
    qDebug() << object;

    //object->setProperty("text", "lol");


    QObject *speedometer = object->findChild<QObject*>("speedoMeter");
    QObject *batterygauge = object->findChild<QObject*>("batteryGauge");
    QObject *rightColor = object->findChild<QObject*>("rightColor");

    rightColor->setProperty("color", QColor(128,255,0));

    //QObject *compass = object->findChild<QObject*>("compass");


    ptrBatteryGauge = qobject_cast<Batterygauge*>(batterygauge);
    ptrSpeedometer = qobject_cast<Speedometer*>(speedometer);
    //ptrCompass = qobject_cast<QObject*>(compass);

    ptrSpeedometer->setProperty("speed", 67);
    setSpeedometerColor(120);

    ptrBatteryGauge->setProperty("batteryLevel", 84);


    //ptrCompass->setProperty("rotation", 60);

    //ptrSpeedometer->setSpeed(3500);

    if (engine.rootObjects().isEmpty())
        return -1;

    //QObject::connect(canTimer, &QTimer::timeout, [ptrSpeedometer, voltage] {
      //  checkFrames(ptrSpeedometer, voltage);
    //});


    /************************************** CAN Bus functionality ***************************************/

    if(QCanBus::instance()->plugins().contains("socketcan"))
        {

            // Create CAN bus device and connect to can0 via SocketCAN plugin.
            device = QCanBus::instance()->createDevice("socketcan", "can0");

            device->connectDevice();

            // Apply filters to CAN Bus device.
            QList<QCanBusDevice::Filter> filterList;

            filterList.append(setCanFilter(VEHICLE_SPEED));

            bool fdKey = true;

            device->setConfigurationParameter(QCanBusDevice::RawFilterKey, QVariant::fromValue(filterList));
            device->setConfigurationParameter(QCanBusDevice::CanFdKey, fdKey);

            // Connect framesRecieved signal to slot function for reading frames.
            QObject::connect(device, &QCanBusDevice::framesReceived, checkFrames);
    }
    return app.exec();
}
