/**
 * @file geminiclient.h
 * @author Ayoub Gharbi
 * @brief Client to communicate with OLLAMA API to generate scores locally
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
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

    static QString ollamaBaseUrlFromEnvironment();
    static QString ollamaModelFromEnvironment();

signals:
    void analysisComplete(int scorePreventif, const QString &indiceRisque, const QString &commentaire);
    void analysisFailed(const QString &errorMessage);

private slots:
    void onReplyFinished();

private:
    QString buildPrompt(const QVariantMap &machineData) const;
    bool parseOllamaResponse(const QByteArray &raw, int *outScore, QString *outRisk, QString *outComment,
                             QString *outError) const;

    QNetworkAccessManager m_nam;
    QNetworkReply *m_reply = nullptr;
};

#endif // GEMINICLIENT_H
