#pragma once
#include <QObject>
#include <QSqlDatabase>
#include "PriceFetcher.h"

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

private:
    QSqlDatabase m_db;
    QString m_dbPath;
};
