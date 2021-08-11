#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTime>
#include <QMessageBox>

MainWindow* MainWindow::g_pThis = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    g_pThis = this;
    ui = new Ui::MainWindow();
    ui->setupUi(this);
    m_com = Communication::GetInstance();
    m_com->init();
    m_com->registerReceivedEventCB(receiveCallback);

    m_showContour = false;
    m_contourReceivedCount = 0;
    m_isWaitingAck = false;
    m_timerTimeout = -1;
    m_isHost = true;

    connect(this, SIGNAL(cmdSignal(DataFrame)), SLOT(on_cmdReceived(DataFrame)));
}

MainWindow::~MainWindow()
{
    delete ui;
    m_camera.close();
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == m_timerUpdate)
    {
       this->update();
    }
    else if(event->timerId() == m_timerTimeout)
    {
        this->killTimer(m_timerTimeout);
        if(m_isWaitingAck)
        {
            QMessageBox::about(NULL, "警告", "命令超时，磨边机无响应！");
        }

        m_timerTimeout = -1;
        m_isWaitingAck = false;
    }
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHints(QPainter::Antialiasing
                           | QPainter::SmoothPixmapTransform);

    int offsetX = (SCREEN_WIDTH - DISPLAY_WIDTH) / 2;
    int offsetY = (SCREEN_HEIGHT - DISPLAY_HEIGHT) / 2;

    m_camera.draw(painter, offsetX, offsetY, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    painter.setPen(QColor(0, 0, 255));
    painter.drawLine(offsetX, offsetY, offsetX, offsetY + DISPLAY_HEIGHT);
    painter.drawLine(offsetX, offsetY + DISPLAY_HEIGHT, offsetX + DISPLAY_WIDTH, offsetY + DISPLAY_HEIGHT);
    painter.drawLine(offsetX + DISPLAY_WIDTH, offsetY + DISPLAY_HEIGHT, offsetX + DISPLAY_WIDTH, offsetY);
    painter.drawLine(offsetX + DISPLAY_WIDTH, offsetY, offsetX, offsetY);

    if(m_showContour)
    {
        Point *pData = m_contourPoints.getContourData();
        Point center = m_contourPoints.getCenter();
        Point offset = m_contourPoints.getOffsetPoint();

        if(m_isHost)
        {
            for(int i = 0; i < m_contourPoints.getContourSize(); i++)
            {
                painter.setPen(QColor(255, 0, 0));
                painter.drawPoint(pData[i].col + offsetX + center.col + offset.col, pData[i].row + offsetY + center.row + offset.row);
            }
        }
        else
        {
            for(int i = 0; i < m_contourPoints.getContourSize(); i++)
            {
                painter.setPen(QColor(255, 0, 0));
                painter.drawPoint(pData[i].col + offsetX + DISPLAY_WIDTH / 2 + offset.col, pData[i].row + offsetY + DISPLAY_HEIGHT / 2 + offset.row);
            }
        }
    }
}


void MainWindow::receiveCallback(DataFrame &dataFrame)
{
    emit g_pThis->cmdSignal(dataFrame);
}

void MainWindow::on_btnOpenCamera_clicked()
{
#ifdef COMPILE_ARM
    m_camera.open((char *)("/dev/video2"), 30);
#else
    m_camera.open((char *)("/dev/video0"), 30);
#endif

    m_timerUpdate = this->startTimer(33);
}

void MainWindow::on_btnDisplayContour_clicked()
{
    m_camera.getImage(m_image);

    int pixmapWidth = m_image.width();
    int pixmapHeight = m_image.height();


    //m_contourPoints.getContour(m_image.copy((pixmapWidth - DISPLAY_WIDTH) / 2, (pixmapHeight - DISPLAY_HEIGHT) / 2, DISPLAY_WIDTH, DISPLAY_HEIGHT),
    //                           DISPLAY_HEIGHT, DISPLAY_WIDTH, &m_contour, m_contourSize);
    m_contourPoints.createContourFromImage(m_image.copy((pixmapWidth - DISPLAY_WIDTH) / 2, (pixmapHeight - DISPLAY_HEIGHT) / 2, DISPLAY_WIDTH, DISPLAY_HEIGHT),
                                           DISPLAY_HEIGHT, DISPLAY_WIDTH);
    m_isHost = true;
    m_showContour = true;
}

