#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include "employes.h"
#include <QFile>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
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
    }

    ui->DateNaissance->calendarWidget()->setStyleSheet(this->styleSheet());
    ui->DateRecrutementEmploye->calendarWidget()->setStyleSheet(this->styleSheet());
    setupCalendar(ui->DateNaissance->calendarWidget());
    setupCalendar(ui->DateRecrutementEmploye->calendarWidget());
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
    // 1. Initialize Models
    QString detPath = "/home/amine/Desktop/WoodSync-OCI/face_detection_yunet_2023mar.onnx";
    QString recPath = "/home/amine/Desktop/WoodSync-OCI/face_recognition_sface_2021dec.onnx";

    // 2. Load the Reference Image (The "Target" to match)
    cv::Mat imageRef = cv::imread("/home/amine/Desktop/dataset/amine/1.jpg");
    if (imageRef.empty()) {
        qDebug() << "Error: Could not load reference image!";
        return;
    }

    // 3. Create Detector & Recognizer
    // We initialize the detector with imageRef size first to extract the target feature
    detector = cv::FaceDetectorYN::create(detPath.toStdString(), "", imageRef.size(), 0.5f, 0.3f, 5000, cv::dnn::DNN_BACKEND_CUDA, cv::dnn::DNN_TARGET_CUDA);
    recognizer = cv::FaceRecognizerSF::create(recPath.toStdString(), "", cv::dnn::DNN_BACKEND_CUDA, cv::dnn::DNN_TARGET_CUDA);

    // 4. Extract the "Stored Feature" from your photo (Done ONCE)
    cv::Mat refFaces, alignedRef, storeFeature;
    detector->detect(imageRef, refFaces);

    if (refFaces.rows > 0) {
        recognizer->alignCrop(imageRef, refFaces.row(0), alignedRef);
        recognizer->feature(alignedRef, storeFeature);
        // We clone to ensure the memory stays valid throughout the loop
        storeFeature = storeFeature.clone();
        qDebug() << "Reference feature extracted. Ready for camera...";
    } else {
        qDebug() << "No face found in reference photo. Login aborted.";
        return;
    }

    // 5. Open Camera
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

        for (int i = 0; i < faces.rows; i++) {
            // A. Align and Extract from the LIVE Camera Frame
            cv::Mat alignedLive, liveFeature;
            recognizer->alignCrop(frame, faces.row(i), alignedLive);
            recognizer->feature(alignedLive, liveFeature);

            // B. Compare Live vs Stored
            // Cosine Similarity: 1.0 is identical, 0.363 is the standard threshold
            double score = recognizer->match(liveFeature, storeFeature);

            // C. Visual Feedback
            float* f = faces.ptr<float>(i);
            cv::Rect faceRect(f[0], f[1], f[2], f[3]);

            cv::Scalar color = (score > 0.363) ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
            QString label = (score > 0.363) ? "AMINE" : "UNKNOWN";
            label += QString(" (%1)").arg(score, 0, 'f', 2);

            cv::rectangle(frame, faceRect, color, 2);
            cv::putText(frame, label.toStdString(), cv::Point(f[0], f[1] - 10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);

            if (score > 0.363) {
                qDebug() << "Match Found! Score:" << score;
                // Optional: break loop and trigger login success here
            }
        }

        cv::imshow("Face Login Test", frame);

        // Process GUI events to keep the window responsive
        QCoreApplication::processEvents();

        // Exit on 'ESC' or if window is closed
        if (cv::waitKey(1) == 27) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
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
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Erreur lors du traitement du votre requête !"));
    }
}

