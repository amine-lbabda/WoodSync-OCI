/**
 * @file employes.cpp
 * @author Mohamed Amine Lbabda
 * @brief 
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "employes.h"
/**
 * @brief Construct a new Employes:: Employes object
 * 
 */
Employes::Employes() {
    this->id = -1;
    this->nom = "";
    this->prenom = "";
    this->role = "";
    this->tel = "0";
    this->heures = 0.0;
    this->date_recrutement = QDate::currentDate();
    this->date_naissance = QDate::currentDate();
    this->mdp="";
    this->mdp_hash="";
    this->id_supervised=-1;
}
/**
 * @brief Construct a new Employes:: Employes object
 * 
 * @param nom 
 * @param prenom 
 * @param tel 
 * @param heures 
 * @param date_recrutement 
 * @param date_naissance 
 * @param role 
 * @param mdp 
 * @param mdp_hash 
 * @param id_supervised 
 * @param id 
 */
Employes::Employes(QString nom, QString prenom, QString tel, float heures, QDate date_recrutement, QDate date_naissance, QString role, QString mdp, QString mdp_hash, int id_supervised, int id)
{
    this->id = id;
    this->nom = nom;
    this->prenom = prenom;
    this->tel = tel;
    this->heures = heures;
    this->date_recrutement = date_recrutement;
    this->date_naissance = date_naissance;
    this->role = role;
    this->mdp = mdp;
    this->mdp_hash = mdp_hash;
    this->id_supervised = id_supervised;

}
/**
 * @brief Adding an employee to the database
 * 
 * @return true 
 * @return false 
 */
bool Employes::ajouter()
{
    QSqlQuery query;
    if (id_supervised == -1) {
        query.prepare("INSERT INTO EMPLOYES (NOM,PRENOM,TEL,HEURETRAVAILLE,DATERECRUTEMENT,DATENAISSANCE,ROLE,TOKEN_EXPIRY) VALUES(?,?,?,?,?,?,?,NULL)");
        query.addBindValue(nom);
        query.addBindValue(prenom);
        query.addBindValue(tel);
        query.addBindValue(heures);
        query.addBindValue(date_recrutement);
        query.addBindValue(date_naissance);
        query.addBindValue(role);
    } else {
        query.prepare("INSERT INTO EMPLOYES (NOM,PRENOM,TEL,HEURETRAVAILLE,DATERECRUTEMENT,DATENAISSANCE,ROLE,IDSUPERVISEUR,TOKEN_EXPIRY) VALUES(?,?,?,?,?,?,?,?,NULL)");
        query.addBindValue(nom);
        query.addBindValue(prenom);
        query.addBindValue(tel);
        query.addBindValue(heures);
        query.addBindValue(date_recrutement);
        query.addBindValue(date_naissance);
        query.addBindValue(role);
        query.addBindValue(id_supervised);
    }


    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee added successfully!";
        return true;
    }
}
/**
 * @brief Shwoing an employee on the tableview 
 * 
 * @return QSqlQueryModel* 
 */
