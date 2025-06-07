#include "PriceFetcher.h"
#include "DatabaseManager.h"
#include <QRegularExpression>
#include <QDate>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#if HAVE_WEBENGINE
#  include <QWebEngineProfile>
#  include <QWebEnginePage>
#  include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif

static QString slugify(const QString &text)
{
    QString out = text.toLower();
    out.replace(QRegularExpression("[^a-z0-9]+"), "_");
    return out;
}

static QString findOfflineHtml(const QString &dirPath,
                               const QString &store,
                               const QString &item)
{
    QDir dir(dirPath);
    QString slugStore = slugify(store);
    QString slugItem = slugify(item);
    const QStringList files = dir.entryList(QStringList() << "*.html",
                                           QDir::Files);
    for (const QString &file : files) {
        QString slugName = slugify(QFileInfo(file).completeBaseName());
        if (slugName.contains(slugStore) && slugName.contains(slugItem))
            return dir.filePath(file);
    }
    return QString();
}

PriceFetcher::PriceFetcher(DatabaseManager *db, QObject *parent)
    : QObject(parent), m_db(db)
{
    connect(&m_manager, &QNetworkAccessManager::finished,
            this, &PriceFetcher::onReply);
    connect(&m_manager, &QNetworkAccessManager::sslErrors,
            this, [](QNetworkReply *reply, const QList<QSslError> &) {
                reply->ignoreSslErrors();
            });
#if HAVE_WEBENGINE
    connect(&m_page, &QWebEnginePage::loadFinished,
            this, &PriceFetcher::onBrowserLoadFinished);
    connect(&m_page, &QWebEnginePage::certificateError,
            this, [](const QWebEngineCertificateError &) {
                return true;
            });
#endif

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

void PriceFetcher::setOfflinePath(const QString &path)
{
    m_offlinePath = path;
    scanOfflineFolder();
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
            if (!m_offlinePath.isEmpty()) {
                QString path = findOfflineHtml(m_offlinePath, info.store, item);
                QFile file(path);

                PriceEntry entry;
                entry.store = info.store;
                entry.item = item;
                entry.date = QDate::currentDate();
                entry.price = 0.0;
                entry.currency = QStringLiteral("CHF");

                IssueEntry issue;
                issue.store = entry.store;
                issue.item = entry.item;
                issue.date = entry.date;

                if (file.open(QIODevice::ReadOnly)) {
                    QString html = QString::fromUtf8(file.readAll());
                    QRegularExpression regex(info.priceRegex);
                    QRegularExpressionMatch match = regex.match(html);
                    if (match.hasMatch()) {
                        entry.price = match.captured(1).toDouble();
                        emit priceFetched(entry);
                    } else {
                        issue.error = QStringLiteral("Price not found");
                        emit issueOccurred(issue);
                    }
                } else {
                    issue.error = QStringLiteral("Offline file not found");
                    emit issueOccurred(issue);
                }

                if (--m_pending == 0) {
                    emit progressChanged(m_total - m_pending, m_total);
                    emit fetchFinished();
                } else {
                    emit progressChanged(m_total - m_pending, m_total);
                }
                continue;
            }
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
        QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        QString errorText;
        if (status.isValid())
            errorText = QString::number(status.toInt()) + QLatin1Char(' ');
        errorText += reply->errorString();

        QByteArray snippet = reply->read(200);
        if (!snippet.isEmpty()) {
            errorText += QLatin1String(" | ");
            errorText += QString::fromUtf8(snippet);
        }

        issue.error = errorText;
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

#if HAVE_WEBENGINE
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

#else
void PriceFetcher::enqueueBrowserRequest(const BrowserRequest &req)
{
    IssueEntry issue;
    issue.store = req.store;
    issue.item = req.item;
    issue.date = QDate::currentDate();
    issue.error = QStringLiteral("Qt WebEngine unavailable");
    emit issueOccurred(issue);
    if (--m_pending == 0) {
        emit progressChanged(m_total - m_pending, m_total);
        emit fetchFinished();
    } else {
        emit progressChanged(m_total - m_pending, m_total);
    }
    Q_UNUSED(req);
}

void PriceFetcher::startNextBrowserRequest() {}

void PriceFetcher::onBrowserLoadFinished(bool)
{
    if (--m_pending == 0) {
        emit progressChanged(m_total - m_pending, m_total);
        emit fetchFinished();
    } else {
        emit progressChanged(m_total - m_pending, m_total);
    }
}

void PriceFetcher::onBrowserHtmlReady(const QString &)
{
}
#endif

void PriceFetcher::scanOfflineFolder()
{
    if (m_offlinePath.isEmpty())
        return;

    QDir dir(m_offlinePath);
    const QStringList files = dir.entryList(QStringList() << "*.html" << "*.htm", QDir::Files);
    for (const QString &fileName : files) {
        QString path = dir.filePath(fileName);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;

        QString html = QString::fromUtf8(file.readAll());
        QString slugName = slugify(QFileInfo(fileName).completeBaseName());

        const StoreInfo *storeInfo = nullptr;
        for (const StoreInfo &info : m_stores) {
            QString slugStore = slugify(info.store);
            if (slugName.contains(slugStore) || html.contains(info.store, Qt::CaseInsensitive)) {
                storeInfo = &info;
                break;
            }
        }
        if (!storeInfo)
            continue;

        QString product;
        QRegularExpression titleRe(QStringLiteral("<title>([^<]+)</title>"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch tm = titleRe.match(html);
        if (tm.hasMatch()) {
            product = tm.captured(1).split(QRegularExpression("[\u2022|\-]")).first().trimmed();
        }

        double price = 0.0;
        QRegularExpression priceRe(storeInfo->priceRegex);
        QRegularExpressionMatch pm = priceRe.match(html);
        if (pm.hasMatch())
            price = pm.captured(1).toDouble();

        QRegularExpression countryRe(QStringLiteral("(?:Herkunft|Origin|Country)[^<:]*:?\\s*([^<\n]+)"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch cm = countryRe.match(html);
        QString country = cm.hasMatch() ? cm.captured(1).trimmed() : QString();

        QRegularExpression weightRe(QStringLiteral("([0-9]+(?:,[0-9]+)?\\s*(?:g|kg|ml|l))"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch wm = weightRe.match(html);
        QString weight = wm.hasMatch() ? wm.captured(1).trimmed() : QString();

        qInfo() << "Scanned" << fileName << "store:" << storeInfo->store
                << "item:" << product << "price:" << price
                << "country:" << country << "weight:" << weight;
    }
}

