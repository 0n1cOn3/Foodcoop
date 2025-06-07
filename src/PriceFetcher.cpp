#include "PriceFetcher.h"
#include "DatabaseManager.h"
#include <QRegularExpression>
#include <QDate>
#include <QUrl>
#include <QWebEngineProfile>
#include <QWebEnginePage>

#include <QtWebEngineQuick/qtwebenginequickglobal.h>

PriceFetcher::PriceFetcher(DatabaseManager *db, QObject *parent)
    : QObject(parent), m_db(db)
{
    connect(&m_manager, &QNetworkAccessManager::finished,
            this, &PriceFetcher::onReply);
    connect(&m_manager, &QNetworkAccessManager::sslErrors,
            this, [](QNetworkReply *reply, const QList<QSslError> &) {
                reply->ignoreSslErrors();
            });
    connect(&m_page, &QWebEnginePage::loadFinished,
            this, &PriceFetcher::onBrowserLoadFinished);
    connect(&m_page, &QWebEnginePage::certificateError,
            this, [](const QWebEngineCertificateError &) {
                return true;
            });

    // Configure stores and a generic product to track. The URLs are templates
    // for a search query. We will dynamically extract the first product link and
    // then scrape its price.
    QRegularExpression productRe{R"(href=['"]([^'" ]*/p[^'" ]+)['"])"};
    QRegularExpression priceRe{R"(price[^0-9]*([0-9]+\.[0-9]{2}))"};

    m_stores = {
        {"Coop", "https://www.coop.ch/de/search/?text=%1", productRe, priceRe},
        {"Migros", "https://www.migros.ch/en/search?query=%1", productRe, priceRe},
        {"Denner", "https://www.denner.ch/de/search?q=%1", productRe, priceRe},
        {"Aldi Suisse", "https://www.aldi-now.ch/de/search?q=%1", productRe, priceRe},
        {"Lidl Suisse", "https://www.lidl.ch/q/de-CH/search?q=%1", productRe, priceRe},
        {"Ottos Warenposten", "https://www.ottos.ch/de/search/%1", productRe, priceRe},
    };

    if (m_db) {
        m_items = m_db->loadItems();
        for (const StoreInfo &info : m_stores)
            for (const QString &item : m_items)
                m_db->ensureProduct(info.store, item);
    }
}

void PriceFetcher::fetchDailyPrices()
{
    m_total = m_stores.size() * m_items.size();
    m_pending = m_total;
    emit fetchStarted();
    emit progressChanged(0, m_total);

    if (m_total == 0) {
        emit fetchFinished();
        return;
    }

    for (const StoreInfo &info : m_stores) {
        for (const QString &item : m_items) {
            QString storedUrl = m_db ? m_db->productUrl(info.store, item) : QString();
            if (!storedUrl.isEmpty()) {
                QNetworkRequest req{QUrl(storedUrl)};
                req.setHeader(QNetworkRequest::UserAgentHeader,
                              "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                              "AppleWebKit/537.36 (KHTML, like Gecko) "
                              "Chrome/120.0 Safari/537.36");
                req.setRawHeader("Accept-Language", "de-CH,de;q=0.9,en;q=0.8");
                QNetworkReply *reply = m_manager.get(req);
                reply->setProperty("store", info.store);
                reply->setProperty("item", item);
                reply->setProperty("stage", static_cast<int>(RequestStage::Product));
                reply->setProperty("priceRegex", info.priceRegex.pattern());
                reply->setProperty("productRegex", info.productRegex.pattern());
                reply->setProperty("searchUrl", info.searchUrl);
            } else {
                QUrl url(info.searchUrl.arg(QUrl::toPercentEncoding(item)));
                QNetworkRequest request(url);
                request.setHeader(QNetworkRequest::UserAgentHeader,
                                  "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                                  "AppleWebKit/537.36 (KHTML, like Gecko) "
                                  "Chrome/120.0 Safari/537.36");
                request.setRawHeader("Accept-Language", "de-CH,de;q=0.9,en;q=0.8");
                QNetworkReply *reply = m_manager.get(request);
                reply->setProperty("store", info.store);
                reply->setProperty("item", item);
                reply->setProperty("stage", static_cast<int>(RequestStage::Search));
                reply->setProperty("priceRegex", info.priceRegex.pattern());
                reply->setProperty("productRegex", info.productRegex.pattern());
                reply->setProperty("searchUrl", info.searchUrl);
            }
        }
    }
}

