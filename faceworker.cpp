#include "faceworker.h"

FaceWorker::FaceWorker(QObject *parent)
    : QObject{parent},m_abort(false)
{
}

FaceWorker::~FaceWorker(){
    stop();
    if (cap.isOpened()){
        cap.release();
    }
}

void FaceWorker::stop(){
    mutex.lock();
    m_abort=true;
    mutex.unlock();
}

void FaceWorker::process(){
    bool isFound = false;
    bool isError = false;
    model = face::LBPHFaceRecognizer::create();
    try {
        model->read("/home/amine/Desktop/WoodSync-OCI/woodsync_model.yml");
    } catch (...) {
        isError = true;
        emit error(isError);
        return;
    }
    if (!faceCascade.load("/home/amine/Desktop/WoodSync-OCI/haarcascade_frontalface_default.xml")) {
        isError = true;
        emit error(isError);
        return;
    } else {
        Mat frame,output;
        cap.open(0);
        int label = -1;
        double confidence = 0.0;
        while (!m_abort){
            cap.read(frame);
            if (frame.empty()){
                break;
            }
            cvtColor(frame,output,COLOR_BGR2GRAY);
            equalizeHist(output,output);
            faceCascade.detectMultiScale(output,faces);
            for (vector<Rect>::iterator it=faces.begin();it != faces.end();++it){
                Rect faceRect = *it;
                Mat faceROI = output(faceRect);
                cv::resize(faceROI,faceROI,Size(200,200));

                model->predict(faceROI,label,confidence);
                Scalar color;
                if (confidence < 70.0 && label == 1){
                    color = Scalar(0,255,0);
                    putText(frame,"ID:1",Point(faceRect.x,faceRect.y - 10),FONT_HERSHEY_SIMPLEX,0.8,color,2);
                    isFound = true;
                } else {
                    color = Scalar(0,0,255);
                    putText(frame,"Unknown",Point(faceRect.x,faceRect.y - 10),FONT_HERSHEY_SIMPLEX,0.8,color,2);
                }
                rectangle(frame,faceRect,color,2);

            }
            emit frameReady(frame);
            waitKey(1);
            if (isFound) {
                emit faceRecognized(label);
                if (cap.isOpened()) {
                    cap.release();
                }
                break;
            }
        }
        emit finished();
    }
}
