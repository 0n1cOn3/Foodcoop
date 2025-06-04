#pragma once
#include <QMainWindow>
#include <QChartView>
#include <QLineSeries>
#include "DatabaseManager.h"
#include "TrendDetector.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class PlotWindow : public QMainWindow
{
    Q_OBJECT
public:
    PlotWindow(DatabaseManager *db, QWidget *parent = nullptr);

private slots:
    void updateChart();

private:
    DatabaseManager *m_db;
    TrendDetector m_detector;
    QtCharts::QChartView *m_chartView;
    QtCharts::QLineSeries *m_series;
    QTimer *m_timer;
};