void MainWindow::on_btnSendContour_clicked()
{
    // step 1: check target machine status for sending
    sendCommand(COMMAND_CHECKSTATUS, 3000);
}

void MainWindow::sendCommand(const char *cmd, int timeout_ms)
{
    if(m_isWaitingAck)
    {
        qDebug() << "Previous command is still in sending..";
        return;
    }

    DataFrame dataFrame;
    memset(&dataFrame, 0, sizeof(DataFrame));

    dataFrame.head[0] = 0xAA;
    dataFrame.head[1] = 0xAA;
    dataFrame.type    = 0x01;
    dataFrame.length  = (unsigned short)m_contourPoints.getContourSize();
    dataFrame.crc  = 0x00;
    dataFrame.tail[0] = 0xFE;
    dataFrame.tail[1] = 0xFE;

    memcpy(dataFrame.cmd, cmd, strlen(cmd));

    if(m_com)
    {
        dataFrame.crc = m_com->getCRC8(dataFrame);
        m_com->sendFrame(&dataFrame);
        if(m_timerTimeout != -1)
        {
            this->killTimer(m_timerTimeout);
        }

        if(timeout_ms > 0)
        {
            m_timerTimeout = this->startTimer(timeout_ms);
            m_isWaitingAck = true;
        }
        else
        {
            m_isWaitingAck = false;
        }

        qDebug() << "Send command: " << cmd;
    }
    else
    {
        qDebug() << "Error, m_com is not available.";
    }
}

void MainWindow::sendContourData()
{
    DataFrame dataFrame;
    int frameCount = (m_contourPoints.getContourSize() + POINTS_PER_FRAME - 1) / POINTS_PER_FRAME;
    Point *pData = m_contourPoints.getContourData();
    Point offset = m_contourPoints.getOffsetPoint();

    dataFrame.head[0] = 0xAA;
    dataFrame.head[1] = 0xAA;
    dataFrame.type = 0x02;
    dataFrame.length = (unsigned short)m_contourPoints.getContourSize();
    dataFrame.crc = 0x00;
    dataFrame.tail[0] = 0xFE;
    dataFrame.tail[1] = 0xFE;

    for(int i = 0; i < frameCount; i++)
    {
        for(int j = 0; j < POINTS_PER_FRAME; j++)
        {
            int index = POINTS_PER_FRAME * i + j;
            if(index < m_contourPoints.getContourSize())
            {
                dataFrame.data[j * 2] = pData[index].col + offset.col;
                dataFrame.data[j * 2 + 1] = pData[index].row + offset.row;
            }
            else
            {
                dataFrame.data[j * 2] = 32767;
                dataFrame.data[j * 2 + 1] = 32767;
            }
        }
        dataFrame.crc = 0x00;
        dataFrame.crc = m_com->getCRC8(dataFrame);
        m_com->sendFrame(&dataFrame);

        qDebug() << "Send cloud points of contour, Number of frame sent: " << i;

//        QTime dieTime = QTime::currentTime().addMSecs(10);
//        while (QTime::currentTime() < dieTime)
//        {

//        }
    }
}

