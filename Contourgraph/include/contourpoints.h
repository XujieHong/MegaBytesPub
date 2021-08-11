#ifndef CONTOURPOINTS_H
#define CONTOURPOINTS_H

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include "qpixmap.h"


// = Point =
class Point
{
private:
    friend class ContourPoints;

public:
    int row;
    int col;

public:
    Point();

    bool samePoint(Point b);
    void setPoint(int r, int c);

};

// = Node =
class Border
{
private:
    friend class ContourPoints;
    int seq_num;
    int border_type;

};

class Node
{
private:
    friend class ContourPoints;
    Border border;
    int parent;
    int first_child;
    int next_sibling;

public:
    Node();
    void resetNode();
};

// = NodeVector =
class nodeVector
{
private:
    friend class ContourPoints;
    Node *vector;
    int current_max;
    int current_index;

public:
    nodeVector();

    void initNodeVector();
    void resizeNodeVector();
    void addNodeVector(Node node);
    Node* trimNodeVector(int *vector_size);
};


// = IntVector =
class intVector
{
private:
    friend class ContourPoints;
    int *vector;
    int current_max;
    int current_index;

public:
    intVector();

    void initIntVector();

    void resizeIntVector();
    void addIntVector(int value);
    int* trimIntVector(int *vector_size);
};

// = Pixel =
class Pixel
{
private:
    friend class ContourPoints;
    unsigned char red;
    unsigned char blue;
    unsigned char green;

public:
    Pixel();

    void setPixel(unsigned char r, unsigned char g, unsigned char b);
};

// = PointVector =
class pointVector
{
private:
    Point *vector;
    int current_max;
    int current_index;

public:
    pointVector();

    void initPointVector();
    void resizePointVector();
    void addPointVector(Point point);
    Point* trimPointVector(int *vector_size);
};

// = Point2dVector =
class point2dVector
{
private:
    Point **vector;
    int current_max;
    int current_index;

public:
    point2dVector();

    void initPoint2dVector();
    void resizePoint2dVector();
    void addPoint2dVector(Point *point_vector);
    Point** trimPoint2dVector(int *vector_size);
};


// = ContourPoints =
class ContourPoints
{
private:
    Point *m_contour;
    Point m_center;
    int m_contourSize;

    Point m_offsetPoint;


private:
    short** create2dArray(int r, int c);
    void free2dArray(short **arr, int r);

    void stepCCW(Point *current, Point pivot);
    void stepCW(Point *current, Point pivot);
    bool pixelOutOfBounds(Point contoursp, int numrows, int numcols);
    void markExamined(Point mark, Point center, bool checked[4]);
    bool isExamined(bool checked[4]);
    void followBorder(short **image, int numrows, int numcols, int row, int col, Point p2,
                      Border NBD, point2dVector *contour_vector, intVector *contour_counter);
    cv::Mat QImageToCvMat( const QImage &inImage, bool inCloneImageData = true );

    short** getMonoDataFromCvMat(const cv::Mat &imageSrc);
    void drawContour(Point **contours, int *contour_index, Pixel **color, int seq_num, Pixel pix);
    Pixel chooseColor(int n);
    Pixel** createChannels(int h, int w, Point **contours, int *contour_index, int contour_size);
    void saveImageFile(const char * file_name, int h, int w, Point **contours, int *contour_index, int contour_size);
    int getMaxContourID(int rows, int cols, Point **contours, int *contour_index, int contour_size);

    void calculateCenter();


public:
    ContourPoints();
    ~ContourPoints();

    void clone(Point *pData, int length);

    void createContourFromImage(const QImage &inImage, int numrows, int numcols);
    int getContourSize()
    {
        return m_contourSize;
    }

    Point *getContourData()
    {
        return m_contour;
    }

    Point getCenter()
    {
        return m_center;
    }

    void setOffset(int x, int y)
    {
        m_offsetPoint.col = x;
        m_offsetPoint.row = y;
    }

    void addOffset(int x, int y)
    {
        m_offsetPoint.col += x;
        m_offsetPoint.row += y;
    }

    Point getOffsetPoint()
    {
        return m_offsetPoint;
    }

    //void getContourByCV(const QImage &inImage, int numrows, int numcols, Point **outContour, int &outContourIndex);
};

#endif // CONTOURPOINTS_H
