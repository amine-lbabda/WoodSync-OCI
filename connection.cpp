#include "connection.h"
#include "dotenv.h"
using namespace std;
Connection &Connection::createInstance()
{
    static Connection instance;
    return instance;
}

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

Connection::Connection() {
    db = QSqlDatabase::addDatabase("QOCI");
}

Connection::~Connection()
{
    if (db.isOpen()){
        db.close();
    }
}
