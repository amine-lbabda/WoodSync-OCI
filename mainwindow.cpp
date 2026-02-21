#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "faceworker.h"
#include <QThread>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QList<QPushButton*> allButtons = this->findChildren<QPushButton*>();
    for (QList<QPushButton*>::Iterator it=allButtons.begin();it != allButtons.end();++it) {
        (*it)->setCursor(Qt::PointingHandCursor);
    }
    qRegisterMetaType<Mat>("Mat");
}

MainWindow::~MainWindow()
{
    delete ui;
    if (cap.isOpened()){
        cap.release();
        destroyWindow(Title);
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
    QThread* thread = new QThread();
    FaceWorker* worker = new FaceWorker();
    worker->moveToThread(thread);
    connect(worker,&FaceWorker::frameReady,this,&MainWindow::handleFrame);
    connect(worker,&FaceWorker::faceRecognized,this,[this,worker,thread](){
        ui->stackedWidget->setCurrentIndex(2);
        ui->GestionEmployes->setChecked(true);
        destroyAllWindows();
        worker->stop();
        thread->quit();

    });
    connect(thread,&QThread::started,worker,&FaceWorker::process);
    connect(worker,&FaceWorker::error,this,[this](bool isError){
        if (isError == true){
            msgBox.setCursor(Qt::PointingHandCursor);
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setWindowTitle(QObject::tr("Erreur"));
            msgBox.setText(QObject::tr("Erreur inconnue ! "));
            msgBox.exec();
        }

    });
    thread->start();

}

void MainWindow::handleFrame(Mat frame)
{
    if (frame.empty()) return;
    cv::imshow(Title,frame);
    waitKey(1);
}


