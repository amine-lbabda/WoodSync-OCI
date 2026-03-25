#ifndef EMAILDIALOG_H
#define EMAILDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;

class EmailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EmailDialog(const QString &reportText, QWidget *parent = nullptr);

    QString recipientEmail() const;

private slots:
    void onSendClicked();

private:
    bool validateAndAccept();

    QLineEdit *m_editTo;
    QString m_reportText;
};

#endif // EMAILDIALOG_H
