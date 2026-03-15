#include "mainwindow.h"
#include "qtconcurrentrun.h"
#include "ui_mainwindow.h"
#include <QThread>
#include "employes.h"
#include <QFile>
#include <QUuid>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    loadFaceRegistry();
    ignore = QtConcurrent::run([this](){
        detector = FaceDetectorYN::create(detPath.toStdString(), "", Size(640,480), 0.9f, 0.3f, 5000, dnn::DNN_BACKEND_CUDA, dnn::DNN_TARGET_CUDA);
        recognizer = FaceRecognizerSF::create(recPath.toStdString(), "", dnn::DNN_BACKEND_CUDA, dnn::DNN_TARGET_CUDA);
    });
    ui->setupUi(this);
    populateComboBox();
    ReadPasswordJob* job = new ReadPasswordJob("WoodSync",this);
    job->setKey("session_token");
    connect(job,&ReadPasswordJob::finished,this,&MainWindow::onTokenRead);
    job->start();
    ui->tableView->setModel(Etmp.afficher());
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
    ui->stackedWidget->setCurrentIndex(0);
    ui->DeconnecterUtilisateur->hide();
    ui->SideBar->hide();
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
        currentId = e.getId();
        persistSessionUser(currentId);
        ui->DeconnecterUtilisateur->setVisible(true);
        ui->SideBar->setVisible(true);
        ui->stackedWidget->setCurrentIndex(2);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vérifier votre mdp !"));
        return;
    }

}

