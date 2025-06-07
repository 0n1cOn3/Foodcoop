#include <QApplication>
#if HAVE_WEBENGINE
#  include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif
#include "PriceFetcher.h"
#include "DatabaseManager.h"
#include "PlotWindow.h"
#include "FirstRunDialog.h"
#include <QEventLoop>
#include <QMessageBox>

static bool performFirstScrape(PriceFetcher &fetcher, DatabaseManager &db)
{
    if (fetcher.categoryList().isEmpty())
        return true;
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
    while (!db.hasPrices() && !canceled) {
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
    return db.hasPrices() && !canceled;
}

int main(int argc, char *argv[])
{
#if HAVE_WEBENGINE
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", QByteArray("1"));
    QtWebEngineQuick::initialize();
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
#endif
    QApplication app(argc, argv);
    QApplication::setApplicationName("Foodcoop");

    DatabaseManager db;
    db.open("prices.db");

    PlotWindow w(&db);

    PriceFetcher fetcher(&db);
    const QByteArray off = qgetenv("OFFLINE_PATH");
    if (!off.isEmpty())
        fetcher.setOfflinePath(QString::fromLocal8Bit(off));
    w.setStoreList(fetcher.storeList());
    w.setCategoryList(fetcher.categoryList());
    QObject::connect(&fetcher, &PriceFetcher::priceFetched,
                     [&db](const PriceEntry &entry) { db.insertPrice(entry); });
    QObject::connect(&fetcher, &PriceFetcher::issueOccurred,
                     [&db, &w](const IssueEntry &issue) {
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
    if (!db.hasPrices()) {
        if (!performFirstScrape(fetcher, db))
            return 0;
    }

    w.show();
    if (!db.hasPrices() && !fetcher.categoryList().isEmpty())
        fetcher.fetchDailyPrices();

    return app.exec();
}
