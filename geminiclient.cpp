#include "geminiclient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QRegularExpression>

// Clé API : variable d'environnement GEMINI_API_KEY (recommandé) ou remplir ci-dessous (ne pas commiter une vraie clé)
static const char *kGeminiApiKeyEmbedded = "AIzaSyAYJiXquODrbQpgLj1CKEexvG6t6Ye8nUg";

// Alias « latest » (souvent meilleur quota / dispo que les ids versionnés sur le plan gratuit). Voir https://ai.google.dev/api/rest/v1beta/models
static const char *kGeminiModel = "gemini-flash-latest";

QString GeminiClient::apiKeyFromEnvironment()
{
    QString key;
    const QByteArray env = qgetenv("GEMINI_API_KEY");
    if (!env.isEmpty())
        key = QString::fromUtf8(env).trimmed();
    else if (kGeminiApiKeyEmbedded && kGeminiApiKeyEmbedded[0] != '\0')
        key = QString::fromUtf8(kGeminiApiKeyEmbedded).trimmed();
    else
        return QString();
    // Faute fréquente : « l » minuscule au lieu de « I » majuscule dans « AIza »
    if (key.startsWith(QStringLiteral("Alza")))
        key = QStringLiteral("AIza") + key.mid(4);
    return key;
}

GeminiClient::GeminiClient(QObject *parent)
    : QObject(parent)
    , m_nam(this)
{
}

GeminiClient::~GeminiClient()
{
    cancel();
}

void GeminiClient::cancel()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

QString GeminiClient::buildPrompt(const QVariantMap &d) const
{
    QJsonObject o;
    for (auto it = d.constBegin(); it != d.constEnd(); ++it)
        o.insert(it.key(), QJsonValue::fromVariant(it.value()));
    const QString json = QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));

    return QStringLiteral(
        "Tu es un expert en maintenance industrielle et gestion des risques d'équipements.\n"
        "Analyse la machine décrite par les données JSON suivantes (état de santé, dates, fréquence d'utilisation, incidents).\n"
        "Règles :\n"
        "- Calcule un score préventif entier entre 0 et 100 (100 = situation optimale, 0 = situation critique).\n"
        "- Détermine l'indice de risque EXACTEMENT parmi ces quatre valeurs : Faible, Moyen, Élevé, Critique.\n"
        "- Le score et le niveau de risque doivent être cohérents (ex. score bas => risque Élevé ou Critique).\n"
        "- Rédige un commentaire système professionnel en français, MAXIMUM 2 phrases courtes, sans liste ni rapport détaillé.\n"
        "Réponds UNIQUEMENT avec un objet JSON valide, sans markdown, sans texte avant ou après, avec exactement ces clés :\n"
        "{\"score_preventif\": <entier>, \"indice_risque\": \"<Faible|Moyen|Élevé|Critique>\", \"commentaire\": \"<texte>\"}\n\n"
        "Données machine :\n%1"
    ).arg(json);
}

static QString extractGoogleApiErrorMessage(const QJsonObject &root)
{
    const QJsonObject err = root.value(QStringLiteral("error")).toObject();
    return err.value(QStringLiteral("message")).toString();
}

// Dernier bloc {...} (utile si le modèle ajoute du texte avant/après sans mode structuré)
static QString extractOutermostJsonObject(const QString &text)
{
    const int end = text.lastIndexOf(QLatin1Char('}'));
    if (end < 0)
        return QString();
    int depth = 0;
    for (int i = end; i >= 0; --i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('}'))
            ++depth;
        else if (c == QLatin1Char('{')) {
            --depth;
            if (depth == 0)
                return text.mid(i, end - i + 1);
        }
    }
    return QString();
}

