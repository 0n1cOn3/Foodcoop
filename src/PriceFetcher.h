#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QUrl>
#include <QRegularExpression>

struct PriceEntry {
    QString store;
    QString item;
    QDate date;
    double price;
};

class PriceFetcher : public QObject
{
    Q_OBJECT
public:
    explicit PriceFetcher(QObject *parent = nullptr);
    void fetchDailyPrices();
    QStringList storeList() const;
    QStringList categoryList() const;

signals:
    void priceFetched(const PriceEntry &entry);
    void fetchFinished();
    void fetchStarted();

private slots:
    void onReply(QNetworkReply *reply);

private:
    struct StoreProduct {
        QString store;
        QString item;
        QString url;
        QRegularExpression priceRegex;
    };
    QList<StoreProduct> m_products;
    int m_pending = 0;
    QNetworkAccessManager m_manager;
};
