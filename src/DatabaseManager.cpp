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
    m_dbPath = path;
    if (!m_db.open()) {
        qWarning() << "Failed to open database" << m_db.lastError();
        return false;
    }
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS prices (store TEXT, item TEXT, date TEXT, price REAL)");
    return true;
}

QString DatabaseManager::databasePath() const
{
    return m_dbPath;
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

QList<PriceEntry> DatabaseManager::loadPrices(const QString &item, const QString &store, const QDate &fromDate) const
{
    QList<PriceEntry> list;
    QSqlQuery query;
    QString sql = "SELECT store, item, date, price FROM prices WHERE item = ?";
    if (!store.isEmpty())
        sql += " AND store = ?";
    if (fromDate.isValid())
        sql += " AND date >= ?";
    sql += " ORDER BY date";
    query.prepare(sql);
    query.addBindValue(item);
    if (!store.isEmpty())
        query.addBindValue(store);
    if (fromDate.isValid())
        query.addBindValue(fromDate.toString(Qt::ISODate));
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

PriceEntry DatabaseManager::latestPrice(const QString &item, const QString &store) const
{
    PriceEntry entry;
    QSqlQuery query;
    query.prepare("SELECT store, item, date, price FROM prices WHERE item = ? AND store = ? ORDER BY date DESC LIMIT 1");
    query.addBindValue(item);
    query.addBindValue(store);
    if (query.exec() && query.next()) {
        entry.store = query.value(0).toString();
        entry.item = query.value(1).toString();
        entry.date = QDate::fromString(query.value(2).toString(), Qt::ISODate);
        entry.price = query.value(3).toDouble();
    }
    return entry;
}