QSqlQueryModel *Employes::afficher()
{
    QSqlQuery query;
    query.prepare(R"(
        SELECT
        e.IDEMPLOYE,
        e.NOM,
        e.PRENOM,
        e.TEL,
        e.HEURETRAVAILLE,
        TO_CHAR(e.DATERECRUTEMENT,'DD/MM/YYYY'),
        TO_CHAR(e.DATENAISSANCE,'DD/MM/YYYY'),
        CASE
            WHEN s.IDEMPLOYE IS NULL THEN 'None'
            ELSE CONCAT(s.NOM,CONCAT(' ',s.PRENOM))
        END AS SUPERVISEUR,
        e.ROLE
        FROM EMPLOYES e
        LEFT JOIN EMPLOYES s
        ON e.IDSUPERVISEUR = s.IDEMPLOYE
        WHERE e.IDEMPLOYE <> :id
        ORDER BY IDEMPLOYE ASC
)");
    query.bindValue(":id",id);

    if (!query.exec()) {
        qDebug() << "Oracle Error (afficher):" << query.lastError().text();
        return nullptr;
    }

    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery(std::move(query));
    if (model->lastError().isValid()) {
        qDebug() << "Error: " << model->lastError().text();
        delete model;
        return nullptr;
    }
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("Nom"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("Prénom"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("Tel"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("Heures travaillés"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("Date recrutement"));
    model->setHeaderData(6,Qt::Horizontal,QObject::tr("Date naissance"));
    model->setHeaderData(7,Qt::Horizontal,QObject::tr("Superviseur"));
    model->setHeaderData(8,Qt::Horizontal,QObject::tr("Rôle"));
    return model;
}
/**
 * @brief Showing the result of a search on the table 
 * 
 * @param nom 
 * @return QSqlQueryModel* 
 */
QSqlQueryModel *Employes::rechercher(QString nom)
{
    QString searchText = nom.trimmed();
    QString pattern = "%" + searchText + "%";
    QSqlQuery query;
    query.prepare(R"(
        SELECT
        e.IDEMPLOYE,
        e.NOM,
        e.PRENOM,
        e.TEL,
        e.HEURETRAVAILLE,
        TO_CHAR(e.DATERECRUTEMENT,'DD/MM/YYYY'),
        TO_CHAR(e.DATENAISSANCE,'DD/MM/YYYY'),
        CASE
            WHEN s.IDEMPLOYE IS NULL THEN 'None'
            ELSE CONCAT(s.NOM,CONCAT(' ',s.PRENOM))
        END AS SUPERVISEUR,
        e.ROLE
        FROM EMPLOYES e
        LEFT JOIN EMPLOYES s
        ON e.IDSUPERVISEUR = s.IDEMPLOYE
        WHERE (UPPER(TRIM(e.NOM)) LIKE UPPER(:termNom)
        OR UPPER(TRIM(e.PRENOM)) LIKE UPPER(:termPrenom)
        OR UPPER(TRIM(e.NOM || ' ' || e.PRENOM)) LIKE UPPER(:termFull))
        AND e.IDEMPLOYE <> :id
)"
    );
    query.bindValue(":termNom", pattern);
    query.bindValue(":termPrenom", pattern);
    query.bindValue(":termFull", pattern);
    query.bindValue(":termId", pattern);
    query.bindValue(":id",id);
    if (!query.exec()) {
        qDebug() << "Oracle error (rechercher):" << query.lastError().text() << "search=" << searchText;
        return nullptr;
    }

    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery(std::move(query));
    if (model->lastError().isValid()) {
        qDebug() << "Model error (rechercher):" << model->lastError().text();
        delete model;
        return nullptr;
    }
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("Nom"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("Prénom"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("Tel"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("Heures travaillés"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("Date recrutement"));
    model->setHeaderData(6,Qt::Horizontal,QObject::tr("Date naissance"));
    model->setHeaderData(7,Qt::Horizontal,QObject::tr("Superviseur"));
    model->setHeaderData(8,Qt::Horizontal,QObject::tr("Rôle"));
    return model;
}
/**
 * @brief Sorting table employees based on a criteria
 * 
 * @param choice 
 * @return QSqlQueryModel* 
 */
QSqlQueryModel *Employes::trier(QString choice)
{
    QSqlQueryModel* model = new QSqlQueryModel();
    QSqlQuery query;
    QString col;
    if (choice == "Recrutement") {
        col = "e.DATERECRUTEMENT";
    } else if (choice == "Naissance") {
        col = "e.DATENAISSANCE";
    } else if (choice == "Heures travaillés") {
        col = "e.HEURETRAVAILLE";
    }
    QString queryString = QString(R"(
        SELECT
        e.IDEMPLOYE,
        e.NOM,
        e.PRENOM,
        e.TEL,
        e.HEURETRAVAILLE,
        TO_CHAR(e.DATERECRUTEMENT,'DD/MM/YYYY'),
        TO_CHAR(e.DATENAISSANCE,'DD/MM/YYYY'),
        CASE
            WHEN s.IDEMPLOYE IS NULL THEN 'None'
            ELSE CONCAT(s.NOM,CONCAT(' ',s.PRENOM))
        END AS SUPERVISEUR,
        e.ROLE
        FROM EMPLOYES e
        LEFT JOIN EMPLOYES s
        ON e.IDSUPERVISEUR = s.IDEMPLOYE
        WHERE e.IDEMPLOYE <> :id
        ORDER BY %1 ASC
)").arg(col);

    query.prepare(queryString);
    query.bindValue(":id",id);
    if (!query.exec()) {
        qDebug() << "Sort Query Error:" << query.lastError().text();
        delete model;
        return nullptr;
    }
    model->setQuery(std::move(query));
    if (model->lastError().isValid()) {
        qDebug() << "Model error:" << model->lastError().text();
        delete model;
        return nullptr;
    }

    model->setHeaderData(0,Qt::Horizontal,QObject::tr("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("Nom"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("Prénom"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("Tel"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("Heures travaillés"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("Date recrutement"));
    model->setHeaderData(6,Qt::Horizontal,QObject::tr("Date naissance"));
    model->setHeaderData(7,Qt::Horizontal,QObject::tr("Superviseur"));
    model->setHeaderData(8,Qt::Horizontal,QObject::tr("Rôle"));
    return model;
}

/**
 * @brief Updating an employee data 
 * 
 * @param id 
 * @return true 
 * @return false 
 */
bool Employes::modifier(int id)
{
    QString res = QString::number(id);
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET NOM=:nom,PRENOM=:prenom,TEL=:tel,HEURETRAVAILLE=:heures,DATERECRUTEMENT=:recru,DATENAISSANCE=:naissance,ROLE=:role,UPDATED_AT=SYSDATE,IDSUPERVISEUR=:sup WHERE IDEMPLOYE=:id");
    query.bindValue(":nom",nom);
    query.bindValue(":prenom",prenom);
    query.bindValue(":tel",tel);
    query.bindValue(":heures",heures);
    query.bindValue(":recru",date_recrutement);
    query.bindValue(":naissance",date_naissance);
    query.bindValue(":role",role);
    query.bindValue(":id",res);
    if (id_supervised != -1) {
        query.bindValue(":sup",id_supervised);
    } else {
        query.bindValue(":sup",QVariant(QMetaType::fromType<int>()));
    }
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        return true;
    }
}
/**
 * @brief Deleting an employee based on the ID 
 * 
 * @param id 
 * @return true 
 * @return false 
 */
bool Employes::supprimer(int id)
{
    QString res = QString::number(id);
    QSqlQuery query;
    query.prepare("DELETE FROM EMPLOYES WHERE IDEMPLOYE=?");
    query.addBindValue(res);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        return true;
    }
}
/**
 * @brief Adding an account to access the interface 
 * 
 * @return true 
 * @return false 
 */
bool Employes::ajoutCompte()
{
    QSqlQuery query,seqQuery;
    seqQuery.exec("SELECT EMPLOYES_SEQ.NEXTVAL FROM DUAL");
    if (!seqQuery.next()) return false;
    id = seqQuery.value(0).toInt();
    query.prepare("INSERT INTO EMPLOYES (IDEMPLOYE,NOM,PRENOM,MDP,MDP_SALT,ROLE,CREATED_AT) VALUES(?,?,?,?,?,?,?)");
    query.addBindValue(id);
    query.addBindValue(nom);
    query.addBindValue(prenom);
    query.addBindValue(mdp);
    query.addBindValue(mdp_hash);
    query.addBindValue("Menuisier");
    query.addBindValue(QDate::currentDate());
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee added successfully!";
        return true;
    }
}
/**
 * @brief Checking whether the account exists or not
 * 
 * @return true 
 * @return false 
 */
bool Employes::existanceCompte()
{
    QSqlQuery query;
    query.prepare("SELECT MDP,MDP_SALT,IDEMPLOYE FROM EMPLOYES WHERE NOM=? AND PRENOM=?");
    query.addBindValue(nom);
    query.addBindValue(prenom);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        if (query.next()) {
            QString storedHash = query.value(0).toString();
            QString storedSalt = query.value(1).toString();
            QByteArray verifyBytes = QCryptographicHash::hash((mdp+storedSalt).toUtf8(),QCryptographicHash::Sha512);
            QString verifyHex = verifyBytes.toHex();
            if (verifyHex == storedHash) {
                id = query.value(2).toInt();
                return true;
            }
        }
    }
    return false;
}
/**
 * @brief Ajout de la reconnaissance faciale 
 * 
 * @param data 
 * @return true 
 * @return false 
 */
bool Employes::ajoutReconaissanceFaciale(QByteArray data)
{
    if (data.isEmpty()) {
        return false;
    }
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET FACE_EMBEDDING=? WHERE NOM=? AND PRENOM=?");
    query.addBindValue(data);
    query.addBindValue(nom);
    query.addBindValue(prenom);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee added successfully!";
        return true;
    }
}
/**
 * @brief Export de la liste des employés sous format d'un fichier csv
 * 
 * @param view 
 * @param filePath 
 * @return true 
 * @return false 
 */
bool Employes::exportToCSV(QTableView *view,QString filePath)
{
    StringData data;
    QAbstractItemModel* model = view->model();
    QStringList headers;
    for (int i = 0;i < model->columnCount();++i) {
        headers.push_back(model->headerData(i,Qt::Horizontal).toString());
    }
    data.addRow(headers);
    for (int row = 0;row < model->rowCount();++row) {
        QStringList rowData;
        for (int col=0;col < model->columnCount();++col) {
            rowData.push_back(model->data(model->index(row,col)).toString());
        }
        data.addRow(rowData);
    }
    if (Writer::write(filePath,data)) {
        return true;
    }
    return false;
}
/**
 * @brief Importation d'un fichier csv et insertion de ces données dans la base de données
 * 
 * @param view 
 * @return true 
 * @return false 
 */
bool Employes::importCSV(QTableView *view)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen() || !db.transaction()) {
        return false;
    }

    QString filePath = QFileDialog::getOpenFileName(nullptr, "Import employee data", QDir::homePath(), "CSV Files (*.csv)");
    if (filePath.isEmpty()) {
        db.rollback();
        return false;
    }

    const QList<QStringList> readData = Reader::readToList(filePath);
    if (readData.size() <= 1) {
        db.rollback();
        return false;
    }

    int importedCount = 0;

    for (int i = 1; i < readData.size(); ++i) {
        const QStringList row = readData.at(i);
        if (row.size() < 9) {
            db.rollback();
            return false;
        }

        const QString nom = row.at(1).trimmed();
        const QString prenom = row.at(2).trimmed();
        const QString tel = row.at(3).trimmed().isEmpty() ? "0" : row.at(3).trimmed();
        const float heures = row.at(4).trimmed().isEmpty() ? 0.0f : row.at(4).trimmed().toFloat();
        const QDate dateRecrutement = QDate::fromString(row.at(5).trimmed(), "dd/MM/yyyy");
        const QDate dateNaissance = QDate::fromString(row.at(6).trimmed(), "dd/MM/yyyy");
        const QString superviseur = row.at(7).trimmed();
        const QString role = row.at(8).trimmed();

        if (nom.isEmpty() || prenom.isEmpty() || role.isEmpty() || !dateRecrutement.isValid() || !dateNaissance.isValid()) {
            db.rollback();
            return false;
        }

        setNom(nom);
        setPrenom(prenom);
        setTel(tel);
        setHeures(heures);
        setDate_recrutement(dateRecrutement);
        setDate_naissance(dateNaissance);
        if (role == "Employe") {
            setRole("Employé");
        } else {
            setRole(role);
        }


        setIdSupervised(-1);

        if (!superviseur.isEmpty()) {
            QSqlQuery query;
            query.prepare("SELECT IDEMPLOYE FROM EMPLOYES WHERE TRIM(NOM) || ' ' || TRIM(PRENOM) = ?");
            query.addBindValue(superviseur);

            if (!query.exec() || !query.next()) {
                db.rollback();
                return false;
            }

            const QVariant id = query.value(0);
            if (!id.isValid() || id.toString().isEmpty()) {
                db.rollback();
                return false;
            }

            setIdSupervised(id.toInt());
        }

        if (!ajouter()) {
            db.rollback();
            return false;
        }

        importedCount++;
    }

    if (!db.commit()) {
        db.rollback();
        return false;
    }

    if (importedCount > 0 && view) {
        QSqlQueryModel *model = afficher();
        if (!model) {
            return false;
        }
        view->setModel(model);
    }

    return importedCount > 0;
}
/**
 * @brief Sauvgarder le token générée lors du login
 * 
 * @param token 
 * @param expiry 
 * @param userId 
 * @return true 
 * @return false 
 */
