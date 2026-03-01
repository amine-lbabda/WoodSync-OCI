#ifndef FACEWORKER_H
#define FACEWORKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>
using namespace std;
using namespace cv;
using namespace cv::dnn;
class FaceWorker : public QObject
{
    Q_OBJECT
public:
    explicit FaceWorker(QObject *parent = nullptr);
    void stop();
public slots:
    void process();


signals:
    void employeeRecognized(int employeeId,QString fullName);
    void finished();
    void errorOccured(QString message);
private:
    Ptr<FaceDetectorYN> m_detector;
    Ptr<FaceRecognizerSF> m_recognizer;
    QString m_detectorPath;
    QString m_recognizerPath;
    bool m_running;
    VideoCapture cap;

};

#endif // FACEWORKER_H
