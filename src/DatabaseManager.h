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
    void insertPrice(const PriceEntry &entry);
    QList<PriceEntry> loadPrices(const QString &item) const;

private:
    QSqlDatabase m_db;
};
