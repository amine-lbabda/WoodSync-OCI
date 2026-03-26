#include "geminiclient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>

static const char kOllamaBaseUrlDefault[] = "http://localhost:11434";
static const char kOllamaModelDefault[] = "llama3.1:8b";

QString GeminiClient::ollamaBaseUrlFromEnvironment()
{
    const QByteArray env = qgetenv("OLLAMA_URL");
    const QString configured = QString::fromUtf8(env).trimmed();
    return configured.isEmpty() ? QString::fromLatin1(kOllamaBaseUrlDefault) : configured;
}

QString GeminiClient::ollamaModelFromEnvironment()
{
    const QByteArray env = qgetenv("OLLAMA_MODEL");
    const QString configured = QString::fromUtf8(env).trimmed();
    return configured.isEmpty() ? QString::fromLatin1(kOllamaModelDefault) : configured;
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

static QString extractOllamaApiErrorMessage(const QJsonObject &root)
{
    const QJsonValue err = root.value(QStringLiteral("error"));
    if (err.isString())
        return err.toString();
    if (err.isObject())
        return err.toObject().value(QStringLiteral("message")).toString();
    return QString();
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

bool GeminiClient::parseOllamaResponse(const QByteArray &raw, int *outScore, QString *outRisk, QString *outComment, QString *outError) const
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
            *outError = extractOllamaApiErrorMessage(root);
            if (outError->isEmpty())
                *outError = tr("Erreur renvoyée par le serveur Ollama.");
        }
        return false;
    }

    QString text = root.value(QStringLiteral("response")).toString().trimmed();
    if (text.isEmpty()) {
        if (outError) *outError = tr("Réponse du modèle vide.");
        return false;
    }

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

    const QString baseUrl = ollamaBaseUrlFromEnvironment();
    const QString model = ollamaModelFromEnvironment();
    if (baseUrl.isEmpty()) {
        emit analysisFailed(tr("Configuration Ollama absente. Définissez OLLAMA_URL."));
        return;
    }
    if (model.isEmpty()) {
        emit analysisFailed(tr("Configuration Ollama absente. Définissez OLLAMA_MODEL."));
        return;
    }

    qDebug() << "[OLLAMA] analyzeMachine called with machine data";

    QString normalizedBase = baseUrl;
    if (normalizedBase.endsWith(QLatin1Char('/')))
        normalizedBase.chop(1);
    QUrl url(QStringLiteral("%1/api/generate").arg(normalizedBase));

    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    body.insert(QStringLiteral("prompt"), buildPrompt(machineData));
    body.insert(QStringLiteral("stream"), false);
    body.insert(QStringLiteral("format"), QStringLiteral("json"));

    QJsonObject options;
    options.insert(QStringLiteral("temperature"), 0.25);
    body.insert(QStringLiteral("options"), options);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    qDebug() << "[OLLAMA] Sending request to:" << url;
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

    qDebug() << "[OLLAMA] onReplyFinished called - httpStatus:" << httpStatus << "netErr:" << netErr << "raw length:" << raw.length();

    int score = 0;
    QString risk, comment, err;

    // Toujours tenter d'analyser le corps si présent (ignore une erreur Qt fantôme quand HTTP = OK)
    if (!raw.isEmpty()) {
        if (parseOllamaResponse(raw, &score, &risk, &comment, &err)) {
            qDebug() << "[OLLAMA] Successfully parsed response - score:" << score << "risk:" << risk << "comment:" << comment;
            qDebug() << "[OLLAMA] Emitting analysisComplete signal";
            emit analysisComplete(score, risk, comment);
            return;
        }
    }

    if (!err.isEmpty()) {
        qDebug() << "[OLLAMA] Parse error:" << err;
        if (netErr != QNetworkReply::NoError)
            emit analysisFailed(tr("%1\n(%2)").arg(err, reply->errorString()));
        else
            emit analysisFailed(err);
        return;
    }

    if (httpStatus > 0 && httpStatus != 200) {
        qDebug() << "[OLLAMA] HTTP error:" << httpStatus;
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
