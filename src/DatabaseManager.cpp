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
    query.exec("CREATE TABLE IF NOT EXISTS prices (store TEXT, item TEXT, date TEXT, price REAL, currency TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS issues (store TEXT, item TEXT, date TEXT, error TEXT)");
    query.exec("CREATE TABLE IF NOT EXISTS products (store TEXT, item TEXT, url TEXT, PRIMARY KEY(store, item))");
    return true;
}

QString DatabaseManager::databasePath() const
{
    return m_dbPath;
}

void DatabaseManager::insertPrice(const PriceEntry &entry)
{
    QSqlQuery query;
    query.prepare("INSERT INTO prices (store, item, date, price, currency) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(entry.store);
    query.addBindValue(entry.item);
    query.addBindValue(entry.date.toString(Qt::ISODate));
    query.addBindValue(entry.price);
    query.addBindValue(entry.currency);
    if (!query.exec()) {
        qWarning() << "Insert failed" << query.lastError();
    }
}

void DatabaseManager::insertIssue(const IssueEntry &issue)
{
    QSqlQuery query;
    query.prepare("INSERT INTO issues (store, item, date, error) VALUES (?, ?, ?, ?)");
    query.addBindValue(issue.store);
    query.addBindValue(issue.item);
    query.addBindValue(issue.date.toString(Qt::ISODate));
    query.addBindValue(issue.error);
    if (!query.exec()) {
        qWarning() << "Insert issue failed" << query.lastError();
    }
}

QList<PriceEntry> DatabaseManager::loadPrices(const QString &item, const QString &store, const QDate &fromDate) const
{
    QList<PriceEntry> list;
    QSqlQuery query;
    QString sql = "SELECT store, item, date, price, currency FROM prices WHERE item = ?";
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
            entry.currency = query.value(4).toString();
            list.append(entry);
        }
    }
    return list;
}

QList<IssueEntry> DatabaseManager::loadIssues() const
{
    QList<IssueEntry> list;
    QSqlQuery query("SELECT store, item, date, error FROM issues ORDER BY date DESC");
    while (query.next()) {
        IssueEntry issue;
        issue.store = query.value(0).toString();
        issue.item = query.value(1).toString();
        issue.date = QDate::fromString(query.value(2).toString(), Qt::ISODate);
        issue.error = query.value(3).toString();
        list.append(issue);
    }
    return list;
}

PriceEntry DatabaseManager::latestPrice(const QString &item, const QString &store) const
{
    PriceEntry entry;
    QSqlQuery query;
    query.prepare("SELECT store, item, date, price, currency FROM prices WHERE item = ? AND store = ? ORDER BY date DESC LIMIT 1");
    query.addBindValue(item);
    query.addBindValue(store);
    if (query.exec() && query.next()) {
        entry.store = query.value(0).toString();
        entry.item = query.value(1).toString();
        entry.date = QDate::fromString(query.value(2).toString(), Qt::ISODate);
        entry.price = query.value(3).toDouble();
        entry.currency = query.value(4).toString();
    }
    return entry;
}

bool DatabaseManager::hasPrices() const
{
    QSqlQuery query("SELECT COUNT(*) FROM prices");
    if (query.next())
        return query.value(0).toInt() > 0;
    return false;
}

void DatabaseManager::ensureProduct(const QString &store, const QString &item)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO products (store, item, url) VALUES (?, ?, '')");
    query.addBindValue(store);
    query.addBindValue(item);
    if (!query.exec())
        qWarning() << "ensureProduct failed" << query.lastError();
}

void DatabaseManager::setProductUrl(const QString &store, const QString &item, const QString &url)
{
    QSqlQuery query;
    query.prepare("INSERT INTO products (store, item, url) VALUES (?, ?, ?) "
                  "ON CONFLICT(store, item) DO UPDATE SET url=excluded.url");
    query.addBindValue(store);
    query.addBindValue(item);
    query.addBindValue(url);
    if (!query.exec())
        qWarning() << "setProductUrl failed" << query.lastError();
}

QString DatabaseManager::productUrl(const QString &store, const QString &item) const
{
    QSqlQuery query;
    query.prepare("SELECT url FROM products WHERE store=? AND item=?");
    query.addBindValue(store);
    query.addBindValue(item);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return QString();
}

QStringList DatabaseManager::loadItems() const
{
    QStringList list;
    QSqlQuery query("SELECT DISTINCT item FROM products ORDER BY item");
    while (query.next())
        list.append(query.value(0).toString());
    return list;
}
