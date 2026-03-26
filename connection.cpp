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
#include "connection.h"
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
bool Connection::createConnection()
{
    dotenv::init("/home/amine/Desktop/WoodSync-OCI/.env");
    QString name = QString::fromUtf8(dotenv::getenv("DATABASE_NAME"));
    QString username = QString::fromUtf8(dotenv::getenv("DATABASE_USERNAME"));
    QString hostname = QString::fromUtf8(dotenv::getenv("DATABSE_HOST"));
    QString password = QString::fromUtf8(dotenv::getenv("DATABASE_PASSWORD"));
    db.setDatabaseName(name); //remplacer avec votre nom du projet
    db.setUserName(username); //remplacer avec votre nom d'utilisateur
    db.setHostName(hostname); //remplacer avec localhost ou 127.0.0.1
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
    db = QSqlDatabase::addDatabase("QOCI");
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