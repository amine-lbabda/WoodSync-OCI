#include "emaildialog.h"
#include "smtpclient.h"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>

// Expéditeur par défaut (enregistré dans le code)
static const QString DEFAULT_SENDER_EMAIL = QStringLiteral("ayoubgharbi2005@gmail.com");
// Mot de passe d'application Gmail (16 caractères, sans espaces)
static const QString DEFAULT_APP_PASSWORD = QStringLiteral("ikvgwboaisngzcow");

// Style rapport (aligné sur l'application WoodSync), sans logo
static const char *REPORT_HTML_STYLE =
    "body { margin: 0; padding: 20px; font-family: 'Segoe UI', 'Roboto Condensed', Arial, sans-serif; "
    "font-size: 14px; color: #414833; background-color: #fdfefe; } "
    ".report-box { background-color: rgb(253, 255, 237); border: 2px solid #7F5539; border-radius: 12px; "
    "padding: 20px; color: #003366; line-height: 1.6; } "
    ".report-box p { margin: 0 0 0.8em 0; } "
    ".report-box p:last-child { margin-bottom: 0; } "
    "h2 { color: #7F5539; font-size: 16px; margin: 0 0 12px 0; }";

static bool looksLikeEmail(const QString &s)
{
    return s.contains('@') && s.contains('.') && s.length() >= 5;
}

static QString htmlEscape(const QString &s)
{
    QString out;
    out.reserve(s.size());
    for (QChar c : s) {
        if (c == QLatin1Char('<')) out += QLatin1String("&lt;");
        else if (c == QLatin1Char('>')) out += QLatin1String("&gt;");
        else if (c == QLatin1Char('&')) out += QLatin1String("&amp;");
        else if (c == QLatin1Char('"')) out += QLatin1String("&quot;");
        else out += c;
    }
    return out;
}

static QString buildReportHtmlEmail(const QString &plainReport)
{
    QString bodyEscaped = htmlEscape(plainReport);
    bodyEscaped.replace(QLatin1String("\n"), QLatin1String("<br>\n"));

    return QStringLiteral(
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><style>%1</style></head><body>"
        "<div class=\"report-box\"><h2>Rapport machine - WoodSync</h2><p>%2</p></div>"
        "</body></html>"
    ).arg(QLatin1String(REPORT_HTML_STYLE), bodyEscaped);
}

EmailDialog::EmailDialog(const QString &reportText, QWidget *parent)
    : QDialog(parent)
    , m_reportText(reportText)
{
    setWindowTitle(tr("Envoyer le rapport par e-mail"));
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_editTo = new QLineEdit(this);
    m_editTo->setPlaceholderText(tr("exemple@domaine.com"));
    m_editTo->setClearButtonEnabled(true);

    QFormLayout *form = new QFormLayout();
    form->addRow(tr("Destinataire (e-mail) :"), m_editTo);
    mainLayout->addLayout(form);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    box->button(QDialogButtonBox::Ok)->setText(tr("Envoyer"));
    connect(box, &QDialogButtonBox::accepted, this, &EmailDialog::onSendClicked, Qt::UniqueConnection);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(box);
}

QString EmailDialog::recipientEmail() const
{
    return m_editTo->text().trimmed();
}

void EmailDialog::onSendClicked()
{
    if (!validateAndAccept())
        return;

    QString to = recipientEmail();
    QString from = DEFAULT_SENDER_EMAIL;
    QString pass = DEFAULT_APP_PASSWORD;

    SmtpClient smtp;
    smtp.setHost("smtp.gmail.com");
    smtp.setPort(587);
    smtp.setCredentials(from, pass);

    QString htmlBody = buildReportHtmlEmail(m_reportText);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    bool ok = smtp.sendMail(from, to,
                            tr("Rapport machine - WoodSync"),
                            htmlBody,
                            &err,
                            true);
    QApplication::restoreOverrideCursor();

    if (ok) {
        QMessageBox::information(this, tr("Envoi"), tr("Le rapport a été envoyé avec succès à %1.").arg(to));
        accept();
    } else {
        QMessageBox::warning(this, tr("Erreur d'envoi"), tr("Impossible d'envoyer l'e-mail : %1").arg(err));
    }
}

bool EmailDialog::validateAndAccept()
{
    QString to = m_editTo->text().trimmed();

    if (to.isEmpty()) {
        QMessageBox::warning(this, tr("Champ requis"), tr("Veuillez saisir l'adresse e-mail du destinataire."));
        m_editTo->setFocus();
        return false;
    }
    if (!looksLikeEmail(to)) {
        QMessageBox::warning(this, tr("E-mail invalide"), tr("L'adresse du destinataire ne semble pas valide."));
        m_editTo->setFocus();
        return false;
    }
    return true;
}
