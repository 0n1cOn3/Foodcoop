#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QList>
#include <QUrl>
#include <QRegularExpression>
#include <QDate>
#include <QWebEnginePage>
#include <QWebEngineHttpRequest>
#include <QWebEngineCertificateError>
#include <QQueue>

struct PriceEntry {
    QString store;
    QString item;
    QDate date;
    double price;
    QString currency;
};

struct IssueEntry {
    QString store;
    QString item;
    QDate date;
    QString error;
};

class DatabaseManager;

class PriceFetcher : public QObject
{
    Q_OBJECT
public:
    explicit PriceFetcher(DatabaseManager *db, QObject *parent = nullptr);
    void fetchDailyPrices();
    QStringList storeList() const;
    QStringList categoryList() const;
    void addItem(const QString &item);

signals:
    void priceFetched(const PriceEntry &entry);
    void issueOccurred(const IssueEntry &issue);
    void fetchFinished();
    void fetchStarted();
    void progressChanged(int done, int total);
    void itemListChanged();

private slots:
    void onReply(QNetworkReply *reply);
    void onBrowserLoadFinished(bool ok);
    void onBrowserHtmlReady(const QString &html);

private:
    enum class RequestStage { Search, Product };

    struct StoreInfo {
        QString store;
        QString searchUrl;          // use %1 for item
        QRegularExpression productRegex; // extracts product URL from search page
        QRegularExpression priceRegex;   // extracts price from product page
    };

    QList<StoreInfo> m_stores;
    QStringList m_items;
    int m_pending = 0;
    int m_total = 0;
    QNetworkAccessManager m_manager;
    DatabaseManager *m_db = nullptr;

    struct BrowserRequest {
        QUrl url;
        QString store;
        QString item;
        QString priceRegex;
        QString productRegex;
        RequestStage stage;
        QString searchUrl;
    };

    QWebEnginePage m_page;
    QQueue<BrowserRequest> m_browserQueue;
    BrowserRequest m_currentRequest;
    bool m_browserBusy = false;

    void enqueueBrowserRequest(const BrowserRequest &req);
    void startNextBrowserRequest();
};
