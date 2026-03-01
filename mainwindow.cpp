#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include "employes.h"
#include <QFile>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    loadFaceRegistry();
    detector = FaceDetectorYN::create(detPath.toStdString(), "", Size(640,480), 0.9f, 0.3f, 5000, dnn::DNN_BACKEND_CUDA, dnn::DNN_TARGET_CUDA);
    recognizer = FaceRecognizerSF::create(recPath.toStdString(), "", dnn::DNN_BACKEND_CUDA, dnn::DNN_TARGET_CUDA);
    ui->setupUi(this);
    ui->tableView->setModel(Etmp.afficher());
    ui->stackedWidget->setCurrentIndex(1);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(4,QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(5,QHeaderView::ResizeToContents);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->DateNaissance->setDate(QDate::currentDate());
    ui->DateRecrutementEmploye->setDate(QDate::currentDate());
    QList<QPushButton*> allButtons = this->findChildren<QPushButton*>();
    for (QList<QPushButton*>::Iterator it=allButtons.begin();it != allButtons.end();++it) {
        (*it)->setCursor(Qt::PointingHandCursor);
    }
    QList<QDateEdit*> allDates = this->findChildren<QDateEdit*>();
    for (QList<QDateEdit*>::Iterator it=allDates.begin();it != allDates.end();++it) {
        (*it)->setCalendarPopup(true);
        setupCalendar((*it)->calendarWidget());
    }
}
void MainWindow::setupCalendar(QCalendarWidget *calendar) {
    if (!calendar) return;

    // 1. Force the Palette (The "Mechanical" fix)
    QPalette pal = calendar->palette();
    pal.setColor(QPalette::Window, Qt::white);      // Background
    pal.setColor(QPalette::WindowText, Qt::black);  // Text
    pal.setColor(QPalette::Base, Qt::white);        // Grid Background
    pal.setColor(QPalette::Text, Qt::black);        // Date Numbers
    pal.setColor(QPalette::Button, Qt::white);      // Header Buttons
    pal.setColor(QPalette::ButtonText, Qt::black);  // Header Text
    pal.setColor(QPalette::Highlight, QColor("#ffff")); // Selection Green
    pal.setColor(QPalette::HighlightedText, Qt::white);

    calendar->setPalette(pal);

    // 2. Force the internal view to follow the palette
    calendar->findChild<QAbstractItemView*>()->setPalette(pal);

    // 3. One very specific CSS line to kill the global Dark Theme
    calendar->setStyleSheet("background-color: white; color: black; border: 1px solid #ccc;");
}
MainWindow::~MainWindow()
{
    delete ui;
    if (cap.isOpened()) {
        cap.release();
        destroyAllWindows();
    }

}

void MainWindow::on_GestionStock_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}


void MainWindow::on_GestionReclamations_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}


void MainWindow::on_GestionEmployes_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
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
    QString nom = ui->NomLoginMenuisier->text();
    QString prenom = ui->PrenomLoginMenuisier->text();
    QString mdp = ui->MdpLoginMenuisier->text();
    Employes e;
    e.setNom(nom);
    e.setPrenom(prenom);
    e.setMdp(mdp);
    if (e.existanceCompte()) {
        ui->stackedWidget->setCurrentIndex(2);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vérifier votre mdp !"));
    }

}

void MainWindow::on_BtnLoginFace_clicked()
{
    cap.open(0);
    if (!cap.isOpened()) return;
    Mat frame,faces;
    bool isSuccess = false;
    while (cap.read(frame) && !isSuccess) {
        if (frame.empty()) break;
        detector->setInputSize(frame.size());
        detector->detect(frame,faces);
        if (faces.rows > 0) {
            Mat alignedLive,liveFeature;
            recognizer->alignCrop(frame,faces.row(0),alignedLive);
            recognizer->feature(alignedLive,liveFeature);
            int matchId = -1;
            QString matchName = "UNKNOWN";
            double maxScore = 0.0;
            for (vector<FaceTemplate>::iterator it = registry.begin(); it != registry.end(); ++it) {
                double score = recognizer->match(liveFeature,it->vector);
                if (score > 0.363 && score > maxScore) {
                    maxScore = score;
                    matchId = it->id;
                    matchName = it->name;
                }
            }
            float *f = faces.ptr<float>(0);
            Rect faceRect(f[0],f[1],f[2],f[3]);
            Scalar color;
            if (matchId != -1) {
                color = Scalar(0,255,0);
            } else {
                color = Scalar(0,0,255);
            }
            rectangle(frame,faceRect,color,2);
            putText(frame,matchName.toStdString() + "(" + QString::number(maxScore,'f',2).toStdString() + ")",Point(f[0],f[1]-10),FONT_HERSHEY_SIMPLEX,0.6,color,2);
            imshow("Reconnaissance faciale",frame);
            QThread::msleep(2000);
            if (matchId != -1) {
                isSuccess = true;
            }
            if (waitKey(1) == 27 || !getWindowProperty("Reconnaissance faciale",WND_PROP_VISIBLE)) break;
        }
    }
    cap.release();
    destroyAllWindows();
    if (isSuccess) {
        ui->stackedWidget->setCurrentIndex(2);
    }

}


