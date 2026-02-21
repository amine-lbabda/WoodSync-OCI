#ifndef FACEWORKER_H
#define FACEWORKER_H

#include <QObject>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <vector>
using namespace cv;
using namespace std;
class FaceWorker : public QObject
{
    Q_OBJECT
public:
    explicit FaceWorker(QObject *parent = nullptr);
    ~FaceWorker();
public slots:
    void process();
    void stop();

signals:
    void faceRecognized(int userId);
    void frameReady(const Mat& frame);
    void finished();
    void error(bool isError);
private:
    bool m_abort;
    QMutex mutex;
    VideoCapture cap;
    Ptr<face::LBPHFaceRecognizer> model;
    CascadeClassifier faceCascade;
    vector<Rect> faces;
};

#endif // FACEWORKER_H
