#pragma once
#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
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
    QChartView *m_chartView;
    QLineSeries *m_series;
    QTimer *m_timer;
};
