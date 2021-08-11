#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>


typedef struct Frame
{
    unsigned char head[2];
    unsigned char type;
    unsigned char crc;
    union
    {
        short data[28];
        char cmd[56];
    };
    unsigned short length;
    unsigned char tail[2];
}DataFrame;

typedef void (*receiveCallbackFunc) (DataFrame &frame);

class Communication : public QWidget
{
    Q_OBJECT
public:
    static Communication* GetInstance()
    {
         if(m_pInstance == NULL)
         {
             m_pInstance = new Communication();
         }
         return m_pInstance;
    }

private:
    static Communication *m_pInstance;
    Communication()
    {
        m_receiveCb = NULL;
        m_serialPort = NULL;
    }
    void convertStringToHex(const QString &str, QByteArray &byteData);
    char convertCharToHex(char ch);

private:
    const char * class_log_name = "MB:Communication: ";

    QSerialPort *m_serialPort;
    receiveCallbackFunc m_receiveCb;
    DataFrame m_receivedFrame;
    int m_receivedByteCount = 0;

public:
    int openPort(QSerialPort *serialPort, const QString& str);
    int init();
    void deinit();
    unsigned char getCRC8(DataFrame &frame);


public slots:
    void registerReceivedEventCB(receiveCallbackFunc cb);
    void receiveInfo();
    void sendHexStringInfo(QString &info);
    void sendByteInfo(const QByteArray &info);
    void sendFrame(const DataFrame *frame);

};

#endif // COMMUNICATION_H