void MainWindow::on_cmdReceived(DataFrame frame)
{
    // parse frame
    unsigned char crc = frame.crc;
    frame.crc = 0x00;
    if(crc != m_com->getCRC8(frame))
    {
        QMessageBox::about(NULL, "警告", "crc校验错误, 数据传输失败！");
        return;
    }

    if(frame.type == 0x01)
    {
        m_isWaitingAck = false;
        char *cmd = frame.cmd;
        if(0 == strcmp(COMMAND_CHECKSTATUS, cmd))
        {
            sendCommand(ACK_STATUSREADY);
        }
        else if(0 == strcmp(COMMAND_DATASTART, cmd))
        {
            m_contourReceivedCount = 0;
        }
        else if(0 == strcmp(COMMAND_DATASTOP, cmd))
        {
            // check received points count
            if(frame.length != m_contourReceivedCount)
            {
                sendCommand(ACK_DATAERR);
            }
            else
            {
                sendCommand(ACK_DATARECEIVED);
                m_contourPoints.clone(m_contourReceived, m_contourReceivedCount);
                m_isHost = false;
                m_showContour = true;
                this->update();
            }
        }

        // Host callback process
        if(0 == strcmp(ACK_STATUSREADY, cmd))
        {
            // step 2: send a start request
            sendCommand(COMMAND_DATASTART);

            // step 3: send cloud points of contour
            sendContourData();

            // step 4: send a stop request
            sendCommand(COMMAND_DATASTOP, 3000);
        }
        else if(0 == strcmp(ACK_STATUSBUSY, cmd))
        {
            QMessageBox::about(NULL, "警告", "磨边机正在工作中！");
        }
        else if(0 == strcmp(ACK_STATUSERR, cmd))
        {
            QMessageBox::about(NULL, "警告", "磨边机故障！");
        }
        else if(0 == strcmp(ACK_DATARECEIVED, cmd))
        {
            QMessageBox::about(NULL, "通知", "发送成功！");
        }
        else if(0 == strcmp(ACK_DATAERR, cmd))
        {
            QMessageBox::about(NULL, "警告", "数据传输失败！");
        }
    }
    else if(frame.type == 0x02)
    {
        // Target for simulator test
        for(int i = 0; i < POINTS_PER_FRAME; i++)
        {
            if(frame.data[i * 2] != 32767 && frame.data[i * 2 + 1] != 32767)
            {
                m_contourReceived[m_contourReceivedCount].col = frame.data[i * 2];
                m_contourReceived[m_contourReceivedCount].row = frame.data[i * 2 + 1];
                m_contourReceivedCount++;
            }
        }

        qDebug() << "Cloud points is received, Number of cloudpointReceived: " << m_contourReceivedCount;
    }
}

void MainWindow::on_btnLeft_clicked()
{
    m_contourPoints.addOffset(-1, 0);
    bool ok;
    int offset = ui->textEditH->toPlainText().toInt(&ok);
    ui->textEditH->setText(QString::number(offset - 1));
    this->update();
}

void MainWindow::on_btnRight_clicked()
{
    m_contourPoints.addOffset(1, 0);
    bool ok;
    int offset = ui->textEditH->toPlainText().toInt(&ok);
    ui->textEditH->setText(QString::number(offset + 1));
    this->update();
}

void MainWindow::on_btnUp_clicked()
{
    m_contourPoints.addOffset(0, -1);
    bool ok;
    int offset = ui->textEditV->toPlainText().toInt(&ok);
    ui->textEditV->setText(QString::number(offset - 1));
    this->update();
}

void MainWindow::on_btnDown_clicked()
{
    m_contourPoints.addOffset(0, 1);
    bool ok;
    int offset = ui->textEditV->toPlainText().toInt(&ok);
    ui->textEditV->setText(QString::number(offset + 1));
    this->update();
}

void MainWindow::on_btnShowContour_clicked()
{
    m_isHost = !m_isHost;
    this->update();
}

void MainWindow::on_textEditH_textChanged()
{
    bool ok;
    int offsetX = ui->textEditH->toPlainText().toInt(&ok);
    int offsetY = ui->textEditV->toPlainText().toInt(&ok);
    if(ok)
    {
         m_contourPoints.setOffset(offsetX, offsetY);
    }
    this->update();
}

void MainWindow::on_textEditV_textChanged()
{
    bool ok;
    int offsetX = ui->textEditH->toPlainText().toInt(&ok);
    int offsetY = ui->textEditV->toPlainText().toInt(&ok);
    if(ok)
    {
         m_contourPoints.setOffset(offsetX, offsetY);
    }
    this->update();
}