//void MainWindow::handleFrame(Mat frame)
//{
//}
void MainWindow::on_ConnectionLink_linkActivated(const QString &link)
{
    if (link == "loginPage") {
        ui->stackedWidget->setCurrentIndex(0);
    }
}


void MainWindow::on_ConnectionLink_2_linkActivated(const QString &link)
{
    if (link == "CreateAccount") {
        ui->stackedWidget->setCurrentIndex(1);
    }
}


void MainWindow::on_AjoutEmploye_clicked()
{
    QString nom = ui->nomEmploye->text();
    QString prenom = ui->PrenomEmploye->text();
    QDate date_naissance = ui->DateNaissance->date();
    QDate date_recrutement = ui->DateRecrutementEmploye->date();
    float heures = ui->HeuresTravailleEmploye->value();
    int tel = ui->TelEmploye->text().toInt();
    QString role = ui->RoleEmploye->currentText();
    Employes e(nom,prenom,tel,heures,date_recrutement,date_naissance,role);
    if (ui->AjoutEmploye->text() == "Ajouter") {
        if (nom.isEmpty() || prenom.isEmpty() || date_naissance.isNull() || date_recrutement.isNull()){
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Un des champs sont manquants !"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            return;
        }
        bool isSuccessful = e.ajouter();
        if (isSuccessful){
            QMessageBox msg(QMessageBox::Information,
                            tr("Succés"),
                            tr("L'employé a été ajouté avec succés"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            ui->tableView->setModel(Etmp.afficher());
            ui->DateNaissance->setDate(QDate::currentDate());
            ui->DateRecrutementEmploye->setDate(QDate::currentDate());
            ui->nomEmploye->setText("");
            ui->PrenomEmploye->setText("");
            ui->HeuresTravailleEmploye->setValue(0.0);
            ui->TelEmploye->setText("");
        } else {
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Erreur lors du traitement du requête"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
        }
    } else if (ui->AjoutEmploye->text() == "Modifier") {
        QModelIndex currentIndex = ui->tableView->currentIndex();
        int row = currentIndex.row();
        int id = ui->tableView->model()->index(row,0).data().toInt();

        bool isSuccessful = e.modifier(id);
        if (isSuccessful) {
            QMessageBox msg(QMessageBox::Information,
                            tr("Succés"),
                            tr("L'employé a été modifié avec succés"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            ui->tableView->setModel(Etmp.afficher());
            ui->DateNaissance->setDate(QDate::currentDate());
            ui->DateRecrutementEmploye->setDate(QDate::currentDate());
            ui->nomEmploye->setText("");
            ui->PrenomEmploye->setText("");
            ui->HeuresTravailleEmploye->setValue(0.0);
            ui->TelEmploye->setText("");
            ui->AjoutEmploye->setText("Ajouter");
        } else {
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Erreur lors du traitement du requête"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
        }
    }

}


void MainWindow::on_SupprimerEmploye_clicked()
{
    QModelIndex currentIndex = ui->tableView->currentIndex();

    if (!currentIndex.isValid()) {
        QMessageBox msg(QMessageBox::Critical,
                        tr("Erreur"),
                        tr("Séléctionnez un employé !"),
                        QMessageBox::Ok,
                        nullptr);
        msg.setCursor(Qt::PointingHandCursor);
        msg.exec();
        return;
    }
    int row = currentIndex.row();
    int id = ui->tableView->model()->index(row,0).data().toInt();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr,"Confirmation","Êtes-vous sûr de supprimer ce employé?");
    if (reply == QMessageBox::Yes) {
        bool isSuccessful = Etmp.supprimer(id);
        if (isSuccessful) {
            QMessageBox msg(QMessageBox::Information,
                            tr("Succés"),
                            tr("L'employé a été supprimé avec succés"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            ui->tableView->setModel(Etmp.afficher());
        } else {
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Erreur lors du traitement du requête"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
        }
    }

}


void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    int row = index.row();
    QString nom = ui->tableView->model()->index(row,1).data().toString();
    QString prenom = ui->tableView->model()->index(row,2).data().toString();
    QDate dateNaissance = ui->tableView->model()->index(row,6).data().toDate();
    QString role = ui->tableView->model()->index(row,7).data().toString();
    QString tel = ui->tableView->model()->index(row,3).data().toString();
    QDate dateRecrutement = ui->tableView->model()->index(row,5).data().toDate();
    double heures = ui->tableView->model()->index(row,4).data().toDouble();

    ui->DateNaissance->setDate(dateNaissance);
    ui->DateRecrutementEmploye->setDate(dateRecrutement);
    ui->nomEmploye->setText(nom);
    ui->PrenomEmploye->setText(prenom);
    ui->HeuresTravailleEmploye->setValue(heures);
    ui->TelEmploye->setText(tel);
    ui->RoleEmploye->setCurrentText(role);
    ui->AjoutEmploye->setText("Modifier");
}


void MainWindow::on_InscriptionEmploye_clicked()
{
    QString nom = ui->NomMenuisier->text();
    QString prenom = ui->PrenomMenuisier->text();
    QString pwd = ui->MdpMenuisiser->text();
    QDate dateNaissance = ui->DateNaissanceMenuisier->date();
    if (nom.isEmpty() || prenom.isEmpty()) {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Il vous manque le nom/prénom !"));
        return;
    } else if (pwd.isEmpty() || pwd.length() <= 8) {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Votre mot de passe est trés court !"));
        return;
    }
    QByteArray saltBytes;
    saltBytes.resize(128);
    QRandomGenerator64::securelySeeded().fillRange(reinterpret_cast<uint64_t*>(saltBytes.data()),8);
    QString mdp_salt = saltBytes.toHex();
    QString mdp = QCryptographicHash::hash((pwd+mdp_salt).toUtf8(),QCryptographicHash::Sha512).toHex();
    Employes e(nom,prenom,0,0,QDate::currentDate(),dateNaissance,"Menuisier",mdp,mdp_salt);

    if (e.ajoutCompte()){
        QMessageBox::information(nullptr,tr("Succées"),tr("Votre compte a été crée avec succés"));
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr,tr("Reconaissance faciale"),tr("Voulez-vous configurer votre reconnaissance faciale?"),QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            cap.open(0);
            if (!cap.isOpened()) {
                qDebug() << "Could not open camera!";
                return;
            }

            cv::Mat frame, faces;
            while (cap.read(frame)) {
                if (frame.empty()) break;

                // Update detector for the camera frame size
                detector->setInputSize(frame.size());
                detector->detect(frame, faces);
                if (faces.rows > 0) {
                    float *f = faces.ptr<float>(0);
                    Rect faceRect(f[0],f[1],f[2],f[3]);
                    rectangle(frame,faceRect,Scalar(255,255,0),2);
                    putText(frame,"Appuyez sur s pour sauvgarder votre image !",Point(10,30),FONT_HERSHEY_COMPLEX,0.7,Scalar(255,255,0),2);
                    imshow("Enregistrement biométrique WoodSync",frame);
                    int key = waitKey(1);
                    if (key == 's' || key == 'S'){
                        if (faces.rows == 1) {
                            Mat alignedFace,featureVector;
                            recognizer->alignCrop(frame,faces.row(0),alignedFace);
                            recognizer->feature(alignedFace,featureVector);
                            QByteArray data(reinterpret_cast<const char*>(featureVector.data),featureVector.total()*featureVector.elemSize());
                            if (e.ajoutReconaissanceFaciale(data)) {
                                QMessageBox::information(nullptr,tr("Succés"),tr("Profile biométrique sauvgardé avec succés !"));
                                break;
                            } else {
                                QMessageBox::critical(nullptr,tr("Échec"),tr("Une personne doit être visible"));
                            }
                        }
                    }

                    // A. Align and Extract from the LIVE Camera Frame
                }
            }
            cap.release();
            cv::destroyAllWindows();
        } else {
            ui->stackedWidget->setCurrentIndex(2);
        }
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Erreur lors du traitement du votre requête !"));
    }
}

void MainWindow::loadFaceRegistry() {
    registry.clear();
    QSqlQuery query("SELECT IDEMPLOYE, NOM, FACE_EMBEDDING FROM EMPLOYES");

    while (query.next()) {
        QByteArray data = query.value(2).toByteArray();
        if (!data.isEmpty()) {
            FaceTemplate ft; //
            ft.id = query.value(0).toInt();
            ft.name = query.value(1).toString();

            // Reconstruct the 1x128 float matrix
            // Use .clone() so the data isn't lost when the QByteArray goes out of scope
            ft.vector = cv::Mat(1, 128, CV_32F, const_cast<char*>(data.data())).clone();

            registry.push_back(ft); // This will now match the vector<FaceTemplate>
        }
    }
}

/*
 * void MainWindow::loadFaceRegistry() {
    registry.clear();
    QSqlQuery query("SELECT IDEMPLOYE, NOM, FACE_EMBEDDING FROM EMPLOYES");

    while (query.next()) {
        QByteArray data = query.value(2).toByteArray();
        if (!data.isEmpty()) {
            FaceTemplate ft;
            ft.id = query.value(0).toInt();
            ft.name = query.value(1).toString();

            // DO NOT point to data.data() directly.
            // Copy the bytes into a temporary vector first to ensure alignment
            std::vector<float> buffer(data.size() / sizeof(float));
            memcpy(buffer.data(), data.constData(), data.size());

            // Create the Mat from the safe buffer and CLONE it
            ft.vector = cv::Mat(buffer).clone();

            registry.push_back(ft);
        }
    }
}
 */

