#ifndef MATERIAL_H
#define MATERIAL_H

#include <QString>
#include <QDate>
#include <QList>
#include <QVariantMap>

class QChart;

class Material
{
public:
    Material();
    explicit Material(const QVariantMap &map);
    
    // Getters
    int idMateriel() const { return m_idMateriel; }
    QString nomMateriel() const { return m_nomMateriel; }
    QString atelier() const { return m_atelier; }
    QString etatSante() const { return m_etatSante; }
    QDate dateAchat() const { return m_dateAchat; }
    QDate dateDernierEntretien() const { return m_dateDernierEntretien; }
    double frequenceUtilisation() const { return m_frequenceUtilisation; }
    int nombreIncidents() const { return m_nombreIncidents; }
    int scorePreventif() const { return m_scorePreventif; }
    int indiceRisquePanne() const { return m_indiceRisquePanne; }
    QString description() const { return m_description; }
    int quantite() const { return m_quantite; }
    QString commentaire() const { return m_commentaire; }
    
    // Setters
    void setIdMateriel(int id) { m_idMateriel = id; }
    void setNomMateriel(const QString &nom) { m_nomMateriel = nom; }
    void setAtelier(const QString &atelier) { m_atelier = atelier; }
    void setEtatSante(const QString &etat) { m_etatSante = etat; }
    void setDateAchat(const QDate &date) { m_dateAchat = date; }
    void setDateDernierEntretien(const QDate &date) { m_dateDernierEntretien = date; }
    void setFrequenceUtilisation(double freq) { m_frequenceUtilisation = freq; }
    void setNombreIncidents(int incidents) { m_nombreIncidents = incidents; }
    void setScorePreventif(int score) { m_scorePreventif = score; }
    void setIndiceRisquePanne(int indice) { m_indiceRisquePanne = indice; }
    void setDescription(const QString &desc) { m_description = desc; }
    void setQuantite(int qty) { m_quantite = qty; }
    void setCommentaire(const QString &comment) { m_commentaire = comment; }
    
    // Utility methods
    QVariantMap toVariantMap() const;
    bool isValid() const;

    // Database CRUD and queries
    bool insert(QString *outError = nullptr) const;
    bool update(QString *outError = nullptr) const;
    static bool removeById(int id, QString *outError = nullptr);
    static bool fetchById(int id, Material *outMaterial, QString *outError = nullptr);
    static QList<Material> fetchAll(const QString &searchName,
                                    const QString &workshopFilter,
                                    const QString &statusFilter,
                                    QString *outError = nullptr);
    static QList<Material> fetchAllById(QString *outError = nullptr);
    static QList<Material> fetchLatest(int limit, QString *outError = nullptr);
    static bool updateAiIndicators(int idMateriel,
                                   int scorePreventif,
                                   int indiceRisqueCode,
                                   const QString &commentaire,
                                   QString *outError = nullptr);
    static bool importFromCsvFile(const QString &filePath,
                                  QString *outError = nullptr,
                                  int *outInserted = nullptr);

    // Statistics charts
    static QChart *createMachinesByYearChart(QString *outError = nullptr);
    static QChart *createMachinesByWorkshopChart(QString *outError = nullptr);
    static QChart *createUsageAndCountByWorkshopChart(QString *outError = nullptr);
    static QChart *createHealthStatusChart(QString *outError = nullptr);
    
private:
    int m_idMateriel = -1;
    QString m_nomMateriel;
    QString m_atelier;
    QString m_etatSante;
    QDate m_dateAchat;
    QDate m_dateDernierEntretien;
    double m_frequenceUtilisation = 0.0;
    int m_nombreIncidents = 0;
    int m_scorePreventif = -1;  // -1 means NULL/not set
    int m_indiceRisquePanne = -1;  // -1 means NULL/not set
    QString m_description;
    int m_quantite = 0;
    QString m_commentaire;
};

#endif // MATERIAL_H
