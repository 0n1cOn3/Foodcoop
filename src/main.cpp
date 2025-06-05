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
    w.setStoreList(fetcher.storeList());
    w.setCategoryList(fetcher.categoryList());
    QObject::connect(&fetcher, &PriceFetcher::priceFetched,
                     [&db](const PriceEntry &entry){ db.insertPrice(entry); });
    QObject::connect(&fetcher, &PriceFetcher::issueOccurred,
                     [&db, &w](const IssueEntry &issue){
                         db.insertIssue(issue);
                         w.onIssueOccurred(issue);
                     });
    QObject::connect(&fetcher, &PriceFetcher::fetchStarted,
                     &w, &PlotWindow::onFetchStarted);
    QObject::connect(&fetcher, &PriceFetcher::fetchFinished,
                     &w, &PlotWindow::onFetchFinished);
    if (!db.hasPrices())
        fetcher.fetchDailyPrices();

    return app.exec();
}
