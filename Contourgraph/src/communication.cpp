#include "communication.h"
#include <QDebug>

Communication *Communication::m_pInstance;

#ifdef COMPILE_ARM
// The serial port name of the dev board is ttymxc2. ttymxc0 is a USB TTL serial port which is for shell/log.
const char *COM_NAME =  "/dev/ttymxc2";
#else
#ifdef COMPILE_X64
const char *COM_NAME =  "/dev/ttyS0";
#else
#ifdef COMPILE_APPLE
const char *COM_NAME =  "/dev/tty.Plser";
#else
const char *COM_NAME =  "/dev/ttyUSB0";
#endif
#endif
#endif


int Communication::init()
{
    // for test only
//    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
//    {
//        qDebug() << "SerialPortName:" << info.portName();
//    }

    memset(&m_receivedFrame, 0, sizeof(DataFrame));
    m_receivedByteCount = 0;

    if(m_serialPort == NULL)
    {
        m_serialPort = new QSerialPort();
    }

    return openPort(m_serialPort, COM_NAME);
}

void Communication::deinit()
{
    if(m_serialPort)
    {
        if(m_serialPort->isOpen())
        {
            m_serialPort->close();
        }
        delete m_serialPort;
        m_serialPort = NULL;
    }
}

int Communication::openPort(QSerialPort *serialPort, const QString& str)
{
    if(serialPort->isOpen())
    {
        serialPort->clear();
        serialPort->close();
    }

    serialPort->setPortName(str);
    serialPort->setBaudRate(QSerialPort::Baud115200, QSerialPort::AllDirections);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);


    if(!serialPort->open(QIODevice::ReadWrite))
    {
        qDebug() << class_log_name << "Open serial port failed!" << str;
        return -1;
    }
    qDebug() << class_log_name << "Open serial port successfully!" << str;
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(receiveInfo()));

    return 1;
}

void Communication::registerReceivedEventCB(receiveCallbackFunc cb)
{
    m_receiveCb = cb;
}

void Communication::receiveInfo()
{
    qDebug() << class_log_name << "receiveInfo!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    QByteArray dataReceived = m_serialPort->readAll();

    while (m_serialPort->waitForReadyRead(500))
    {
        dataReceived += m_serialPort->readAll();
    }
    int size = dataReceived.length();

    if(size % sizeof(DataFrame) > 0)
    {
        //KenHong todo: imcomplete data, give up and response error
        return;
    }

    int frames = size / sizeof(DataFrame);
    unsigned char *pData = (unsigned char *)dataReceived.data();
    for(int i = 0; i < frames; i++)
    {
        DataFrame *pFrame = (DataFrame *)(&pData[i * sizeof(DataFrame)]);
        // KenHong todo: test if it is a correct frame.
        qDebug() << class_log_name << "Received frame num." << i;
        m_receiveCb(*pFrame);
    }
}

char Communication::convertCharToHex(char ch)
{
    if((ch >= '0') && (ch <= '9'))
    {
        return ch - 0x30;
    }
    else if((ch >= 'A') && (ch <= 'F'))
    {
        return ch - 'A' + 10;
    }
    else if((ch >= 'a') && (ch <= 'f'))
    {
        return ch - 'a' + 10;
    }
    else
    {
        return (-1);
    }
}

void Communication::convertStringToHex(const QString &str, QByteArray &byteData)
{
    int hexdata, lowhexdata;
    int hexdatalen = 0;
    int len = str.length();

    byteData.resize(len / 2);
    char lstr, hstr;
    for(int i = 0; i < len;)
    {
        hstr = str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
        {
            break;
        }
        lstr = str[i].toLatin1();
        hexdata = convertCharToHex(hstr);
        lowhexdata = convertCharToHex(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
        {
            break;
        }
        else
        {
            hexdata = hexdata*16+lowhexdata;
        }
        i++;
        byteData[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    byteData.resize(hexdatalen);
}

void Communication::sendHexStringInfo(QString &info)
{
    QByteArray sendBuf;
    if (info.contains(" "))
    {
        info.replace(QString(" "), QString(""));
    }
    convertStringToHex(info, sendBuf);

    m_serialPort->write(sendBuf);

    qDebug() << class_log_name << "Send HexString info: " << sendBuf;
}

void Communication::sendByteInfo(const QByteArray &info)
{
    m_serialPort->write(info);

    qDebug() << class_log_name << "Send byte info: " << info;
}

void Communication::sendFrame(const DataFrame *frame)
{
    qDebug() << class_log_name << "Send a frame";

    m_serialPort->write((const char *)frame, 64);
}

unsigned char Communication::getCRC8(DataFrame &frame)
{
    unsigned char i;
    unsigned char crc = 0x00;
    unsigned char len = sizeof(DataFrame);
    unsigned char *ptr = (unsigned char *)&frame;

    while(len--)
    {
        crc ^= *ptr++;
        for (i = 8; i > 0; --i)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }

    return (crc);
}
