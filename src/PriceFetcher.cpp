#include "PriceFetcher.h"
#include "DatabaseManager.h"
#include <QRegularExpression>
#include <QDate>
#include <QUrl>

PriceFetcher::PriceFetcher(DatabaseManager *db, QObject *parent)
    : QObject(parent), m_db(db)
{
    connect(&m_manager, &QNetworkAccessManager::finished,
            this, &PriceFetcher::onReply);

    // Configure stores and a generic product to track. The URLs are templates
    // for a search query. We will dynamically extract the first product link and
    // then scrape its price.
    m_stores = {
        {"Coop",
         "https://www.coop.ch/de/search/?text=%1",
         "https://www.coop.ch/en/search/?text=%1",
         QRegularExpression(R"(href=\"([^\"]+/p/\d+)\")"),
         QRegularExpression(R"(price[^0-9]*([0-9]+\.[0-9]{2}))")},
        {"Migros",
         "https://www.migros.ch/de/search?q=%1",
         QRegularExpression(R"(href=\"([^\"]+/p/\d+)\")"),
         QRegularExpression(R"(price[^0-9]*([0-9]+\.[0-9]{2}))")},
        {"Denner",
         "https://www.denner.ch/de/suche/?q=%1",
         QRegularExpression(R"(href=\"([^\"]+/p/\d+)\")"),
         QRegularExpression(R"(price[^0-9]*([0-9]+\.[0-9]{2}))")},
        {"Aldi Suisse",
         "https://www.aldi-suisse.ch/de/suchergebnis.html?search=%1",
         QRegularExpression(R"(href=\"([^\"]+/p/\d+)\")"),
         QRegularExpression(R"(price[^0-9]*([0-9]+\.[0-9]{2}))")},
        {"Lidl Suisse",
         "https://www.lidl.ch/sr?query=%1",
         QRegularExpression(R"(href=\"([^\"]+/p\d+)\")"),
         QRegularExpression(R"(price[^0-9]*([0-9]+\.[0-9]{2}))")},
        {"Ottos Warenposten",
         "https://www.ottos.ch/de/search?search=%1",
         QRegularExpression(R"(href=\"([^\"]+/p/\d+)\")"),
         QRegularExpression(R"(price[^0-9]*([0-9]+\.[0-9]{2}))")},
    };

    if (m_db) {
        m_items = m_db->loadItems();
        if (m_items.isEmpty())
            m_items = {"Milk"};
        for (const StoreInfo &info : m_stores)
            for (const QString &item : m_items)
                m_db->ensureProduct(info.store, item);
    } else {
        m_items = {"Milk"};
    }
}

void PriceFetcher::fetchDailyPrices()
{
    m_total = m_stores.size() * m_items.size();
    m_pending = m_total;
    emit fetchStarted();
    emit progressChanged(0, m_total);

    for (const StoreInfo &info : m_stores) {
        for (const QString &item : m_items) {
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

    } else {
        const QByteArray data = reply->readAll();
        QString pattern = reply->property("regex").toString();
        QRegularExpression regex(pattern);
        QRegularExpressionMatch match = regex.match(QString::fromUtf8(data));
        if (match.hasMatch()) {
            entry.price = match.captured(1).toDouble();
            emit priceFetched(entry);
        } else {
            issue.error = QStringLiteral("Price not found");
            emit issueOccurred(issue);
        }
    }

    QByteArray data = reply->readAll();
    RequestStage stage = static_cast<RequestStage>(reply->property("stage").toInt());

    if (stage == RequestStage::Search) {
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
        } else {
            QString storedUrl = m_db ? m_db->productUrl(entry.store, entry.item) : QString();
            if (!storedUrl.isEmpty()) {
                QNetworkRequest req{QUrl(storedUrl)};
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
                issue.error = QStringLiteral("Used stored URL");
                emit issueOccurred(issue);
            } else {
                issue.error = QStringLiteral("Product not found");
                emit issueOccurred(issue);
                if (--m_pending == 0) {
                    emit progressChanged(m_total - m_pending, m_total);
                    emit fetchFinished();
                } else {
                    emit progressChanged(m_total - m_pending, m_total);
                }
            }
        }
    } else {
        if (m_db)
            m_db->setProductUrl(entry.store, entry.item, reply->url().toString());
        QRegularExpression regex(reply->property("priceRegex").toString());
        QRegularExpressionMatch match = regex.match(QString::fromUtf8(data));
        if (match.hasMatch()) {
            entry.price = match.captured(1).toDouble();
            emit priceFetched(entry);
        } else {
            issue.error = QStringLiteral("Price not found");
            emit issueOccurred(issue);
        }
        if (--m_pending == 0) {
            emit progressChanged(m_total - m_pending, m_total);
            emit fetchFinished();
        } else {
            emit progressChanged(m_total - m_pending, m_total);
        }
    }

    reply->deleteLater();
}
