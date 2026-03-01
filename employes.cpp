#include "employes.h"
#include "qcryptographichash.h"
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
}

Employes::Employes(QString nom, QString prenom, int tel, float heures, QDate date_recrutement, QDate date_naissance, QString role, QString mdp, QString mdp_hash)
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

}

bool Employes::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO EMPLOYES (NOM,PRENOM,TEL,HEURETRAVAILLE,DATERECRUTEMENT,DATENAISSANCE,ROLE) VALUES(?,?,?,?,?,?,?)");
    query.addBindValue(nom);
    query.addBindValue(prenom);
    query.addBindValue(tel);
    query.addBindValue(heures);
    query.addBindValue(date_recrutement);
    query.addBindValue(date_naissance);
    query.addBindValue(role);
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
    query.prepare("UPDATE EMPLOYÉS SET NOM=?,PRENOM=?,TEL=?,HEURETRAVAILLE=?,DATERECRUTEMENT=?,DATENAISSANCE=?,ROLE=? WHERE IDEMPLOYE=?");
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
    query.prepare("DELETE FROM EMPLOYÉS WHERE IDEMPLOYE=?");
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
    QSqlQuery query;
    query.prepare("INSERT INTO EMPLOYES (NOM,PRENOM,MDP,MDP_SALT,ROLE) VALUES(?,?,?,?,?)");
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
    query.prepare("SELECT MDP,MDP_SALT FROM EMPLOYES WHERE NOM=? AND PRENOM=?");
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
            return verifyHex == storedHash;
        }
    }
    return false;
}

