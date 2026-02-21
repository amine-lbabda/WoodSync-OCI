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
    QMessageBox msgBox;
    msgBox.setCursor(Qt::PointingHandCursor);
    msgBox.setStandardButtons(QMessageBox::Ok);
    if (test){
        w.show();
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowTitle(QObject::tr("Succés"));
        msgBox.setText(QObject::tr("Connection établie avec succées !"));
        msgBox.exec();
        return a.exec();
    } else {
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(QObject::tr("Erreur"));
        msgBox.setText(QObject::tr("Échec de la connection !"));
        msgBox.exec();
    }
    return -1;
}
