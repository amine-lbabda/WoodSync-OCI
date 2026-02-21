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
