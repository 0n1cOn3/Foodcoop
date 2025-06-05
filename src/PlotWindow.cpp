#include "PlotWindow.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>


PlotWindow::PlotWindow(DatabaseManager *db, QWidget *parent)
    : QMainWindow(parent), m_db(db)
{
    m_series = new QLineSeries(this);
    m_chartView = new QChartView(new QChart(), this);
    m_chartView->chart()->addSeries(m_series);
    m_chartView->chart()->createDefaultAxes();
    setCentralWidget(m_chartView);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PlotWindow::updateChart);
    m_timer->start(10000); // refresh every 10 seconds

    updateChart();
}

void PlotWindow::updateChart()
{
    QList<PriceEntry> prices = m_db->loadPrices("sample");
    m_series->clear();
    for (const PriceEntry &p : prices) {
        m_series->append(p.date.toJulianDay(), p.price);
    }
    m_chartView->chart()->createDefaultAxes();
    QString trend = m_detector.detectTrend(prices);
    m_chartView->chart()->setTitle("Price trend: " + trend);
}