bool Employes::saveSessionToken(const QString &token, const QDate &expiry,int userId)
{
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET SESSION_TOKEN=?,TOKEN_EXPIRY=? WHERE IDEMPLOYE=?");
    query.addBindValue(token);
    query.addBindValue(expiry);
    query.addBindValue(userId);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee added successfully!";
        return true;
    }
}
/**
 * @brief Validation de la validité du token
 * 
 * @param token 
 * @param id 
 * @return true 
 * @return false 
 */
bool Employes::validateSessionToken(const QString &token, int id)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM EMPLOYES WHERE IDEMPLOYE=? AND SESSION_TOKEN=? AND TOKEN_EXPIRY>=TRUNC(SYSDATE)");
    query.addBindValue(id);
    query.addBindValue(token);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Done !";
        return query.next();
    }
}
/**
 * @brief Suppression du token générée lors de la déconnection afin de garantir plus de sécurité
 * 
 * @param id 
 * @return true 
 * @return false 
 */
bool Employes::clearSessionToken(int id)
{
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET SESSION_TOKEN=NULL,TOKEN_EXPIRY=NULL WHERE IDEMPLOYE=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee Removed successfully";
        return true;
    }
}
/**
 * @brief Générer les statistiques des employés (Répartition de la moyenne des heures travaillés par employés)
 * 
 * @return QChart* 
 */
