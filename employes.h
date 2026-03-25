#ifndef EMPLOYES_H
#define EMPLOYES_H
#include <QChart>
#include <QBarSeries>
#include <QPieSeries>
#include <QPieSlice>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QGraphicsScene>
#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QDate>
#include <QTableView>
#include <opencv2/opencv.hpp>
#include <QFile>
#include <qtcsv/writer.h>
#include <qtcsv/stringdata.h>
#include <qtcsv/reader.h>
#include <QCryptographicHash>
#include <QFile>
#include <QGraphicsScene>
#include <QSqlError>
#include <QFont>
#include <QPen>
#include <QFileDialog>
#include <QDir>
using namespace QtCSV;
using namespace cv;
using namespace std;
using namespace cv::dnn;
struct FaceTemplate {
    int id;
    QString name;
    Mat vector;
};
class Employes
{
public:
    Employes();
    Employes(QString nom,
             QString prenom,
             QString tel="0",
             float heures=0,
             QDate date_recrutement=QDate::currentDate()
             , QDate date_naissance=QDate::currentDate()
             , QString role="",
             QString mdp="",
             QString mdp_hash="",
             int id_supervised=0,
             int id=-1);
    QString getNom(){
        return this->nom;
    }
    void setNom(QString nom){
        this->nom = nom;
    }
    QString getPrenom(){
        return this->prenom;
    }
    void setPrenom(QString prenom){
        this->prenom = prenom;
    }
    QString getTel(){
        return this->tel;
    }
    void setTel(QString tel){
        this->tel = tel;
    }
    float getHeures(){
        return this->heures;
    }
    void setHeures(float heures){
        this->heures = heures;
    }
    QDate getDate_recrutement(){
        return this->date_recrutement;
    }
    void setDate_recrutement(QDate date_recrutement){
        this->date_recrutement = date_recrutement;
    }
    QDate getDate_naissance(){
        return this->date_naissance;
    }
    void setDate_naissance(QDate date_naissance){
        this->date_naissance = date_naissance;
    }
    QString getRole(){
        return this->role;
    }

    void setRole(QString role){
        this->role = role;
    }

    QString getMdp(){
        return mdp;
    }
    void setMdp(QString mdp){
        this->mdp = mdp;
    }

    QString getHash(){
        return mdp_hash;
    }
    void setHash(QString hash){
        this->mdp_hash = hash;
    }
    int getId(){
        return id;
    }
    void setId(int id) {
        this->id = id;
    }
    int getIdSupervised(){
        return this->id_supervised;
    }
    void setIdSupervised(int id_supervised) {
        this->id_supervised = id_supervised;
    }

    bool ajouter();
    QSqlQueryModel* afficher();
    QSqlQueryModel* rechercher(QString nom);
    QSqlQueryModel* trier(QString choice);
    bool modifier(int id);
    bool supprimer(int id);
    bool ajoutCompte();
    bool existanceCompte();
    bool ajoutReconaissanceFaciale(QByteArray data);
    bool exportToCSV(QTableView* view, QString filePath);
    bool importCSV(QTableView* view);
    bool saveSessionToken(const QString& token,const QDate& expiry,int userId);
    bool validateSessionToken(const QString& token,int id);
    bool clearSessionToken(int id);
    QChart* genererStatistiquesHeures();
    QChart* genererStatistiquesNaissances();
    QChart* genererStatistiquesRepartition();

private:
    QString nom,prenom,role,mdp,mdp_hash,tel;
    int id,id_supervised;
    float heures;
    QDate date_recrutement,date_naissance;
};

#endif // EMPLOYES_H