void MainWindow::on_BtnLoginFace_clicked()
{
    ignore = QtConcurrent::run([this](){
        cap.open(0);
        if (!cap.isOpened()) return;
        Mat frame, faces;
        bool isSuccess = false;

        while (cap.read(frame)) {
            if (frame.empty()) break;

            int matchId = -1;
            detector->setInputSize(frame.size());
            detector->detect(frame, faces);

            if (faces.rows > 0) {
                Mat alignedLive, liveFeature;
                recognizer->alignCrop(frame, faces.row(0), alignedLive);
                recognizer->feature(alignedLive, liveFeature);

                QString matchName = "UNKNOWN";
                double maxScore = 0.0;

                for (vector<FaceTemplate>::iterator it = registry.begin(); it != registry.end(); ++it) {
                    double score = recognizer->match(liveFeature, it->vector);
                    if (score > 0.363 && score > maxScore) {
                        maxScore = score;
                        matchId  = it->id;
                        matchName = it->name;
                    }
                }

                float *f = faces.ptr<float>(0);
                Rect faceRect(f[0], f[1], f[2], f[3]);
                Scalar color = (matchId != -1) ? Scalar(0,255,0) : Scalar(0,0,255);
                rectangle(frame, faceRect, color, 2);
                putText(frame,
                        matchName.toStdString() + " (" + QString::number(maxScore,'f',2).toStdString() + ")",
                        Point(f[0], f[1]-10), FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
            }

            // Display + get key — ALL on main thread
            Mat frameCopy = frame.clone();
            bool shouldBreak = false;
            QMetaObject::invokeMethod(this, [frameCopy, &shouldBreak](){
                imshow("Reconnaissance faciale", frameCopy);
                int key = waitKey(1);
                if (key == 27 || !getWindowProperty("Reconnaissance faciale", WND_PROP_VISIBLE))
                    shouldBreak = true;
            }, Qt::BlockingQueuedConnection);

            if (shouldBreak) break;

            // Show green rectangle for 1 second THEN redirect
            if (matchId != -1) {
                QThread::msleep(500);
                isSuccess = true;
                break;
            }
        }

        cap.release();
        QMetaObject::invokeMethod(this, [this, isSuccess](){
            destroyAllWindows();
            if (isSuccess) {
                ui->stackedWidget->setCurrentIndex(2);
                ui->SideBar->setVisible(true);
            }

        }, Qt::QueuedConnection);
    });

}
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
    QString id = ui->SuperviseurEmploye->currentData().toString();
    QString nom = ui->nomEmploye->text();
    QString prenom = ui->PrenomEmploye->text();
    QDate date_naissance = ui->DateNaissance->date();
    QDate date_recrutement = ui->DateRecrutementEmploye->date();
    float heures = ui->HeuresTravailleEmploye->value();
    int tel = ui->TelEmploye->text().toInt();
    QString role = ui->RoleEmploye->currentText();
    Employes e(nom,prenom,tel,heures,date_recrutement,date_naissance,role);
    QVariant idSupervised;
    if (id.toInt() == -1) {
        idSupervised = QVariant(QMetaType(QMetaType::Int));
    } else {
        idSupervised = id.toInt();
    }
    if (!id.isEmpty()) {
        e.setIdSupervised(idSupervised.toInt());
    }
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
            populateComboBox();
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
        currentId = e.getId();
        persistSessionUser(currentId);
        ui->DeconnecterUtilisateur->setVisible(true);
        QMessageBox::information(nullptr,tr("Succées"),tr("Votre compte a été crée avec succés"));
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr,tr("Reconaissance faciale"),tr("Voulez-vous configurer votre reconnaissance faciale?"),QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            cap.open(0);
            if (!cap.isOpened()) {
                qDebug() << "Could not open camera!";
                return;
            }
            const string WINNAME="Enregistrement biométrique WoodSync";
            namedWindow(WINNAME,WINDOW_AUTOSIZE);
            Mat frame, faces;
            while (cap.read(frame)) {
                if (frame.empty()) break;

                // Update detector for the camera frame size
                detector->setInputSize(frame.size());
                detector->detect(frame, faces);
                if (faces.rows > 0) {
                    float *f = faces.ptr<float>(0);
                    Rect faceRect(f[0],f[1],f[2],f[3]);
                    rectangle(frame,faceRect,Scalar(255,255,0),2);
                    displayOverlay(WINNAME,"Appuyez sur S pour sauvgarder votre image !",2000);
                    imshow(WINNAME,frame);
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
            destroyWindow(WINNAME);
        }
        ui->stackedWidget->setCurrentIndex(2);
        ui->SideBar->setVisible(true);
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

void MainWindow::populateComboBox()
{
    ui->SuperviseurEmploye->clear();
    ui->SuperviseurEmploye->addItem("Séléctionnez quelqu'un",-1);
    QSqlQuery query;
    query.prepare("SELECT IDEMPLOYE,NOM,PRENOM FROM EMPLOYES");
    if(query.exec()){
        while (query.next()) {
            QString id = query.value(0).toString();
            QString fullDisplayName = query.value(1).toString() + " " + query.value(2).toString();
            ui->SuperviseurEmploye->addItem(fullDisplayName,id);
        }
    }

}

void MainWindow::persistSessionUser(int userId)
{
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Etmp.saveSessionToken(token,QDate::currentDate().addDays(30),userId);
    WritePasswordJob* job = new WritePasswordJob("WoodSync",this);
    job->setKey("session_token");
    job->setTextData(token);
    job->start();
    QSettings s("WoodSync","WoodSyncApp");
    s.setValue("userId",userId);
}

void MainWindow::onTokenRead(Job *job)
{
    ReadPasswordJob* readJob = static_cast<ReadPasswordJob*>(job);
    QSettings s("WoodSync","WoodSyncApp");
    int savedId = s.value("userId",-1).toInt();
    if (readJob->error() || readJob->textData().isEmpty() || savedId == -1) {
        ui->stackedWidget->setCurrentIndex(0);
        return;
    }
    QString token = readJob->textData();
    if (Etmp.validateSessionToken(token,savedId)) {
        currentId =savedId;
        ui->DeconnecterUtilisateur->setVisible(true);
        ui->SideBar->setVisible(true);
        ui->stackedWidget->setCurrentIndex(2);
    } else {
        s.remove("userId");
        DeletePasswordJob* del = new DeletePasswordJob("WoodSync",this);
        del->setKey("session_token");
        del->start();
        ui->DeconnecterUtilisateur->setVisible(false);
        ui->stackedWidget->setCurrentIndex(0);
    }
}


void MainWindow::on_ExportEmploye_clicked()
{
    QString filter = "CSV Files (*.csv)";
    QString filePath = QFileDialog::getSaveFileName(this,"Export Employee Data",QDir::homePath() + "/employees.csv",filter);
    if (filePath.isEmpty()) {
        return;
    }
    if (Etmp.exportToCSV(ui->tableView,filePath)) {
        QMessageBox::information(nullptr, "Export Success", "Data saved to: " + filePath);
    } else {
        QMessageBox::critical(nullptr, "Export Failed", "Could not save the file.");
    }
}


void MainWindow::on_ImportEmployes_clicked()
{
    if (Etmp.importCSV(ui->tableView)) {
        QMessageBox::information(nullptr, tr("Succés"),tr("Votre fichier est importé avec succés !"));
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Votre fichier n'a pas été importé avec succés !"));
    }
}


void MainWindow::on_DeconnecterUtilisateur_clicked()
{
    Etmp.clearSessionToken(currentId);
    DeletePasswordJob* job = new DeletePasswordJob("WoodSync",this);
    job->setKey("session_token");
    job->start();
    QSettings s("WoodSync","WoodSyncApp");
    s.remove("userId");
    currentId = -1;
    ui->DeconnecterUtilisateur->setVisible(false);
    ui->SideBar->setVisible(false);
    ui->stackedWidget->setCurrentIndex(0);
    ui->NomLoginMenuisier->setText("");
    ui->PrenomLoginMenuisier->setText("");
    ui->MdpLoginMenuisier->setText("");
}

