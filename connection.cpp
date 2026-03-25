#include "connection.h"

Connection &Connection::createInstance()
{
    static Connection instance;
    return instance;
}

bool Connection::createConnection()
{
    db.setDatabaseName("Projet_2A"); //remplacer avec votre nom du projet
    db.setUserName("amine"); //remplacer avec votre nom d'utilisateur
    db.setPassword("admin"); //remplacer avec votre mdp
    if (db.open()){
        qDebug() << "Connection established !";
        return true;
    } else {
        qDebug() << "Error: " << db.lastError().text();
        return false;
    }
}

Connection::Connection() {
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection::~Connection()
{
    if (db.isOpen()){
        db.close();
    }
}
