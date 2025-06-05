#include "PriceFetcher.h"
#include <QRegularExpression>
#include <QDate>
#include <QUrl>

PriceFetcher::PriceFetcher(QObject *parent)
    : QObject(parent)
{
    connect(&m_manager, &QNetworkAccessManager::finished,
            this, &PriceFetcher::onReply);

    // Example product pages. These URLs are publicly accessible HTML pages and
    // not official API endpoints. We request them with a desktop User-Agent and
    // extract the price using a regular expression. The regex patterns are
    // simple and may need adjustment if the page structure changes.
    m_products = {
        {"Coop", "Milk",
         "https://www.coop.ch/en/shop/getraenke/milch/coop-milch-35-/p/614300600000",
         QRegularExpression("price[^0-9]*([0-9]+\\.[0-9]{2})")},
        {"Migros", "Milk",
         "https://produkte.migros.ch/migros-milch-vollmilch",
         QRegularExpression("price[^0-9]*([0-9]+\\.[0-9]{2})")},
        {"Denner", "Milk",
         "https://www.denner.ch/de/shop/getraenke/milch/p/30700/",
         QRegularExpression("price[^0-9]*([0-9]+\\.[0-9]{2})")},
        {"Aldi Suisse", "Milk",
         "https://www.aldi-suisse.ch/de/sortiment/kuhlprodukte/milch/p/10203/",
         QRegularExpression("price[^0-9]*([0-9]+\\.[0-9]{2})")},
        {"Lidl Suisse", "Milk",
         "https://www.lidl.ch/de/Milch/p1000",
         QRegularExpression("price[^0-9]*([0-9]+\\.[0-9]{2})")}
    };
}

void PriceFetcher::fetchDailyPrices()
{
    m_pending = m_products.size();
    emit fetchStarted();
    for (const StoreProduct &p : m_products) {
        QNetworkRequest request(QUrl(p.url));
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                          "(KHTML, like Gecko) Chrome/120.0 Safari/537.36");
        QNetworkReply *reply = m_manager.get(request);
        reply->setProperty("store", p.store);
        reply->setProperty("item", p.item);
        reply->setProperty("regex", p.priceRegex.pattern());
    }
}

QStringList PriceFetcher::storeList() const
{
    QStringList list;
    for (const StoreProduct &p : m_products) {
        if (!list.contains(p.store))
            list.append(p.store);
    }
    return list;
}

QStringList PriceFetcher::categoryList() const
{
    QStringList list;
    for (const StoreProduct &p : m_products) {
        if (!list.contains(p.item))
            list.append(p.item);
    }
    return list;
}

void PriceFetcher::onReply(QNetworkReply *reply)
{
    PriceEntry entry;
    entry.store = reply->property("store").toString();
    entry.item = reply->property("item").toString();
    entry.date = QDate::currentDate();
    entry.price = 0.0;

    const QByteArray data = reply->readAll();
    QString pattern = reply->property("regex").toString();
    QRegularExpression regex(pattern);
    QRegularExpressionMatch match = regex.match(QString::fromUtf8(data));
    if (match.hasMatch())
        entry.price = match.captured(1).toDouble();

    emit priceFetched(entry);

    if (--m_pending == 0)
        emit fetchFinished();

    reply->deleteLater();
}