QStringList PriceFetcher::storeList() const
{
    QStringList list;
    for (const StoreInfo &p : m_stores) {
        if (!list.contains(p.store))
            list.append(p.store);
    }
    return list;
}

QStringList PriceFetcher::categoryList() const
{
    return m_items;
}

void PriceFetcher::addItem(const QString &item)
{
    if (m_items.contains(item))
        return;
    m_items.append(item);
    if (m_db) {
        for (const StoreInfo &info : m_stores)
            m_db->ensureProduct(info.store, item);
    }
    emit itemListChanged();
}

void PriceFetcher::onReply(QNetworkReply *reply)
{
    PriceEntry entry;
    entry.store = reply->property("store").toString();
    entry.item = reply->property("item").toString();
    entry.date = QDate::currentDate();
    entry.price = 0.0;
    entry.currency = QStringLiteral("CHF");

    IssueEntry issue;
    issue.store = entry.store;
    issue.item = entry.item;
    issue.date = entry.date;

    if (reply->error() != QNetworkReply::NoError) {
        issue.error = reply->errorString();
        emit issueOccurred(issue);
        if (--m_pending == 0) {
            emit progressChanged(m_total - m_pending, m_total);
            emit fetchFinished();
        } else {
            emit progressChanged(m_total - m_pending, m_total);
        }
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    RequestStage stage = static_cast<RequestStage>(reply->property("stage").toInt());
    bool success = false;

    if (stage == RequestStage::Product) {
        QRegularExpression regex(reply->property("priceRegex").toString());
        QRegularExpressionMatch match = regex.match(QString::fromUtf8(data));
        if (match.hasMatch()) {
            entry.price = match.captured(1).toDouble();
            emit priceFetched(entry);
            if (m_db)
                m_db->setProductUrl(entry.store, entry.item, reply->url().toString());
            success = true;
        } else {
            issue.error = QStringLiteral("Price not found");
            emit issueOccurred(issue);
        }
    } else if (stage == RequestStage::Search) {
        QRegularExpression regex(reply->property("productRegex").toString());
        QRegularExpressionMatch match = regex.match(QString::fromUtf8(data));
        if (match.hasMatch()) {
            QUrl next = reply->url().resolved(QUrl(match.captured(1)));
            if (m_db)
                m_db->setProductUrl(entry.store, entry.item, next.toString());
            QNetworkRequest req(next);
            req.setHeader(QNetworkRequest::UserAgentHeader,
                          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                          "AppleWebKit/537.36 (KHTML, like Gecko) "
                          "Chrome/120.0 Safari/537.36");
            req.setRawHeader("Accept-Language", "de-CH,de;q=0.9,en;q=0.8");
            QNetworkReply *nr = m_manager.get(req);
            nr->setProperty("store", entry.store);
            nr->setProperty("item", entry.item);
            nr->setProperty("stage", static_cast<int>(RequestStage::Product));
            nr->setProperty("priceRegex", reply->property("priceRegex").toString());
            nr->setProperty("productRegex", reply->property("productRegex").toString());
            nr->setProperty("searchUrl", reply->property("searchUrl").toString());
            reply->deleteLater();
            return;
        } else {
            issue.error = QStringLiteral("Product not found");
            emit issueOccurred(issue);
        }
    }

    if (!success && stage == RequestStage::Product) {
        QString searchUrl = reply->property("searchUrl").toString();
        if (!searchUrl.isEmpty()) {
            QUrl url(searchUrl.arg(QUrl::toPercentEncoding(entry.item)));
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::UserAgentHeader,
                          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                          "AppleWebKit/537.36 (KHTML, like Gecko) "
                          "Chrome/120.0 Safari/537.36");
            req.setRawHeader("Accept-Language", "de-CH,de;q=0.9,en;q=0.8");
            QNetworkReply *nr = m_manager.get(req);
            nr->setProperty("store", entry.store);
            nr->setProperty("item", entry.item);
            nr->setProperty("stage", static_cast<int>(RequestStage::Search));
            nr->setProperty("priceRegex", reply->property("priceRegex").toString());
            nr->setProperty("productRegex", reply->property("productRegex").toString());
            nr->setProperty("searchUrl", searchUrl);
            reply->deleteLater();
            emit issueOccurred(issue); // log failure before retrying
            return;
        }
    }

    if (!success) {
        BrowserRequest br;
        br.url = reply->url();
        br.store = entry.store;
        br.item = entry.item;
        br.priceRegex = reply->property("priceRegex").toString();
        br.productRegex = reply->property("productRegex").toString();
        br.stage = stage;
        br.searchUrl = reply->property("searchUrl").toString();
        enqueueBrowserRequest(br);
        reply->deleteLater();
        return;
    }

    if (--m_pending == 0) {
        emit progressChanged(m_total - m_pending, m_total);
        emit fetchFinished();
    } else {
        emit progressChanged(m_total - m_pending, m_total);
    }

    reply->deleteLater();
}

