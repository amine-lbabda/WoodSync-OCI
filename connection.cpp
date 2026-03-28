/**
 * @file connection.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
// ...existing code...
#include "connection.h"
#include "dotenv.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
using namespace std;
/**
 * @brief Creating the instance of a singleton
 * 
 * @return Connection& 
 */
Connection &Connection::createInstance()
{
    static Connection instance;
    return instance;
}
/**
 * @brief Creating the connection to the Oracle Database
 * 
 * @return true 
 * @return false 
 */
static QString resolveEnvPath(const QString &fileName)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::cleanPath(appDir + "/" + fileName),
        QDir::cleanPath(appDir + "/../" + fileName),
        QDir::cleanPath(appDir + "/../../" + fileName),
        QDir::cleanPath(appDir + "/../../../" + fileName),
        QDir::cleanPath(QDir::currentPath() + "/" + fileName)
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return path;
    }
    return candidates.first();
}

bool Connection::createConnection()
{
    // Unified .env loading using relative path
    dotenv::init(resolveEnvPath(".env").toStdString().c_str());
    QString name = QString::fromUtf8(dotenv::getenv("DATABASE_NAME"));
    qDebug() << name;
    QString username = QString::fromUtf8(dotenv::getenv("DATABASE_USERNAME"));
    qDebug() << username;
    QString hostname = QString::fromUtf8(dotenv::getenv("DATABSE_HOST"));
    qDebug() << hostname;
    QString password = QString::fromUtf8(dotenv::getenv("DATABASE_PASSWORD"));
    qDebug() << password;
    db.setDatabaseName(name); //remplacer avec votre nom du projet
    db.setUserName(username); //remplacer avec votre nom d'utilisateur
    //db.setHostName(hostname); //remplacer avec localhost ou 127.0.0.1
    db.setPassword(password); //remplacer avec votre mdp
    if (db.open()){
        qDebug() << "Connection established !";
        return true;
    } else {
        qDebug() << "Error: " << db.lastError().text();
        return false;
    }
}
/**
 * @brief Construct a new Connection:: Connection object
 * 
 */
Connection::Connection() {
    db = QSqlDatabase::addDatabase("QODBC");
}
/**
 * @brief Destroy the Connection:: Connection object
 * 
 */
Connection::~Connection()
{
    if (db.isOpen()){
        db.close();
    }
}
