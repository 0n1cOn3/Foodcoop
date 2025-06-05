#pragma once
#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include "DatabaseManager.h"
#include "TrendDetector.h"
#include <QDockWidget>
#include <QComboBox>
#include <QLabel>
#include <QDateEdit>
#include <QTabWidget>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class PlotWindow : public QMainWindow
{
    Q_OBJECT
public:
    PlotWindow(DatabaseManager *db, QWidget *parent = nullptr);
    void setStoreList(const QStringList &stores);
    void setCategoryList(const QStringList &categories);

public slots:
    void updateChart();
    void onStoreChanged(const QString &store);
    void onCategoryChanged(const QString &category);
    void onFetchStarted();
    void onFetchFinished();
    void onFromDateChanged(const QDate &date);

private:
    DatabaseManager *m_db;
    TrendDetector m_detector;
    QChartView *m_chartView;
    QLineSeries *m_series;
    QTableWidget *m_table;
    QTabWidget *m_tabs;
    QTimer *m_timer;
    QDockWidget *m_menuDock;
    QLabel *m_dbSizeLabel;
    QLabel *m_currentPriceLabel;
    QLabel *m_browserStatusLabel;
    QComboBox *m_storeCombo;
    QComboBox *m_categoryCombo;
    QDateEdit *m_fromDateEdit;
    QString m_currentStore;
    QString m_currentCategory;
    void updateDbInfo();
};
