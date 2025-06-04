#include <QApplication>
#include "PriceFetcher.h"
#include "DatabaseManager.h"
#include "PlotWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DatabaseManager db;
    db.open("prices.db");

    PlotWindow w(&db);
    w.show();

    PriceFetcher fetcher;
    QObject::connect(&fetcher, &PriceFetcher::priceFetched,
                     [&db](const PriceEntry &entry){ db.insertPrice(entry); });
    fetcher.fetchDailyPrices();

    return app.exec();
}
