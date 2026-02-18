#include "connection.h"

Connection &Connection::createInstance()
{
    static Connection instance;
    return instance;
}

bool Connection::createConnection()
{
    db.setDatabaseName("XE"); //remplacer avec votre nom du projet
    db.setUserName("amine"); //remplacer avec votre nom d'utilisateur
    //db.setHostName("192.168.1.111"); //remplacer avec localhost ou 127.0.0.1
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
    db = QSqlDatabase::addDatabase("QOCI");
}

Connection::~Connection()
{
    if (db.isOpen()){
        db.close();
    }
}