void PriceFetcher::enqueueBrowserRequest(const BrowserRequest &req)
{
    m_browserQueue.enqueue(req);
    if (!m_browserBusy)
        startNextBrowserRequest();
}

void PriceFetcher::startNextBrowserRequest()
{
    if (m_browserQueue.isEmpty()) {
        m_browserBusy = false;
        return;
    }
    m_browserBusy = true;
    m_currentRequest = m_browserQueue.dequeue();
    QWebEngineHttpRequest req(m_currentRequest.url);
    req.setHeader("User-Agent",
                  QByteArray("Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                             "AppleWebKit/537.36 (KHTML, like Gecko) "
                             "Chrome/120.0 Safari/537.36"));
    m_page.load(req);
}

void PriceFetcher::onBrowserLoadFinished(bool ok)
{
    if (!ok) {
        IssueEntry issue;
        issue.store = m_currentRequest.store;
        issue.item = m_currentRequest.item;
        issue.date = QDate::currentDate();
        issue.error = QStringLiteral("Browser load failed");
        emit issueOccurred(issue);
        startNextBrowserRequest();
        if (--m_pending == 0) {
            emit progressChanged(m_total - m_pending, m_total);
            emit fetchFinished();
        } else {
            emit progressChanged(m_total - m_pending, m_total);
        }
        return;
    }

    m_page.toHtml([this](const QString &html) { onBrowserHtmlReady(html); });
}

void PriceFetcher::onBrowserHtmlReady(const QString &html)
{
    PriceEntry entry;
    entry.store = m_currentRequest.store;
    entry.item = m_currentRequest.item;
    entry.date = QDate::currentDate();
    entry.price = 0.0;
    entry.currency = QStringLiteral("CHF");

    IssueEntry issue;
    issue.store = entry.store;
    issue.item = entry.item;
    issue.date = entry.date;

    if (m_currentRequest.stage == RequestStage::Product) {
        QRegularExpression regex(m_currentRequest.priceRegex);
        QRegularExpressionMatch match = regex.match(html);
        if (match.hasMatch()) {
            entry.price = match.captured(1).toDouble();
            emit priceFetched(entry);
            if (m_db)
                m_db->setProductUrl(entry.store, entry.item, m_currentRequest.url.toString());
        } else {
            issue.error = QStringLiteral("Price not found");
            emit issueOccurred(issue);
        }
    } else {
        QRegularExpression regex(m_currentRequest.productRegex);
        QRegularExpressionMatch match = regex.match(html);
        if (match.hasMatch()) {
            QUrl next = m_currentRequest.url.resolved(QUrl(match.captured(1)));
            if (m_db)
                m_db->setProductUrl(entry.store, entry.item, next.toString());
            BrowserRequest req;
            req.url = next;
            req.store = entry.store;
            req.item = entry.item;
            req.priceRegex = m_currentRequest.priceRegex;
            req.productRegex = m_currentRequest.productRegex;
            req.stage = RequestStage::Product;
            req.searchUrl = m_currentRequest.searchUrl;
            enqueueBrowserRequest(req);
            startNextBrowserRequest();
            return;
        } else {
            issue.error = QStringLiteral("Product not found");
            emit issueOccurred(issue);
        }
    }

    startNextBrowserRequest();

    if (--m_pending == 0) {
        emit progressChanged(m_total - m_pending, m_total);
        emit fetchFinished();
    } else {
        emit progressChanged(m_total - m_pending, m_total);
    }
}
