#ifndef GEMINICLIENT_H
#define GEMINICLIENT_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QByteArray>

class QNetworkReply;

class GeminiClient : public QObject
{
    Q_OBJECT
public:
    explicit GeminiClient(QObject *parent = nullptr);
    ~GeminiClient() override;

    void analyzeMachine(const QVariantMap &machineData);
    void cancel();

    static QString apiKeyFromEnvironment();

signals:
    void analysisComplete(int scorePreventif, const QString &indiceRisque, const QString &commentaire);
    void analysisFailed(const QString &errorMessage);

private slots:
    void onReplyFinished();

private:
    QString buildPrompt(const QVariantMap &machineData) const;
    bool parseGeminiResponse(const QByteArray &raw, int *outScore, QString *outRisk, QString *outComment,
                             QString *outError) const;

    QNetworkAccessManager m_nam;
    QNetworkReply *m_reply = nullptr;
};

#endif // GEMINICLIENT_H
