#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

bool DatabaseManager::open(const QString &path)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        qWarning() << "Failed to open database" << m_db.lastError();
        return false;
    }
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS prices (store TEXT, item TEXT, date TEXT, price REAL)");
    return true;
}

void DatabaseManager::insertPrice(const PriceEntry &entry)
{
    QSqlQuery query;
    query.prepare("INSERT INTO prices (store, item, date, price) VALUES (?, ?, ?, ?)");
    query.addBindValue(entry.store);
    query.addBindValue(entry.item);
    query.addBindValue(entry.date.toString(Qt::ISODate));
    query.addBindValue(entry.price);
    if (!query.exec()) {
        qWarning() << "Insert failed" << query.lastError();
    }
}

QList<PriceEntry> DatabaseManager::loadPrices(const QString &item) const
{
    QList<PriceEntry> list;
    QSqlQuery query;
    query.prepare("SELECT store, item, date, price FROM prices WHERE item = ? ORDER BY date");
    query.addBindValue(item);
    if (query.exec()) {
        while (query.next()) {
            PriceEntry entry;
            entry.store = query.value(0).toString();
            entry.item = query.value(1).toString();
            entry.date = QDate::fromString(query.value(2).toString(), Qt::ISODate);
            entry.price = query.value(3).toDouble();
            list.append(entry);
        }
    }
    return list;
}
