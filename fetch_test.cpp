#include <QGuiApplication>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#include "DatabaseManager.h"
#include "PriceFetcher.h"

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", QByteArray("1"));
    qputenv("QT_OPENGL", QByteArray("software"));
    QtWebEngineQuick::initialize();
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    QGuiApplication app(argc, argv);

    DatabaseManager db;
    db.open("test.db");

    PriceFetcher fetcher(&db);
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i)
        fetcher.addItem(args.at(i));
    QObject::connect(&fetcher, &PriceFetcher::fetchFinished,
                     &app, &QGuiApplication::quit);
    fetcher.fetchDailyPrices();
    return app.exec();
}
