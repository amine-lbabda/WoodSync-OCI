/**
 * @file smtpclient.h
 * @author Ayoub Gharbi
 * @brief Client to send emails
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QString>
#include <QSslSocket>
#include <QAbstractSocket>
#include <QRegularExpression>

class SmtpClient : public QObject
{
    Q_OBJECT
public:
    explicit SmtpClient(QObject *parent = nullptr);

    void setHost(const QString &host);
    void setPort(quint16 port);
    void setCredentials(const QString &user, const QString &password);

    bool sendMail(const QString &from, const QString &to, const QString &subject, const QString &body,
                  QString *outError = nullptr, bool bodyIsHtml = false);

private:
    bool waitForResponse(int code, int timeoutMs, QString *lastLine = nullptr);
    bool sendCommand(const QByteArray &cmd);
    QByteArray base64Encode(const QString &s);

    QString m_host;
    quint16 m_port = 587;
    QString m_user;
    QString m_password;
    QSslSocket m_socket;
};

#endif // SMTPCLIENT_H
