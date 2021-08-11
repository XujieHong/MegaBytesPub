#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QObject>
#include "v4l2.h"
#include <QImage>
#include <QPainter>
#include <QLabel>

class CameraThread : public QObject
{
    Q_OBJECT
public:
    CameraThread();
    ~CameraThread();

    bool open(char *devPath, int fps);
    bool close();
    bool start();
    bool stop();

    void draw(QPainter &painter, int x, int y, int width, int height);

    bool getImage(QImage &outImg);

    bool m_isOpen;

private:
    V4L2 m_v4l;
};

#endif // CAMERATHREAD_H
