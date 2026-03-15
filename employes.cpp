#include "employes.h"
#include "qcryptographichash.h"
#include "qfiledialog.h"
#include <QSqlError>
Employes::Employes() {
    this->nom = "";
    this->prenom = "";
    this->role = "";
    this->tel = 0;
    this->heures = 0.0;
    this->date_recrutement = QDate::currentDate();
    this->date_naissance = QDate::currentDate();
    this->mdp="";
    this->mdp_hash="";
    this->id_supervised=-1;
}

Employes::Employes(QString nom, QString prenom, int tel, float heures, QDate date_recrutement, QDate date_naissance, QString role, QString mdp, QString mdp_hash, int id_supervised)
{
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

bool Employes::ajouter()
{
    QSqlQuery query;
    if (id_supervised == 0) {
        query.prepare("INSERT INTO EMPLOYES (NOM,PRENOM,TEL,HEURETRAVAILLE,DATERECRUTEMENT,DATENAISSANCE,ROLE) VALUES(?,?,?,?,?,?,?)");
        query.addBindValue(nom);
        query.addBindValue(prenom);
        query.addBindValue(tel);
        query.addBindValue(heures);
        query.addBindValue(date_recrutement);
        query.addBindValue(date_naissance);
        query.addBindValue(role);
    } else {
            query.prepare("INSERT INTO EMPLOYES (NOM,PRENOM,TEL,HEURETRAVAILLE,DATERECRUTEMENT,DATENAISSANCE,ROLE,IDSUPERVISEUR) VALUES(?,?,?,?,?,?,?,?)");
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

QSqlQueryModel *Employes::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT IDEMPLOYE,NOM, PRENOM, TO_CHAR(TEL,'99999999999999'), HEURETRAVAILLE, TO_CHAR(DATERECRUTEMENT,'DD/MM/YYYY'), TO_CHAR(DATENAISSANCE,'DD/MM/YYYY'),ROLE FROM EMPLOYES");
    if (model->lastError().isValid()) {
        qDebug() << "Error: " << model->lastError().text();
    }
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("Nom"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("Prénom"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("Tel"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("Heures travaillés"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("Date recrutement"));
    model->setHeaderData(6,Qt::Horizontal,QObject::tr("Date naissance"));
    model->setHeaderData(7,Qt::Horizontal,QObject::tr("Rôle"));
    return model;
}

bool Employes::modifier(int id)
{
    QString res = QString::number(id);
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET NOM=?,PRENOM=?,TEL=?,HEURETRAVAILLE=?,DATERECRUTEMENT=?,DATENAISSANCE=?,ROLE=? WHERE IDEMPLOYE=?");
    query.addBindValue(nom);
    query.addBindValue(prenom);
    query.addBindValue(tel);
    query.addBindValue(heures);
    query.addBindValue(date_recrutement);
    query.addBindValue(date_naissance);
    query.addBindValue(role);
    query.addBindValue(res);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        return true;
    }
}

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

bool Employes::ajoutCompte()
{
    QSqlQuery query,seqQuery;
    seqQuery.exec("SELECT EMPLOYES_SEQ.NEXTVAL FROM DUAL");
    if (!seqQuery.next()) return false;
    id = seqQuery.value(0).toInt();
    query.prepare("INSERT INTO EMPLOYES (IDEMPLOYE,NOM,PRENOM,MDP,MDP_SALT,ROLE) VALUES(?,?,?,?,?,?)");
    query.addBindValue(id);
    query.addBindValue(nom);
    query.addBindValue(prenom);
    query.addBindValue(mdp);
    query.addBindValue(mdp_hash);
    query.addBindValue("Menuisier");
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee added successfully!";
        return true;
    }
}

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

bool Employes::importCSV(QTableView *view)
{
    QString filePath = QFileDialog::getOpenFileName(nullptr,"Import employee data",QDir::homePath(),"CSV Files (*.csv)");
    if (filePath.isEmpty()) return false;
    QList<QStringList> readData = Reader::readToList(filePath);
    if (readData.isEmpty()) {
        return false;
    }
    int importedCount = 0;
    for (int i= 1;i< readData.size();++i) {
        QStringList row = readData.at(i);
        if (row.size() >= 8) {
            QString nom = row.at(1).trimmed();
            QString prenom = row.at(2).trimmed();
            int tel = row.at(3).isEmpty() ? 0: row.at(3).toInt();
            float heures = row.at(4).isEmpty() ? 0: row.at(4).toFloat();
            QDate dateRecrutement = QDate::fromString(row.at(5).trimmed(),"dd/MM/yyyy");
            QDate dateNaissance = QDate::fromString(row.at(6).trimmed(),"dd/MM/yyyy");
            QString role = row.at(7).trimmed();
            setNom(nom);
            setPrenom(prenom);
            setTel(tel);
            setHeures(heures);
            setDate_recrutement(dateRecrutement);
            setDate_naissance(dateNaissance);
            setRole(role);
            if (ajouter()) {
                importedCount++;
            }
        }
    }
    if (importedCount > 0) {
        view->setModel(afficher());
        return true;
    }
    return false;
}

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
        qDebug() << "Employee added successfully!";
        return query.next();
    }
}

bool Employes::clearSessionToken(int id)
{
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET SESSION_TOKEN=NULL,TOKEN_EXPIRY=NULL WHERE IDEMPLOYE=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "Oracle Error:" << query.lastError().text();
        return false;
    } else {
        qDebug() << "Employee added successfully!";
        return true;
    }
}

