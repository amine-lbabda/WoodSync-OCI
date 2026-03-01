#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <QMessageBox>
#include <QCalendarWidget>
#include "employes.h"
#include <QTimer>
#include <QRandomGenerator64>
#include <QCryptographicHash>
using namespace std;
using namespace cv;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupCalendar(QCalendarWidget *calendar);

private slots:
    void on_GestionStock_clicked();

    void on_GestionReclamations_clicked();

    void on_GestionEmployes_clicked();

    void on_GestionProduits_clicked();

    void on_GestionMateriels_clicked();

    void on_GestionCommandes_clicked();

    void on_btnAdd_2_clicked();

    void on_btnCancel_2_clicked();

    void on_BtnLogin_clicked();

    void on_BtnLoginFace_clicked();



    void on_ConnectionLink_linkActivated(const QString &link);

    void on_ConnectionLink_2_linkActivated(const QString &link);

    void on_AjoutEmploye_clicked();

    void on_SupprimerEmploye_clicked();

    void on_tableView_doubleClicked(const QModelIndex &index);

    void on_InscriptionEmploye_clicked();

private:
    Ui::MainWindow *ui;
    Employes Etmp;
    VideoCapture cap;
    Ptr<FaceDetectorYN> detector;
    Ptr<FaceRecognizerSF> recognizer;

};
#endif // MAINWINDOW_H
