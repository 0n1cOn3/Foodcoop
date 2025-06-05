#include <QApplication>
#include "PriceFetcher.h"
#include "DatabaseManager.h"
#include "PlotWindow.h"
#include "FirstRunDialog.h"
#include <QEventLoop>
#include <QMessageBox>

static bool performFirstScrape(PriceFetcher &fetcher, DatabaseManager &db, const QStringList &stores)
{
    FirstRunDialog dialog;
    QObject::connect(&fetcher, &PriceFetcher::progressChanged,
                     &dialog, &FirstRunDialog::onProgress);
    QObject::connect(&fetcher, &PriceFetcher::issueOccurred,
                     &dialog, &FirstRunDialog::onIssue);
    QObject::connect(&fetcher, &PriceFetcher::fetchStarted,
                     &dialog, &FirstRunDialog::onFetchStarted);

    bool canceled = false;
    QObject::connect(&dialog, &FirstRunDialog::canceled, [&]() { canceled = true; });
    dialog.show();

    bool firstAttempt = true;
    while (!db.hasPricesForAllStores(stores) && !canceled) {
        QEventLoop loop;
        QObject::connect(&fetcher, &PriceFetcher::fetchFinished,
                         &loop, &QEventLoop::quit);
        fetcher.fetchDailyPrices();
        loop.exec();

        if (firstAttempt) {
            firstAttempt = false;
            if (!db.hasPrices()) {
                QMessageBox::warning(&dialog, QObject::tr("No Data"),
                                     QObject::tr("None of the stores returned any data. "
                                                 "The application cannot continue."));
                canceled = true;
            }
        }
    }

    dialog.hide();
    return db.hasPricesForAllStores(stores) && !canceled;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Foodcoop");

    DatabaseManager db;
    db.open("prices.db");

    PlotWindow w(&db);

    PriceFetcher fetcher(&db);
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
    QObject::connect(&w, &PlotWindow::addItemRequested,
                     &fetcher, &PriceFetcher::addItem);
    QObject::connect(&fetcher, &PriceFetcher::itemListChanged,
                     [&w, &fetcher]() {
                         w.setCategoryList(fetcher.categoryList());
                         w.updateChart();
                     });
    QStringList stores = fetcher.storeList();
    if (!db.hasPricesForAllStores(stores)) {
        if (!performFirstScrape(fetcher, db, stores))
            return 0;
    }

    w.show();

    return app.exec();
}