static QJsonObject machineAnalysisResponseJsonSchema()
{
    QJsonObject scoreProp;
    scoreProp.insert(QStringLiteral("type"), QStringLiteral("integer"));
    scoreProp.insert(QStringLiteral("description"), QStringLiteral("Score préventif de 0 à 100 (100 = optimal)."));

    QJsonObject riskProp;
    riskProp.insert(QStringLiteral("type"), QStringLiteral("string"));
    riskProp.insert(QStringLiteral("description"),
                     QStringLiteral("Une des valeurs : Faible, Moyen, Élevé, Critique."));

    QJsonObject commentProp;
    commentProp.insert(QStringLiteral("type"), QStringLiteral("string"));
    commentProp.insert(QStringLiteral("description"),
                       QStringLiteral("Commentaire professionnel en français, maximum 2 phrases courtes."));

    QJsonObject properties;
    properties.insert(QStringLiteral("score_preventif"), scoreProp);
    properties.insert(QStringLiteral("indice_risque"), riskProp);
    properties.insert(QStringLiteral("commentaire"), commentProp);

    QJsonObject schema;
    schema.insert(QStringLiteral("type"), QStringLiteral("object"));
    schema.insert(QStringLiteral("properties"), properties);
    schema.insert(QStringLiteral("required"),
                  QJsonArray{QStringLiteral("score_preventif"), QStringLiteral("indice_risque"),
                             QStringLiteral("commentaire")});
    return schema;
}

bool GeminiClient::parseGeminiResponse(const QByteArray &raw, int *outScore, QString *outRisk, QString *outComment, QString *outError) const
{
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
    if (!doc.isObject()) {
        if (outError) *outError = tr("Réponse API invalide (JSON racine).");
        return false;
    }
    QJsonObject root = doc.object();
    if (root.contains(QStringLiteral("error"))) {
        if (outError) {
            *outError = extractGoogleApiErrorMessage(root);
            if (outError->isEmpty())
                *outError = tr("Erreur renvoyée par l'API Google.");
        }
        return false;
    }
    QJsonArray candidates = root.value("candidates").toArray();
    if (candidates.isEmpty()) {
        if (outError) *outError = tr("Aucune réponse du modèle (candidates vides).");
        return false;
    }
    QJsonObject cand0 = candidates.at(0).toObject();
    QJsonObject content = cand0.value("content").toObject();
    QJsonArray parts = content.value("parts").toArray();
    if (parts.isEmpty()) {
        if (outError) *outError = tr("Réponse du modèle vide.");
        return false;
    }
    QString text = parts.at(0).toObject().value("text").toString().trimmed();
    text.remove(QRegularExpression("^```json\\s*"));
    text.remove(QRegularExpression("^```\\s*"));
    text.remove(QRegularExpression("\\s*```$"));
    text = text.trimmed();

    QByteArray utf8 = text.toUtf8();
    QJsonDocument inner = QJsonDocument::fromJson(utf8, &pe);
    if (!inner.isObject()) {
        const QString slice = extractOutermostJsonObject(text);
        if (!slice.isEmpty() && slice != text) {
            utf8 = slice.toUtf8();
            inner = QJsonDocument::fromJson(utf8, &pe);
        }
    }
    if (!inner.isObject()) {
        if (outError) *outError = tr("Le modèle n'a pas renvoyé un JSON exploitable : %1").arg(pe.errorString());
        return false;
    }
    QJsonObject obj = inner.object();
    int score = obj.value("score_preventif").toInt(-1);
    if (score < 0)
        score = qBound(0, static_cast<int>(obj.value("score_preventif").toDouble()), 100);
    else
        score = qBound(0, score, 100);

    QString risk = obj.value("indice_risque").toString().trimmed();
    QString comment = obj.value("commentaire").toString().trimmed();

    if (risk.isEmpty() || comment.isEmpty()) {
        if (outError) *outError = tr("Champs indice_risque ou commentaire manquants.");
        return false;
    }

    QString rLower = risk.toLower();
    if (rLower.contains("faible")) risk = QStringLiteral("Faible");
    else if (rLower.contains("moyen")) risk = QStringLiteral("Moyen");
    else if (rLower.contains("critique")) risk = QStringLiteral("Critique");
    else if (rLower.contains("élev") || rLower.contains("elev")) risk = QStringLiteral("Élevé");
    else risk = QStringLiteral("Moyen");

    *outScore = score;
    *outRisk = risk;
    *outComment = comment;
    return true;
}

