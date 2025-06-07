#include <QGuiApplication>
#if HAVE_WEBENGINE
#  include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif
#include "DatabaseManager.h"
#include "PriceFetcher.h"

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
#if HAVE_WEBENGINE
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", QByteArray("1"));
    qputenv("QT_OPENGL", QByteArray("software"));
    QtWebEngineQuick::initialize();
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
#endif
    QGuiApplication app(argc, argv);

    DatabaseManager db;
    db.open("test.db");

    PriceFetcher fetcher(&db);
    const QByteArray off = qgetenv("OFFLINE_PATH");
    if (!off.isEmpty())
        fetcher.setOfflinePath(QString::fromLocal8Bit(off));
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i)
        fetcher.addItem(args.at(i));
    QObject::connect(&fetcher, &PriceFetcher::fetchFinished,
                     &app, &QGuiApplication::quit);
    fetcher.fetchDailyPrices();
    return app.exec();
}
