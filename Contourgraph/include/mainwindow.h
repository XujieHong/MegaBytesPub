#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include "contourpoints.h"
#include "communication.h"
#include "camerathread.h"

namespace Ui
{
    class MainWindow;
}

// COMMAND
const char COMMAND_CHECKSTATUS[] = "CHECKSTATUS";
const char COMMAND_DATASTART[]   = "DATASTART";
const char COMMAND_DATASTOP[]    = "DATASTOP";

// ACK
const char ACK_STATUSREADY[]     = "STATUSREADY";
const char ACK_STATUSBUSY[]      = "STATUSBUSY";
const char ACK_STATUSERR[]       = "STATUSERR";
const char ACK_DATARECEIVED[]    = "DATARECEIVED";
const char ACK_DATAERR[]         = "DATAERR";


class MainWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void cmdSignal(DataFrame frame);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event);

private:
    static void receiveCallback(DataFrame &str);
    virtual void timerEvent( QTimerEvent *event);

    void sendCommand(const char *cmd, int timeout_ms = 0);
    void sendContourData();

private slots:
    void on_btnOpenCamera_clicked();
    void on_btnDisplayContour_clicked();
    void on_btnSendContour_clicked();

    void on_cmdReceived(DataFrame frame);

    void on_btnLeft_clicked();

    void on_btnRight_clicked();

    void on_btnUp_clicked();

    void on_btnDown_clicked();

    void on_btnShowContour_clicked();

    void on_textEditH_textChanged();

    void on_textEditV_textChanged();

private:
    static MainWindow* g_pThis;
    const int POINTS_PER_FRAME = 14;

    const int SCREEN_WIDTH = 1024;
    const int SCREEN_HEIGHT = 600;
    const int DISPLAY_WIDTH = 512;
    const int DISPLAY_HEIGHT = 512;

    Ui::MainWindow *ui;
//    Point *m_contour;
//    int m_contourSize;

    // for test only
    Point m_contourReceived[10000];
    int m_contourReceivedCount;


    QImage m_image;

    bool m_showContour;

    ContourPoints m_contourPoints;
    Communication *m_com;
    CameraThread m_camera;

    int m_timerUpdate;
    int m_timerTimeout;
    bool m_isWaitingAck;

    bool m_isHost;

};

#endif // MAINWINDOW_H
