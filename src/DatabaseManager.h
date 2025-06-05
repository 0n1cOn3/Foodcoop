#pragma once
#include <QObject>
#include <QSqlDatabase>
#include "PriceFetcher.h"
#include <QStringList>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    bool open(const QString &path);
    QString databasePath() const;
    void insertPrice(const PriceEntry &entry);
    void insertIssue(const IssueEntry &issue);
    QList<IssueEntry> loadIssues() const;
    QList<PriceEntry> loadPrices(const QString &item,
                                 const QString &store = QString(),
                                 const QDate &fromDate = QDate()) const;
    PriceEntry latestPrice(const QString &item, const QString &store) const;
    bool hasPrices() const;
    void ensureProduct(const QString &store, const QString &item);
    void setProductUrl(const QString &store, const QString &item, const QString &url);
    QString productUrl(const QString &store, const QString &item) const;
    QStringList loadItems() const;

private:
    QSqlDatabase m_db;
    QString m_dbPath;
};