QChart* Employes::genererStatistiquesHeures() {
    QColor backgroundColor(0x414833);
    QColor plotColor(0x4a533f);
    QColor textColor(0xf5f2e8);
    QColor accentColor(0xb8860b);
    QColor axisColor(0xd8d4c0);
    QColor gridColor(133, 141, 121, 170);

    QBarSet *set = new QBarSet("Moyenne d'heures");
    set->setColor(accentColor);
    set->setBorderColor(QColor(0xe2bd56));
    set->setLabelColor(textColor);

    QStringList roles;
    double maxAvg = 0.0;
    bool hasData = false;

    QSqlQuery query("SELECT NVL(AVG(HEURETRAVAILLE), 0), TRIM(ROLE) FROM EMPLOYES GROUP BY ROLE");
    while (query.next()) {
        double avg = qMax(0.0, query.value(0).toDouble());
        QString role = query.value(1).toString().trimmed();

        // Skip categories that have no effective worked hours to keep the chart focused.
        if (avg <= 0.0) {
            continue;
        }

        roles << (role.isEmpty() ? "Non defini" : role);
        *set << avg;
        maxAvg = qMax(maxAvg, avg);
        hasData = true;
    }

    if (!hasData) {
        roles << "Aucune donnee exploitable";
        *set << 0.0;
    }

    QBarSeries *series = new QBarSeries();
    series->append(set);
    series->setBarWidth(0.55);
    series->setLabelsVisible(hasData);
    series->setLabelsFormat("@value h");
    series->setLabelsPosition(QAbstractBarSeries::LabelsInsideEnd);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Moyenne des heures travaillees par role");
    chart->setTitleBrush(QBrush(textColor));
    chart->setTitleFont(QFont("Segoe UI", 12, QFont::DemiBold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(true);
    chart->setBackgroundRoundness(8.0);
    chart->setBackgroundBrush(QBrush(backgroundColor));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(plotColor));
    chart->setMargins(QMargins(28, 14, 18, 32));

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(roles);
    axisX->setLabelsColor(textColor);
    axisX->setLabelsFont(QFont("Segoe UI", 10, QFont::DemiBold));
    axisX->setTruncateLabels(false);
    axisX->setLabelsAngle(0);
    axisX->setLinePen(QPen(axisColor, 1.2));
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    double yMax = maxAvg > 0.0 ? maxAvg * 1.10 : 8.0;
    if (yMax < 8.0) {
        yMax = 8.0;
    }
    axisY->setRange(0.0, yMax);
    axisY->setTickCount(4);
    axisY->setLabelFormat("%.0f h");
    axisY->setLabelsColor(textColor);
    axisY->setLabelsFont(QFont("Segoe UI", 10, QFont::DemiBold));
    axisY->setTitleText("");
    axisY->setTitleBrush(QBrush(textColor));
    axisY->setTitleFont(QFont("Segoe UI", 10, QFont::DemiBold));
    axisY->setLinePen(QPen(axisColor, 1.2));
    QPen gridPen(gridColor, 1.0, Qt::DashLine);
    axisY->setGridLinePen(gridPen);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(false);
    return chart;
}
/**
 * @brief Générer les statistiques des employés (Répartition des employés par la naissance)
 * 
 * @return QChart* 
 */
QChart *Employes::genererStatistiquesNaissances()
{
    const QColor backgroundColor(0x414833);
    const QColor plotColor(0x4a533f);
    const QColor textColor(0xf5f2e8);

    QPieSeries* series = new QPieSeries();
    series->setHoleSize(0.38);
    QSqlQuery query(
        "SELECT "
        " CASE "
        " WHEN TRUNC(MONTHS_BETWEEN(SYSDATE, DATENAISSANCE)/12) < 25 THEN '18-25 ans' "
        " WHEN TRUNC(MONTHS_BETWEEN(SYSDATE, DATENAISSANCE)/12) < 35 THEN '25-34 ans' "
        " WHEN TRUNC(MONTHS_BETWEEN(SYSDATE, DATENAISSANCE)/12) < 45 THEN '35-44 ans' "
        " ELSE '45+ ans' "
        " END AS tranche_age, "
        " COUNT(*) AS nb "
        "FROM EMPLOYES "
        "WHERE MONTHS_BETWEEN(SYSDATE,DATENAISSANCE) IS NOT NULL AND MONTHS_BETWEEN(SYSDATE,DATENAISSANCE) <> 0"
        "GROUP BY "
        " CASE "
        " WHEN TRUNC(MONTHS_BETWEEN(SYSDATE, DATENAISSANCE)/12) < 25 THEN '18-25 ans' "
        " WHEN TRUNC(MONTHS_BETWEEN(SYSDATE, DATENAISSANCE)/12) < 35 THEN '25-34 ans' "
        " WHEN TRUNC(MONTHS_BETWEEN(SYSDATE, DATENAISSANCE)/12) < 45 THEN '35-44 ans' "
        " ELSE '45+ ans' "
        " END"
        );
    while (query.next()) {
        QString label = query.value(0).toString();
        double value = query.value(1).toDouble();
        if (value > 0.0) {
            series->append(label, value);
        }
    }
    if (series->slices().isEmpty()) {
        QPieSlice* fallback = series->append("Aucune donnée",1.0);
        fallback->setColor(QColor(0x8a8f7a));
        fallback->setLabelVisible(true);
    }
    QList<QColor> colors = {
        QColor(0xb8860b),
        QColor(0xd4a437),
        QColor(0x8db596),
        QColor(0x6f8f6f),
        QColor(0x9aa38a)
    };
    int i = 0;
    for (QList<QPieSlice*>::ConstIterator it = series->slices().cbegin(); it != series->slices().cend() && i < colors.size(); ++it,++i) {
        QPieSlice* slice = *it;
        if (!slice) {
            continue;
        }
        slice->setColor(colors.at(i));
        slice->setBorderColor(textColor);
        slice->setBorderWidth(1.0);
        slice->setLabel(QString("%1 (%2)").arg(slice->label()).arg(static_cast<int>(slice->value())));
        slice->setLabelColor(textColor);
        slice->setLabelVisible(true);
    }
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des employés par âge");
    chart->setTitleBrush(QBrush(textColor));
    chart->setTitleFont(QFont("Ubuntu", 12, QFont::DemiBold));
    chart->setBackgroundVisible(true);
    chart->setBackgroundBrush(QBrush(backgroundColor));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(plotColor));
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setMargins(QMargins(16, 12, 16, 20));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setLabelColor(textColor);
    chart->legend()->setFont(QFont("Ubuntu", 9, QFont::DemiBold));
    return chart;
}
/**
 * @brief Générer les statistiques des employés (Répartition des employés par la catégorie)
 * @return QChart* 
 */
