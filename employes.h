#ifndef EMPLOYES_H
#define EMPLOYES_H
#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QDate>
#include <QTableView>
using namespace std;
class Employes
{
public:
    Employes();
    Employes(QString nom,
             QString prenom,
             int tel=0,
             float heures=0,
             QDate date_recrutement=QDate::currentDate()
             ,QDate date_naissance=QDate::currentDate()
             ,QString role="",
             QString mdp="",
             QString mdp_hash="");
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
    int getTel(){
        return this->tel;
    }
    void setTel(int tel){
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

    bool ajouter();
    QSqlQueryModel* afficher();
    bool modifier(int id);
    bool supprimer(int id);
    bool ajoutCompte();
    bool existanceCompte();


private:
    QString nom,prenom,role,mdp,mdp_hash;
    int tel;
    float heures;
    QDate date_recrutement,date_naissance;
};

#endif // EMPLOYES_H
