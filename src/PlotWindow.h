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
#include <QLineEdit>
#include <QPushButton>
#include <QMenuBar>

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
    void onIssueOccurred(const IssueEntry &issue);
    void onFromDateChanged(const QDate &date);
    void updateIssues();
    void showFullLog();
    void openSettings();
    void retranslateUi();

signals:
    void addItemRequested(const QString &item);
    void offlinePathChanged(const QString &path);
    void languageChanged(const QString &lang);

private:
    DatabaseManager *m_db;
    TrendDetector m_detector;
    QChartView *m_chartView;
    QLineSeries *m_series;
    QTableWidget *m_table;
    QTableWidget *m_issueTable;
    QTabWidget *m_tabs;
    QTimer *m_timer;
    QDockWidget *m_menuDock;
    QLabel *m_dbSizeLabel;
    QLabel *m_currentPriceLabel;
    QLabel *m_browserStatusLabel;
    QComboBox *m_storeCombo;
    QComboBox *m_categoryCombo;
    QLineEdit *m_newItemEdit;
    QPushButton *m_addItemButton;
    QPushButton *m_showLogButton;
    QDateEdit *m_fromDateEdit;
    QString m_currentStore;
    QString m_currentCategory;
    void updateDbInfo();
};
