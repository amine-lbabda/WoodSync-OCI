#include "smtpclient.h"

#include <QAbstractSocket>
#include <QRegularExpression>

SmtpClient::SmtpClient(QObject *parent)
    : QObject(parent)
    , m_port(587)
    , m_socket(this)
{
}

void SmtpClient::setHost(const QString &host)
{
    m_host = host;
}

void SmtpClient::setPort(quint16 port)
{
    m_port = port;
}

void SmtpClient::setCredentials(const QString &user, const QString &password)
{
    m_user = user;
    m_password = password;
}

QByteArray SmtpClient::base64Encode(const QString &s)
{
    return s.toUtf8().toBase64();
}

bool SmtpClient::sendCommand(const QByteArray &cmd)
{
    if (!m_socket.isWritable()) return false;
    m_socket.write(cmd);
    if (!cmd.endsWith("\r\n"))
        m_socket.write("\r\n");
    return m_socket.flush();
}

bool SmtpClient::waitForResponse(int expectedCode, int timeoutMs, QString *lastLine)
{
    QString buffer;
    while (m_socket.state() == QAbstractSocket::ConnectedState ||
           m_socket.state() == QAbstractSocket::ConnectingState) {
        if (!m_socket.waitForReadyRead(timeoutMs))
            return false;
        buffer += QString::fromUtf8(m_socket.readAll());
        QStringList lines = buffer.split(QRegularExpression("\r?\n"), Qt::SkipEmptyParts);
        if (lines.isEmpty()) continue;
        QString last = lines.last();
        if (last.length() < 4) continue;
        bool ok = false;
        int code = last.left(3).toInt(&ok);
        if (!ok) continue;
        bool isLast = (last.length() == 3 || last[3] != '-');
        if (code == expectedCode && isLast) {
            if (lastLine) *lastLine = last.trimmed();
            return true;
        }
        if (code >= 400 && isLast) {
            if (lastLine) *lastLine = last.trimmed();
            return false;
        }
    }
    return false;
}

bool SmtpClient::sendMail(const QString &from, const QString &to,
                          const QString &subject, const QString &body,
                          QString *outError, bool bodyIsHtml)
{
    const int timeout = 15000;
    m_socket.abort();
    m_socket.connectToHost(m_host, m_port, QIODevice::ReadWrite);
    if (!m_socket.waitForConnected(timeout)) {
        if (outError) *outError = m_socket.errorString();
        return false;
    }

    if (!waitForResponse(220, timeout, nullptr)) {
        if (outError) *outError = tr("Pas de réponse 220 du serveur.");
        return false;
    }

    if (!sendCommand("EHLO localhost")) {
        if (outError) *outError = tr("Échec envoi EHLO.");
        return false;
    }
    if (!waitForResponse(250, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur n'a pas accepté EHLO.");
        return false;
    }

    if (!sendCommand("STARTTLS")) {
        if (outError) *outError = tr("Échec envoi STARTTLS.");
        return false;
    }
    if (!waitForResponse(220, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur n'a pas accepté STARTTLS.");
        return false;
    }

    m_socket.startClientEncryption();
    if (!m_socket.waitForEncrypted(timeout)) {
        if (outError) *outError = m_socket.errorString();
        return false;
    }

    if (!sendCommand("EHLO localhost")) {
        if (outError) *outError = tr("Échec EHLO après TLS.");
        return false;
    }
    if (!waitForResponse(250, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur n'a pas accepté EHLO après TLS.");
        return false;
    }

    if (!sendCommand("AUTH LOGIN")) {
        if (outError) *outError = tr("Échec envoi AUTH LOGIN.");
        return false;
    }
    if (!waitForResponse(334, timeout, nullptr)) {
        if (outError) *outError = tr("Authentification non supportée ou refusée.");
        return false;
    }

    if (!sendCommand(base64Encode(m_user))) {
        if (outError) *outError = tr("Échec envoi identifiant.");
        return false;
    }
    if (!waitForResponse(334, timeout, nullptr)) {
        if (outError) *outError = tr("Identifiant refusé.");
        return false;
    }

    if (!sendCommand(base64Encode(m_password))) {
        if (outError) *outError = tr("Échec envoi mot de passe.");
        return false;
    }
    QString authLine;
    if (!waitForResponse(235, timeout, &authLine)) {
        if (outError) *outError = authLine.isEmpty() ? tr("Mot de passe d'application refusé. Utilisez un mot de passe d'application Gmail.") : authLine;
        return false;
    }

    if (!sendCommand("MAIL FROM:<" + from.toUtf8() + ">")) {
        if (outError) *outError = tr("Échec MAIL FROM.");
        return false;
    }
    if (!waitForResponse(250, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur a refusé l'expéditeur.");
        return false;
    }

    if (!sendCommand("RCPT TO:<" + to.toUtf8() + ">")) {
        if (outError) *outError = tr("Échec RCPT TO.");
        return false;
    }
    if (!waitForResponse(250, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur a refusé le destinataire.");
        return false;
    }

    if (!sendCommand("DATA")) {
        if (outError) *outError = tr("Échec DATA.");
        return false;
    }
    if (!waitForResponse(354, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur n'accepte pas les données.");
        return false;
    }

    QByteArray msg;
    msg.append("From: " + from.toUtf8() + "\r\n");
    msg.append("To: " + to.toUtf8() + "\r\n");
    msg.append("Subject: " + subject.toUtf8() + "\r\n");
    if (bodyIsHtml)
        msg.append("Content-Type: text/html; charset=utf-8\r\n");
    else
        msg.append("Content-Type: text/plain; charset=utf-8\r\n");
    msg.append("\r\n");
    msg.append(body.toUtf8().replace("\n", "\r\n"));
    msg.append("\r\n.\r\n");

    m_socket.write(msg);
    if (!m_socket.flush()) {
        if (outError) *outError = tr("Échec envoi du corps du message.");
        return false;
    }
    if (!waitForResponse(250, timeout, nullptr)) {
        if (outError) *outError = tr("Serveur n'a pas accepté le message.");
        return false;
    }

    sendCommand("QUIT");
    m_socket.disconnectFromHost();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.waitForDisconnected(3000);
    return true;
}
