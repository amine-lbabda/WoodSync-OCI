#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