QChart *Employes::genererStatistiquesRepartition()
{
    const QColor backgroundColor(0x414833);
    const QColor plotColor(0x4a533f);
    const QColor textColor(0xf5f2e8);

    QPieSeries* series = new QPieSeries();
    series->setHoleSize(0.38);
    QSqlQuery query(R"(
     SELECT ROLE,
     (COUNT(*)/ SUM(COUNT(*)) OVER ())*100
     FROM EMPLOYES
     GROUP BY ROLE
    )"
    );
    while (query.next()) {
        QString label = query.value(0).toString();
        int value = query.value(1).toInt();
        if (value > 0) {
            series->append(label, value);
        }
    }
    if (series->slices().isEmpty()) {
        QPieSlice* fallback = series->append("Aucune donnée",1.0);
        fallback->setColor(QColor(0x8a8f7a));
        fallback->setLabelVisible(true);
    }
    QList<QColor> colors = {
        QColor(0xb8860b),
        QColor(0xd4a437),
        QColor(0x8db596),
        QColor(0x6f8f6f),
        QColor(0x9aa38a)
    };
    int i = 0;
    for (QList<QPieSlice*>::ConstIterator it = series->slices().cbegin(); it != series->slices().cend() && i < colors.size(); ++it,++i) {
        QPieSlice* slice = *it;
        if (!slice) {
            continue;
        }
        slice->setColor(colors.at(i));
        slice->setBorderColor(textColor);
        slice->setBorderWidth(1.0);
        slice->setLabel(QString("%1 (%2)").arg(slice->label()).arg(static_cast<int>(slice->value())));
        slice->setLabelColor(textColor);
        slice->setLabelVisible(true);
    }
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des employés par rôle");
    chart->setTitleBrush(QBrush(textColor));
    chart->setTitleFont(QFont("Ubuntu", 12, QFont::DemiBold));
    chart->setBackgroundVisible(true);
    chart->setBackgroundBrush(QBrush(backgroundColor));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(plotColor));
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setMargins(QMargins(16, 12, 16, 20));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setLabelColor(textColor);
    chart->legend()->setFont(QFont("Ubuntu", 9, QFont::DemiBold));
    return chart;
}