void GeminiClient::analyzeMachine(const QVariantMap &machineData)
{
    cancel();

    const QString apiKey = apiKeyFromEnvironment();
    if (apiKey.isEmpty()) {
        emit analysisFailed(tr("Clé API Gemini absente. Définissez la variable d'environnement GEMINI_API_KEY "
                               "ou renseignez kGeminiApiKeyEmbedded dans geminiclient.cpp."));
        return;
    }
    if (!apiKey.startsWith(QStringLiteral("AIza"))) {
        emit analysisFailed(tr("La clé API semble incorrecte : les clés Google commencent généralement par « AIza » (vérifiez qu'il n'y a pas de faute de frappe, ex. « Alza »)."));
        return;
    }

    qDebug() << "[GEMINI] analyzeMachine called with machine data";

    QUrl url(QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent").arg(QLatin1String(kGeminiModel)));

    QJsonObject userPart;
    userPart.insert("text", buildPrompt(machineData));

    QJsonObject userMsg;
    userMsg.insert("role", QStringLiteral("user"));
    userMsg.insert("parts", QJsonArray{userPart});

    QJsonObject body;
    body.insert("contents", QJsonArray{userMsg});
    QJsonObject genCfg;
    genCfg.insert(QStringLiteral("temperature"), 0.25);
    genCfg.insert(QStringLiteral("maxOutputTokens"), 1024);
    genCfg.insert(QStringLiteral("responseMimeType"), QStringLiteral("application/json"));
    genCfg.insert(QStringLiteral("responseJsonSchema"), machineAnalysisResponseJsonSchema());
    body.insert(QStringLiteral("generationConfig"), genCfg);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader(QByteArrayLiteral("X-goog-api-key"), apiKey.toUtf8());

    qDebug() << "[GEMINI] Sending request to Gemini API...";
    m_reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &GeminiClient::onReplyFinished);
}

void GeminiClient::onReplyFinished()
{
    if (!m_reply)
        return;
    m_reply->deleteLater();
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;

    const QByteArray raw = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError netErr = reply->error();

    qDebug() << "[GEMINI] onReplyFinished called - httpStatus:" << httpStatus << "netErr:" << netErr << "raw length:" << raw.length();

    int score = 0;
    QString risk, comment, err;

    // Toujours tenter d'analyser le corps si présent (ignore une erreur Qt fantôme quand HTTP = OK)
    if (!raw.isEmpty()) {
        if (parseGeminiResponse(raw, &score, &risk, &comment, &err)) {
            qDebug() << "[GEMINI] Successfully parsed response - score:" << score << "risk:" << risk << "comment:" << comment;
            qDebug() << "[GEMINI] Emitting analysisComplete signal";
            emit analysisComplete(score, risk, comment);
            return;
        }
    }

    if (!err.isEmpty()) {
        qDebug() << "[GEMINI] Parse error:" << err;
        if (netErr != QNetworkReply::NoError)
            emit analysisFailed(tr("%1\n(%2)").arg(err, reply->errorString()));
        else
            emit analysisFailed(err);
        return;
    }

    if (httpStatus > 0 && httpStatus != 200) {
        qDebug() << "[GEMINI] HTTP error:" << httpStatus;
        emit analysisFailed(tr("Erreur HTTP %1").arg(httpStatus));
        return;
    }

    if (raw.isEmpty() && netErr != QNetworkReply::NoError)
        emit analysisFailed(tr("Réponse vide du serveur.\n%1").arg(reply->errorString()));
    else if (raw.isEmpty())
        emit analysisFailed(tr("Réponse vide du serveur."));
    else
        emit analysisFailed(tr("Erreur réseau : %1").arg(reply->errorString()));
}
