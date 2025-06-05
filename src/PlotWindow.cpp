#include "PlotWindow.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>
#include <QDockWidget>
#include <QComboBox>
#include <QLabel>
#include <QFileInfo>
#include <QDateEdit>
#include <QTabWidget>
#include <QTableWidget>


PlotWindow::PlotWindow(DatabaseManager *db, QWidget *parent)
    : QMainWindow(parent), m_db(db)
{
    m_series = new QLineSeries(this);
    m_chartView = new QChartView(new QChart(), this);
    m_chartView->chart()->addSeries(m_series);
    m_chartView->chart()->createDefaultAxes();

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("Date"), tr("Price")});

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(m_chartView, tr("Chart"));
    m_tabs->addTab(m_table, tr("Table"));
    setCentralWidget(m_tabs);

    // Create user menu dock
    m_menuDock = new QDockWidget(tr("Menu"), this);
    QWidget *dockWidget = new QWidget(m_menuDock);
    QVBoxLayout *dockLayout = new QVBoxLayout(dockWidget);

    m_dbSizeLabel = new QLabel(dockWidget);
    dockLayout->addWidget(m_dbSizeLabel);

    m_browserStatusLabel = new QLabel(tr("Browser: Idle"), dockWidget);
    dockLayout->addWidget(m_browserStatusLabel);

    dockLayout->addWidget(new QLabel(tr("Market:"), dockWidget));
    m_storeCombo = new QComboBox(dockWidget);
    m_storeCombo->addItems({"Coop", "Migros", "Denner", "Aldi Suisse", "Lidl Suisse"});
    dockLayout->addWidget(m_storeCombo);
    connect(m_storeCombo, &QComboBox::currentTextChanged, this, &PlotWindow::onStoreChanged);

    dockLayout->addWidget(new QLabel(tr("Category:"), dockWidget));
    m_categoryCombo = new QComboBox(dockWidget);
    m_categoryCombo->addItems({"Milk"});
    dockLayout->addWidget(m_categoryCombo);
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, &PlotWindow::onCategoryChanged);

    dockLayout->addWidget(new QLabel(tr("From:"), dockWidget));
    m_fromDateEdit = new QDateEdit(QDate::currentDate().addDays(-30), dockWidget);
    m_fromDateEdit->setCalendarPopup(true);
    dockLayout->addWidget(m_fromDateEdit);
    connect(m_fromDateEdit, &QDateEdit::dateChanged, this, &PlotWindow::onFromDateChanged);

    m_currentPriceLabel = new QLabel(dockWidget);
    dockLayout->addWidget(m_currentPriceLabel);

    dockWidget->setLayout(dockLayout);
    m_menuDock->setWidget(dockWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_menuDock);

    m_currentStore = m_storeCombo->currentText();
    m_currentCategory = m_categoryCombo->currentText();
    updateDbInfo();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PlotWindow::updateChart);
    m_timer->start(10000); // refresh every 10 seconds

    updateChart();
}

void PlotWindow::updateChart()
{
    QList<PriceEntry> prices = m_db->loadPrices(m_currentCategory, m_currentStore,
                                               m_fromDateEdit->date());
    m_series->clear();
    for (const PriceEntry &p : prices) {
        m_series->append(p.date.toJulianDay(), p.price);
    }
    m_chartView->chart()->createDefaultAxes();
    QString trend = m_detector.detectTrend(prices);
    m_chartView->chart()->setTitle(tr("Price trend: %1").arg(trend));

    m_table->setRowCount(prices.size());
    for (int i = 0; i < prices.size(); ++i) {
        m_table->setItem(i, 0, new QTableWidgetItem(prices[i].date.toString(Qt::ISODate)));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::number(prices[i].price)));
    }

    PriceEntry latest = m_db->latestPrice(m_currentCategory, m_currentStore);
    if (latest.date.isValid())
        m_currentPriceLabel->setText(tr("Current price: %1 (%2)")
                                        .arg(latest.price)
                                        .arg(latest.date.toString(Qt::ISODate)));
    else
        m_currentPriceLabel->setText(tr("Current price: N/A"));
}

void PlotWindow::onStoreChanged(const QString &store)
{
    m_currentStore = store;
    updateChart();
}

void PlotWindow::onCategoryChanged(const QString &category)
{
    m_currentCategory = category;
    updateChart();
}

void PlotWindow::onFetchStarted()
{
    m_browserStatusLabel->setText(tr("Browser: Fetching"));
}

void PlotWindow::onFetchFinished()
{
    m_browserStatusLabel->setText(tr("Browser: Idle"));
    updateDbInfo();
}

void PlotWindow::onFromDateChanged(const QDate &)
{
    updateChart();
}

void PlotWindow::updateDbInfo()
{
    QFileInfo info(m_db->databasePath());
    m_dbSizeLabel->setText(tr("Database size: %1 KB").arg(info.size() / 1024));
    PriceEntry latest = m_db->latestPrice(m_currentCategory, m_currentStore);
    if (latest.date.isValid())
        m_currentPriceLabel->setText(tr("Current price: %1 (%2)")
                                        .arg(latest.price)
                                        .arg(latest.date.toString(Qt::ISODate)));
    else
        m_currentPriceLabel->setText(tr("Current price: N/A"));
}

void PlotWindow::setStoreList(const QStringList &stores)
{
    m_storeCombo->clear();
    m_storeCombo->addItems(stores);
    m_currentStore = m_storeCombo->currentText();
    updateChart();
}

void PlotWindow::setCategoryList(const QStringList &categories)
{
    m_categoryCombo->clear();
    m_categoryCombo->addItems(categories);
    m_currentCategory = m_categoryCombo->currentText();
    updateChart();
}
