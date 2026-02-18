#include "mainwindow.h"
#include "connection.h"
#include <QApplication>
#include <QMessageBox>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    Connection &c = Connection::createInstance();
    bool test = c.createConnection();
    if (test){
        w.show();
        QMessageBox::information(nullptr,QObject::tr("Succés"),QObject::tr("Connection établie avec succées !"),QMessageBox::Cancel);
    } else {
        QMessageBox::critical(nullptr,QObject::tr("Erreur"),QObject::tr("Échec de la connection !"),QMessageBox::Cancel);
    }

    return a.exec();
}
