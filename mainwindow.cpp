#include "mainwindow.h"
#include "ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->GestionStock->setCursor(Qt::PointingHandCursor);
    ui->GestionCommandes->setCursor(Qt::PointingHandCursor);
    ui->GestionEmployes->setCursor(Qt::PointingHandCursor);
    ui->GestionReclamations->setCursor(Qt::PointingHandCursor);
    ui->GestionProduits->setCursor(Qt::PointingHandCursor);
    ui->GestionMateriels->setCursor(Qt::PointingHandCursor);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (cap.isOpened()){
        cap.release();
        destroyWindow("Test");
    }

}

void MainWindow::on_GestionStock_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void MainWindow::on_GestionReclamations_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}


void MainWindow::on_GestionEmployes_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}


void MainWindow::on_GestionProduits_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
}


void MainWindow::on_GestionMateriels_clicked()
{
    ui->stackedWidget->setCurrentIndex(6);
}


void MainWindow::on_GestionCommandes_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
}


void MainWindow::on_btnAdd_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(7);
}


void MainWindow::on_btnCancel_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(6);
}


void MainWindow::on_BtnLogin_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_BtnLoginFace_clicked()
{
    bool isAuthentificated = false;
    model = face::LBPHFaceRecognizer::create();
    try {
        model->read("/home/amine/Desktop/WoodSync-OCI/woodsync_model.yml");
    } catch (...) {
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(QObject::tr("Erreur"));
        msgBox.setText(QObject::tr("Échec de la connection !"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
    if (!faceCascade.load("/home/amine/Desktop/WoodSync-OCI/haarcascade_frontalface_default.xml")) {
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(QObject::tr("Erreur"));
        msgBox.setText(QObject::tr("Échec de la connection !"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    } else {
        cap.open(0);
        while (!isAuthentificated){
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
                int label = -1;
                double confidence = 0.0;
                model->predict(faceROI,label,confidence);
                Scalar color;
                if (confidence > 70.0 && label != -1){
                    color = Scalar(0,255,0);
                    putText(frame,"ID:1",Point(faceRect.x,faceRect.y - 10),FONT_HERSHEY_SIMPLEX,0.8,color,2);
                    isAuthentificated =true;
                } else {
                    color = Scalar(0,0,255);
                    putText(frame,"Unknown",Point(faceRect.x,faceRect.y - 10),FONT_HERSHEY_SIMPLEX,0.8,color,2);
                    isAuthentificated = false;
                }
                rectangle(frame,faceRect,color,2);

            }
            imshow(Title,frame);
            waitKey(60);
        }
        cap.release();
        destroyWindow(Title);
        if (isAuthentificated) {
            ui->stackedWidget->setCurrentIndex(2);
        }
    }

}

