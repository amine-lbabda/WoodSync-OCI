/**
 * @file connection.h
 * @author Mohamed Amine Lbabda
 * @brief 
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef CONNECTION_H
#define CONNECTION_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include "dotenv.h"
class Connection
{
public:
    static Connection& createInstance();
    bool createConnection();
private:
    QSqlDatabase db;
    Connection();
    ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
};

#endif // CONNECTION_H
