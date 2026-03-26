/**
 * @file material.cpp
 * @author Ayoub Gharbi
 * @brief 
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "material.h"

static QString escSql(QString s)
{
    return s.replace(QLatin1Char('\''), QStringLiteral("''")).trimmed();
}

static QStringList parseCsvLine(const QString &line)
{
    QStringList fields;
    QString cur;
    bool inQuotes = false;
    for (int i = 0; i < line.length(); ++i) {
        const QChar c = line.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.length() && line.at(i + 1) == QLatin1Char('"')) {
                    cur += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur += c;
            }
        } else {
            if (c == QLatin1Char('"')) {
                inQuotes = true;
            } else if (c == QLatin1Char(',')) {
                fields << cur;
                cur.clear();
            } else {
                cur += c;
            }
        }
    }
    fields << cur;
    return fields;
}

static bool riskTextLooksValid(const QString &t)
{
    const QString r = t.trimmed().toLower();
    if (r.isEmpty())
        return true;
    bool ok = false;
    const int n = r.toInt(&ok);
    if (ok && n >= 1 && n <= 4)
        return true;
    return r.contains(QStringLiteral("faible")) || r.contains(QStringLiteral("moyen"))
        || r.contains(QStringLiteral("critique")) || r.contains(QStringLiteral("élev"))
        || r.contains(QStringLiteral("elev"));
}

static QVariant riskColumnToBindValue(const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty() || t == QLatin1Char('-') || t == QStringLiteral("—"))
        return QVariant();
    bool ok = false;
    const int n = t.toInt(&ok);
    if (ok && n >= 1 && n <= 4)
        return n;

    const QString r = t.toLower();
    if (r.contains(QStringLiteral("faible")))
        return 1;
    if (r.contains(QStringLiteral("moyen")))
        return 2;
    if (r.contains(QStringLiteral("critique")))
        return 4;
    if (r.contains(QStringLiteral("élev")) || r.contains(QStringLiteral("elev")))
        return 3;
    return 2;
}

static QList<QColor> statVividPalette()
{
    return {QColor(QStringLiteral("#5B7F66")), QColor(QStringLiteral("#C4956A")),
            QColor(QStringLiteral("#7D9B8E")), QColor(QStringLiteral("#8B7355")),
            QColor(QStringLiteral("#D4A574")), QColor(QStringLiteral("#6B7B8C"))};
}

static QBrush makeVerticalBarGradient(const QColor &top, const QColor &bottom)
{
    QLinearGradient g(0, 0, 0, 1);
    g.setCoordinateMode(QGradient::ObjectBoundingMode);
    g.setColorAt(0, top);
    g.setColorAt(1, bottom);
    return QBrush(g);
}

static void polishStatChartAxes(QChart *chart)
{
    if (!chart)
        return;
    const QPen gridPen(QColor(0xa6, 0x8a, 0x64, 75), 1, Qt::DashLine);
    const QFont axisFont(QStringLiteral("Roboto Condensed"), 10);
    for (QAbstractAxis *axis : chart->axes()) {
        axis->setTitleFont(axisFont);
        axis->setLabelsFont(axisFont);
        axis->setLabelsBrush(QBrush(QColor(0x4a, 0x3d, 0x32)));
        axis->setTitleBrush(QBrush(QColor(0x5c, 0x4a, 0x3a)));
        if (auto *va = qobject_cast<QValueAxis *>(axis)) {
            va->setGridLinePen(gridPen);
            va->setMinorGridLineVisible(false);
        }
    }
}

static void applyStatChartChrome(QChart *chart)
{
    if (!chart)
        return;

    QLinearGradient bg(0, 0, 1, 1);
    bg.setCoordinateMode(QGradient::ObjectBoundingMode);
    bg.setColorAt(0, QColor(0xff, 0xff, 0xfc));
    bg.setColorAt(1, QColor(0xf7, 0xf1, 0xe8));
    chart->setBackgroundBrush(QBrush(bg));
    chart->setBackgroundRoundness(14);

    QFont titleFont(QStringLiteral("Roboto Condensed"));
    titleFont.setPixelSize(16);
    titleFont.setWeight(QFont::DemiBold);
    chart->setTitleFont(titleFont);
    chart->setTitleBrush(QBrush(QColor(0x3d, 0x32, 0x28)));

    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(QColor(0xff, 0xff, 0xff, 0x45)));
    chart->setPlotAreaBackgroundPen(QPen(QColor(0xa6, 0x8a, 0x64, 0x5a), 1));

    if (QLegend *leg = chart->legend()) {
        leg->setAlignment(Qt::AlignBottom);
        leg->setBackgroundVisible(true);
        leg->setBrush(QBrush(QColor(0xff, 0xff, 0xff, 0x88)));
        leg->setPen(QPen(QColor(0x7f, 0x55, 0x39, 0x45)));
        QFont lf(QStringLiteral("Roboto Condensed"), 10);
        leg->setFont(lf);
        leg->setLabelColor(QColor(0x3d, 0x35, 0x30));
    }

    polishStatChartAxes(chart);
}

Material::Material()
    : m_idMateriel(-1)
    , m_frequenceUtilisation(0.0)
    , m_nombreIncidents(0)
    , m_scorePreventif(-1)
    , m_indiceRisquePanne(-1)
    , m_quantite(0)
{
}

Material::Material(const QVariantMap &map)
    : Material()
{
    m_idMateriel = map.value(QStringLiteral("id_materiel"), -1).toInt();
    m_nomMateriel = map.value(QStringLiteral("nom_materiel"), QString()).toString();
    m_atelier = map.value(QStringLiteral("atelier"), QString()).toString();
    m_etatSante = map.value(QStringLiteral("etat_sante"), QString()).toString();
    
    const QVariant dateAchatVar = map.value(QStringLiteral("date_achat"));
    if (dateAchatVar.typeId() == QMetaType::QDate) {
        m_dateAchat = dateAchatVar.toDate();
    } else {
        m_dateAchat = QDate::fromString(dateAchatVar.toString(), Qt::ISODate);
    }
    
    const QVariant dateEntretienVar = map.value(QStringLiteral("date_dernier_entretien"));
    if (dateEntretienVar.typeId() == QMetaType::QDate) {
        m_dateDernierEntretien = dateEntretienVar.toDate();
    } else {
        m_dateDernierEntretien = QDate::fromString(dateEntretienVar.toString(), Qt::ISODate);
    }
    
    m_frequenceUtilisation = map.value(QStringLiteral("frequence_utilisation"), 0.0).toDouble();
    m_nombreIncidents = map.value(QStringLiteral("nombre_incidents"), 0).toInt();
    m_scorePreventif = map.value(QStringLiteral("score_preventif"), -1).toInt();
    m_indiceRisquePanne = map.value(QStringLiteral("indice_risque"), -1).toInt();
    m_description = map.value(QStringLiteral("description"), QString()).toString();
    m_quantite = map.value(QStringLiteral("quantite"), 0).toInt();
    m_commentaire = map.value(QStringLiteral("commentaire"), QString()).toString();
}

QVariantMap Material::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("id_materiel"), m_idMateriel);
    map.insert(QStringLiteral("nom_materiel"), m_nomMateriel);
    map.insert(QStringLiteral("atelier"), m_atelier);
    map.insert(QStringLiteral("etat_sante"), m_etatSante);
    map.insert(QStringLiteral("date_achat"), m_dateAchat.isValid() ? m_dateAchat.toString(Qt::ISODate) : QString());
    map.insert(QStringLiteral("date_dernier_entretien"), m_dateDernierEntretien.isValid() ? m_dateDernierEntretien.toString(Qt::ISODate) : QString());
    map.insert(QStringLiteral("frequence_utilisation"), m_frequenceUtilisation);
    map.insert(QStringLiteral("nombre_incidents"), m_nombreIncidents);
    map.insert(QStringLiteral("score_preventif"), m_scorePreventif >= 0 ? QVariant(m_scorePreventif) : QVariant());
    map.insert(QStringLiteral("indice_risque"), m_indiceRisquePanne >= 0 ? QVariant(m_indiceRisquePanne) : QVariant());
    map.insert(QStringLiteral("description"), m_description);
    map.insert(QStringLiteral("quantite"), m_quantite);
    map.insert(QStringLiteral("commentaire"), m_commentaire);
    return map;
}

bool Material::isValid() const
{
    // Minimum validation: name, workshop, and health status are mandatory
    return !m_nomMateriel.isEmpty() && !m_atelier.isEmpty() && !m_etatSante.isEmpty();
}

bool Material::insert(QString *outError) const
{
    if (!isValid()) {
        if (outError)
            *outError = QStringLiteral("Champs obligatoires manquants.");
        return false;
    }
    if (!m_dateAchat.isValid() || !m_dateDernierEntretien.isValid()) {
        if (outError)
            *outError = QStringLiteral("Dates invalides.");
        return false;
    }

    const QString sql = QStringLiteral(
        "INSERT INTO MATERIELS (NOMMATERIEL, DESCRIPTION, ATELIER, QUANTITE, ETATSANTE, "
        "DATEDERNIERENTRETIEN, DATEACHAT, FREQUENCEUTILISATION, NOMBREINCIDENTS) "
        "VALUES ('%1', '%2', '%3', %4, '%5', DATE '%6', DATE '%7', %8, %9)")
        .arg(escSql(m_nomMateriel))
        .arg(escSql(m_description))
        .arg(escSql(m_atelier))
        .arg(QString::number(m_quantite))
        .arg(escSql(m_etatSante))
        .arg(m_dateDernierEntretien.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(m_dateAchat.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(QString::number(m_frequenceUtilisation, 'f', 4))
        .arg(QString::number(m_nombreIncidents));

    QSqlQuery q;
    if (!q.exec(sql)) {
        if (outError)
            *outError = q.lastError().text();
        return false;
    }
    return true;
}

bool Material::update(QString *outError) const
{
    if (m_idMateriel <= 0) {
        if (outError)
            *outError = QStringLiteral("ID matériel invalide.");
        return false;
    }
    if (!isValid()) {
        if (outError)
            *outError = QStringLiteral("Champs obligatoires manquants.");
        return false;
    }
    if (!m_dateAchat.isValid() || !m_dateDernierEntretien.isValid()) {
        if (outError)
            *outError = QStringLiteral("Dates invalides.");
        return false;
    }

    const QString sql = QStringLiteral(
        "UPDATE MATERIELS SET NOMMATERIEL='%1', DESCRIPTION='%2', ATELIER='%3', QUANTITE=%4, ETATSANTE='%5', "
        "DATEDERNIERENTRETIEN=DATE '%6', DATEACHAT=DATE '%7', FREQUENCEUTILISATION=%8, NOMBREINCIDENTS=%9 WHERE IDMATERIEL=%10")
        .arg(escSql(m_nomMateriel))
        .arg(escSql(m_description))
        .arg(escSql(m_atelier))
        .arg(QString::number(m_quantite))
        .arg(escSql(m_etatSante))
        .arg(m_dateDernierEntretien.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(m_dateAchat.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(QString::number(m_frequenceUtilisation, 'f', 4))
        .arg(QString::number(m_nombreIncidents))
        .arg(QString::number(m_idMateriel));

    QSqlQuery q;
    if (!q.exec(sql)) {
        if (outError)
            *outError = q.lastError().text();
        return false;
    }
    return true;
}

bool Material::removeById(int id, QString *outError)
{
    QSqlQuery q;
    q.prepare(QStringLiteral("DELETE FROM MATERIELS WHERE IDMATERIEL=?"));
    q.addBindValue(id);
    if (!q.exec()) {
        if (outError)
            *outError = q.lastError().text();
        return false;
    }
    return true;
}

bool Material::fetchById(int id, Material *outMaterial, QString *outError)
{
    if (!outMaterial) {
        if (outError)
            *outError = QStringLiteral("Pointeur de sortie invalide.");
        return false;
    }

    QSqlQuery q;
    q.prepare(QStringLiteral(
        "SELECT IDMATERIEL, NOMMATERIEL, DESCRIPTION, ATELIER, QUANTITE, ETATSANTE, "
        "DATEDERNIERENTRETIEN, DATEACHAT, FREQUENCEUTILISATION, NOMBREINCIDENTS, "
        "SCOREPREVENTIF, INDICERISQUEPANNE, COMMENTAIRESSYSTEME FROM MATERIELS WHERE IDMATERIEL=?"));
    q.addBindValue(id);
    if (!q.exec()) {
        if (outError)
            *outError = q.lastError().text();
        return false;
    }
    if (!q.next()) {
        if (outError)
            *outError = QStringLiteral("Matériel introuvable.");
        return false;
    }

    QVariantMap map;
    map.insert(QStringLiteral("id_materiel"), q.value(0));
    map.insert(QStringLiteral("nom_materiel"), q.value(1));
    map.insert(QStringLiteral("description"), q.value(2));
    map.insert(QStringLiteral("atelier"), q.value(3));
    map.insert(QStringLiteral("quantite"), q.value(4));
    map.insert(QStringLiteral("etat_sante"), q.value(5));
    map.insert(QStringLiteral("date_dernier_entretien"), q.value(6));
    map.insert(QStringLiteral("date_achat"), q.value(7));
    map.insert(QStringLiteral("frequence_utilisation"), q.value(8));
    map.insert(QStringLiteral("nombre_incidents"), q.value(9));
    map.insert(QStringLiteral("score_preventif"), q.value(10));
    map.insert(QStringLiteral("indice_risque"), q.value(11));
    map.insert(QStringLiteral("commentaire"), q.value(12));

    *outMaterial = Material(map);
    return true;
}

QList<Material> Material::fetchAll(const QString &searchName,
                                   const QString &workshopFilter,
                                   const QString &statusFilter,
                                   QString *outError)
{
    QList<Material> materials;
    QString sql = QStringLiteral(
        "SELECT IDMATERIEL, NOMMATERIEL, DESCRIPTION, ATELIER, QUANTITE, ETATSANTE, "
        "DATEDERNIERENTRETIEN, DATEACHAT, FREQUENCEUTILISATION, NOMBREINCIDENTS, "
        "SCOREPREVENTIF, INDICERISQUEPANNE, COMMENTAIRESSYSTEME FROM MATERIELS WHERE 1=1");

    QList<QVariant> bindValues;
    if (!searchName.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND UPPER(NOMMATERIEL) LIKE ?");
        bindValues << (QStringLiteral("%") + searchName.trimmed().toUpper() + QStringLiteral("%"));
    }
    if (!workshopFilter.trimmed().isEmpty() && !workshopFilter.startsWith(QStringLiteral("Tous"), Qt::CaseInsensitive)) {
        sql += QStringLiteral(" AND TRIM(ATELIER) = TRIM(?)");
        bindValues << workshopFilter.trimmed();
    }
    if (!statusFilter.trimmed().isEmpty() && !statusFilter.startsWith(QStringLiteral("Tous"), Qt::CaseInsensitive)) {
        sql += QStringLiteral(" AND TRIM(ETATSANTE) = TRIM(?)");
        bindValues << statusFilter.trimmed();
    }
    sql += QStringLiteral(" ORDER BY ATELIER ASC, ETATSANTE ASC, IDMATERIEL ASC");

    QSqlQuery q;
    q.setForwardOnly(true);
    q.prepare(sql);
    for (const QVariant &v : bindValues)
        q.addBindValue(v);
    if (!q.exec()) {
        if (outError)
            *outError = q.lastError().text();
        return materials;
    }

    while (q.next()) {
        QVariantMap map;
        map.insert(QStringLiteral("id_materiel"), q.value(0));
        map.insert(QStringLiteral("nom_materiel"), q.value(1));
        map.insert(QStringLiteral("description"), q.value(2));
        map.insert(QStringLiteral("atelier"), q.value(3));
        map.insert(QStringLiteral("quantite"), q.value(4));
        map.insert(QStringLiteral("etat_sante"), q.value(5));
        map.insert(QStringLiteral("date_dernier_entretien"), q.value(6));
        map.insert(QStringLiteral("date_achat"), q.value(7));
        map.insert(QStringLiteral("frequence_utilisation"), q.value(8));
        map.insert(QStringLiteral("nombre_incidents"), q.value(9));
        map.insert(QStringLiteral("score_preventif"), q.value(10));
        map.insert(QStringLiteral("indice_risque"), q.value(11));
        map.insert(QStringLiteral("commentaire"), q.value(12));
        materials.append(Material(map));
    }

    return materials;
}

QList<Material> Material::fetchLatest(int limit, QString *outError)
{
    QList<Material> materials;
    if (limit <= 0)
        return materials;

    QSqlQuery q;
    q.setForwardOnly(true);
    if (!q.exec(QStringLiteral("SELECT IDMATERIEL, NOMMATERIEL, DESCRIPTION, ATELIER, QUANTITE, ETATSANTE, "
                               "DATEDERNIERENTRETIEN, DATEACHAT, FREQUENCEUTILISATION, NOMBREINCIDENTS, "
                               "SCOREPREVENTIF, INDICERISQUEPANNE, COMMENTAIRESSYSTEME "
                               "FROM MATERIELS ORDER BY IDMATERIEL DESC"))) {
        if (outError)
            *outError = q.lastError().text();
        return materials;
    }

    int count = 0;
    while (q.next() && count < limit) {
        QVariantMap map;
        map.insert(QStringLiteral("id_materiel"), q.value(0));
        map.insert(QStringLiteral("nom_materiel"), q.value(1));
        map.insert(QStringLiteral("description"), q.value(2));
        map.insert(QStringLiteral("atelier"), q.value(3));
        map.insert(QStringLiteral("quantite"), q.value(4));
        map.insert(QStringLiteral("etat_sante"), q.value(5));
        map.insert(QStringLiteral("date_dernier_entretien"), q.value(6));
        map.insert(QStringLiteral("date_achat"), q.value(7));
        map.insert(QStringLiteral("frequence_utilisation"), q.value(8));
        map.insert(QStringLiteral("nombre_incidents"), q.value(9));
        map.insert(QStringLiteral("score_preventif"), q.value(10));
        map.insert(QStringLiteral("indice_risque"), q.value(11));
        map.insert(QStringLiteral("commentaire"), q.value(12));
        materials.append(Material(map));
        ++count;
    }
    return materials;
}

QList<Material> Material::fetchAllById(QString *outError)
{
    QList<Material> materials;
    QSqlQuery q;
    q.setForwardOnly(true);
    if (!q.exec(QStringLiteral(
            "SELECT IDMATERIEL, NOMMATERIEL, DESCRIPTION, ATELIER, QUANTITE, ETATSANTE, "
            "DATEDERNIERENTRETIEN, DATEACHAT, FREQUENCEUTILISATION, NOMBREINCIDENTS, "
            "SCOREPREVENTIF, INDICERISQUEPANNE, COMMENTAIRESSYSTEME "
            "FROM MATERIELS ORDER BY IDMATERIEL ASC"))) {
        if (outError)
            *outError = q.lastError().text();
        return materials;
    }

    while (q.next()) {
        QVariantMap map;
        map.insert(QStringLiteral("id_materiel"), q.value(0));
        map.insert(QStringLiteral("nom_materiel"), q.value(1));
        map.insert(QStringLiteral("description"), q.value(2));
        map.insert(QStringLiteral("atelier"), q.value(3));
        map.insert(QStringLiteral("quantite"), q.value(4));
        map.insert(QStringLiteral("etat_sante"), q.value(5));
        map.insert(QStringLiteral("date_dernier_entretien"), q.value(6));
        map.insert(QStringLiteral("date_achat"), q.value(7));
        map.insert(QStringLiteral("frequence_utilisation"), q.value(8));
        map.insert(QStringLiteral("nombre_incidents"), q.value(9));
        map.insert(QStringLiteral("score_preventif"), q.value(10));
        map.insert(QStringLiteral("indice_risque"), q.value(11));
        map.insert(QStringLiteral("commentaire"), q.value(12));
        materials.append(Material(map));
    }

    return materials;
}

bool Material::updateAiIndicators(int idMateriel,
                                  int scorePreventif,
                                  int indiceRisqueCode,
                                  const QString &commentaire,
                                  QString *outError)
{
    QSqlQuery qu;
    qu.prepare(QStringLiteral("UPDATE MATERIELS SET SCOREPREVENTIF = ?, INDICERISQUEPANNE = ?, COMMENTAIRESSYSTEME = ? WHERE IDMATERIEL = ?"));
    qu.addBindValue(scorePreventif);
    if (indiceRisqueCode > 0)
        qu.addBindValue(indiceRisqueCode);
    else
        qu.addBindValue(QVariant());
    qu.addBindValue(commentaire);
    qu.addBindValue(idMateriel);
    if (!qu.exec()) {
        if (outError)
            *outError = qu.lastError().text();
        return false;
    }
    return true;
}

bool Material::importFromCsvFile(const QString &filePath, QString *outError, int *outInserted)
{
    if (outInserted)
        *outInserted = 0;

    const QStringList expectedHeaders = {
        QObject::tr("Nom"),
        QObject::tr("Description"),
        QObject::tr("Atelier"),
        QObject::tr("Quantité"),
        QObject::tr("État santé"),
        QObject::tr("Date dernier entretien"),
        QObject::tr("Date achat"),
        QObject::tr("Fréq. utilisation"),
        QObject::tr("Nb incidents"),
        QObject::tr("Score préventif"),
        QObject::tr("Indice risque"),
        QObject::tr("Commentaires")
    };

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (outError)
            *outError = QObject::tr("Impossible d'ouvrir le fichier.");
        return false;
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QStringList lines;
    while (!in.atEnd())
        lines << in.readLine();
    file.close();

    if (lines.isEmpty()) {
        if (outError)
            *outError = QObject::tr("Le fichier est vide.");
        return false;
    }
    if (!lines[0].isEmpty() && lines[0].at(0) == QChar(0xFEFF))
        lines[0].remove(0, 1);

    QStringList headers = parseCsvLine(lines[0]);
    for (QString &h : headers)
        h = h.trimmed();

    if (headers.size() != expectedHeaders.size()) {
        if (outError)
            *outError = QObject::tr("Fichier non supportable : le tableau doit comporter %1 colonnes (export CSV de l'application).")
                            .arg(expectedHeaders.size());
        return false;
    }
    for (int i = 0; i < expectedHeaders.size(); ++i) {
        if (QString::compare(headers.at(i), expectedHeaders.at(i), Qt::CaseInsensitive) != 0) {
            if (outError)
                *outError = QObject::tr("Fichier non supportable : les en-têtes ne correspondent pas à l'export des matériels.");
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        if (outError)
            *outError = QObject::tr("Impossible de démarrer la transaction.");
        return false;
    }

    const QLocale cLoc(QLocale::C);
    int inserted = 0;
    for (int li = 1; li < lines.size(); ++li) {
        const QString raw = lines.at(li);
        if (raw.trimmed().isEmpty())
            continue;

        const QStringList f = parseCsvLine(raw);
        if (f.size() != expectedHeaders.size()) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — nombre de colonnes incorrect.").arg(li + 1);
            return false;
        }

        const QString nom = f[0].trimmed();
        const QString description = f[1].trimmed();
        const QString atelier = f[2].trimmed();
        bool okQty = false;
        const int quantite = f[3].trimmed().toInt(&okQty);
        const QString etatSante = f[4].trimmed();
        QDate dateEntretien = QDate::fromString(f[5].trimmed(), Qt::ISODate);
        const QDate dateAchat = QDate::fromString(f[6].trimmed(), Qt::ISODate);
        bool okFreq = false;
        const double freq = cLoc.toDouble(f[7].trimmed(), &okFreq);
        bool okInc = false;
        const int nbIncidents = f[8].trimmed().toInt(&okInc);

        if (nom.isEmpty() || atelier.isEmpty() || etatSante.isEmpty()) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — champs obligatoires manquants (nom, atelier, état).")
                                .arg(li + 1);
            return false;
        }
        if (!okQty || quantite < 0) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — quantité invalide.").arg(li + 1);
            return false;
        }
        if (!dateAchat.isValid()) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — date d'achat invalide (format AAAA-MM-JJ).")
                                .arg(li + 1);
            return false;
        }
        if (!dateEntretien.isValid())
            dateEntretien = dateAchat;
        if (!okFreq || freq < 0.0) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — fréquence d'utilisation invalide.").arg(li + 1);
            return false;
        }
        if (!okInc || nbIncidents < 0) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — nombre d'incidents invalide.").arg(li + 1);
            return false;
        }

        const QString scoreRaw = f[9].trimmed();
        QVariant scoreVar;
        if (scoreRaw.isEmpty() || scoreRaw == QLatin1Char('-') || scoreRaw == QStringLiteral("—")) {
            scoreVar = QVariant();
        } else {
            bool okSc = false;
            const int sc = scoreRaw.toInt(&okSc);
            if (!okSc || sc < 0 || sc > 100) {
                db.rollback();
                if (outError)
                    *outError = QObject::tr("Fichier non supportable : ligne %1 — score préventif invalide (0-100).").arg(li + 1);
                return false;
            }
            scoreVar = sc;
        }

        if (!riskTextLooksValid(f[10])) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Fichier non supportable : ligne %1 — indice de risque non reconnu.").arg(li + 1);
            return false;
        }
        const QVariant riskVar = riskColumnToBindValue(f[10]);
        const QString comment = f[11].trimmed();

        const QString scoreSql = (!scoreVar.isValid() || scoreVar.isNull())
            ? QStringLiteral("NULL")
            : QString::number(scoreVar.toInt());
        const QString riskSql = (!riskVar.isValid() || riskVar.isNull())
            ? QStringLiteral("NULL")
            : QString::number(riskVar.toInt());
        const QString commentSql = comment.isEmpty()
            ? QStringLiteral("NULL")
            : QStringLiteral("'%1'").arg(escSql(comment));

        const QString insertSql =
            QStringLiteral(
                "INSERT INTO MATERIELS (NOMMATERIEL, DESCRIPTION, ATELIER, QUANTITE, ETATSANTE, "
                "DATEDERNIERENTRETIEN, DATEACHAT, FREQUENCEUTILISATION, NOMBREINCIDENTS, "
                "SCOREPREVENTIF, INDICERISQUEPANNE, COMMENTAIRESSYSTEME) "
                "VALUES ('%1', '%2', '%3', %4, '%5', DATE '%6', DATE '%7', %8, %9, %10, %11, %12)")
                .arg(escSql(nom))
                .arg(escSql(description))
                .arg(escSql(atelier))
                .arg(QString::number(quantite))
                .arg(escSql(etatSante))
                .arg(dateEntretien.toString(QStringLiteral("yyyy-MM-dd")))
                .arg(dateAchat.toString(QStringLiteral("yyyy-MM-dd")))
                .arg(QString::number(freq, 'f', 4))
                .arg(QString::number(nbIncidents))
                .arg(scoreSql)
                .arg(riskSql)
                .arg(commentSql);

        QSqlQuery qu;
        if (!qu.exec(insertSql)) {
            db.rollback();
            if (outError)
                *outError = QObject::tr("Échec d'insertion (ligne %1) : %2").arg(li + 1).arg(qu.lastError().text());
            return false;
        }
        ++inserted;
    }

    if (!db.commit()) {
        db.rollback();
        if (outError)
            *outError = QObject::tr("Échec de validation de l'import.");
        return false;
    }

    if (outInserted)
        *outInserted = inserted;
    return true;
}

QChart *Material::createMachinesByYearChart(QString *outError)
{
    QSqlQuery q;
    if (!q.exec(QStringLiteral(
            "SELECT EXTRACT(YEAR FROM DATEACHAT) AS ANNEE, COUNT(*) "
            "FROM MATERIELS "
            "GROUP BY EXTRACT(YEAR FROM DATEACHAT) "
            "ORDER BY ANNEE"))) {
        if (outError)
            *outError = q.lastError().text();
        return nullptr;
    }

    QStringList categories;
    QBarSet *set = new QBarSet(QStringLiteral("Machines"));
    while (q.next()) {
        categories << q.value(0).toString();
        *set << q.value(1).toInt();
    }

    const QList<QColor> colors = statVividPalette();
    const QBrush barBrush = makeVerticalBarGradient(colors.at(0).lighter(115), colors.at(0).darker(108));
    set->setBrush(barBrush);

    QBarSeries *series = new QBarSeries();
    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QObject::tr("Nombre de machines par année"));
    chart->legend()->setVisible(false);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat(QStringLiteral("%d"));
    axisY->setMin(0);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setAnimationDuration(1100);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);
    applyStatChartChrome(chart);
    return chart;
}

QChart *Material::createMachinesByWorkshopChart(QString *outError)
{
    QSqlQuery q;
    if (!q.exec(QStringLiteral("SELECT ATELIER, COUNT(*) FROM MATERIELS GROUP BY ATELIER ORDER BY ATELIER"))) {
        if (outError)
            *outError = q.lastError().text();
        return nullptr;
    }

    QPieSeries *series = new QPieSeries();
    const QList<QColor> colors = statVividPalette();
    int i = 0;
    while (q.next()) {
        QPieSlice *slice = series->append(q.value(0).toString(), q.value(1).toInt());
        slice->setColor(colors.at(i % colors.size()));
        slice->setLabelVisible(true);
        slice->setLabelColor(QColor(QStringLiteral("#3d3530")));
        slice->setBorderColor(QColor(255, 255, 255, 210));
        slice->setBorderWidth(2);
        slice->setExplodeDistanceFactor(0.12);
        QObject::connect(slice, &QPieSlice::hovered, slice, [slice](bool state) {
            slice->setExploded(state);
        });
        ++i;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QObject::tr("Machines par atelier"));
    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setAnimationDuration(1100);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);
    applyStatChartChrome(chart);
    return chart;
}

QChart *Material::createUsageAndCountByWorkshopChart(QString *outError)
{
    QSqlQuery q;
    if (!q.exec(QStringLiteral(
            "SELECT ATELIER, COUNT(*) AS NB_MACHINES, AVG(FREQUENCEUTILISATION) AS MOY_FREQ "
            "FROM MATERIELS "
            "GROUP BY ATELIER "
            "ORDER BY ATELIER"))) {
        if (outError)
            *outError = q.lastError().text();
        return nullptr;
    }

    QStringList categories;
    QBarSet *setNb = new QBarSet(QObject::tr("Nombre de machines"));
    QBarSet *setFreq = new QBarSet(QObject::tr("Fréquence moyenne"));
    while (q.next()) {
        QString at = q.value(0).toString().trimmed();
        if (at.isEmpty())
            at = QObject::tr("(Non renseigné)");
        categories << at;
        *setNb << static_cast<qreal>(q.value(1).toLongLong());
        *setFreq << q.value(2).toDouble();
    }

    const QList<QColor> colors = statVividPalette();
    const QBrush brushNb = makeVerticalBarGradient(colors.at(2).lighter(118), colors.at(2).darker(105));
    const QBrush brushFreq = makeVerticalBarGradient(colors.at(1).lighter(112), colors.at(1).darker(104));
    setNb->setBrush(brushNb);
    setFreq->setBrush(brushFreq);

    QBarSeries *seriesNb = new QBarSeries();
    seriesNb->append(setNb);
    QBarSeries *seriesFreq = new QBarSeries();
    seriesFreq->append(setFreq);

    QChart *chart = new QChart();
    chart->addSeries(seriesNb);
    chart->addSeries(seriesFreq);
    chart->setTitle(QObject::tr("Fréquence d'utilisation et nombre de machines par atelier"));
    chart->legend()->setVisible(true);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    seriesNb->attachAxis(axisX);
    seriesFreq->attachAxis(axisX);

    QValueAxis *axisNb = new QValueAxis();
    axisNb->setTitleText(QObject::tr("Nombre"));
    axisNb->setLabelFormat(QStringLiteral("%d"));
    axisNb->setMin(0);
    chart->addAxis(axisNb, Qt::AlignLeft);
    seriesNb->attachAxis(axisNb);

    QValueAxis *axisFreq = new QValueAxis();
    axisFreq->setTitleText(QObject::tr("Fréquence moy."));
    axisFreq->setLabelFormat(QStringLiteral("%.2f"));
    axisFreq->setMin(0);
    chart->addAxis(axisFreq, Qt::AlignRight);
    seriesFreq->attachAxis(axisFreq);

    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setAnimationDuration(1100);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);
    applyStatChartChrome(chart);
    return chart;
}

QChart *Material::createHealthStatusChart(QString *outError)
{
    QSqlQuery q;
    if (!q.exec(QStringLiteral("SELECT ETATSANTE, COUNT(*) FROM MATERIELS GROUP BY ETATSANTE ORDER BY ETATSANTE"))) {
        if (outError)
            *outError = q.lastError().text();
        return nullptr;
    }

    QPieSeries *series = new QPieSeries();
    series->setHoleSize(0.42);
    const QList<QColor> colors = statVividPalette();
    int i = 0;
    while (q.next()) {
        QPieSlice *slice = series->append(q.value(0).toString(), q.value(1).toInt());
        slice->setColor(colors.at(i % colors.size()));
        slice->setLabelVisible(true);
        slice->setLabelColor(QColor(QStringLiteral("#3d3530")));
        slice->setBorderColor(QColor(255, 255, 255, 210));
        slice->setBorderWidth(2);
        slice->setExplodeDistanceFactor(0.12);
        QObject::connect(slice, &QPieSlice::hovered, slice, [slice](bool state) {
            slice->setExploded(state);
        });
        ++i;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QObject::tr("Machines par état de santé"));
    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setAnimationDuration(1100);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);
    applyStatChartChrome(chart);
    return chart;
}
