#include "FirstRunDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

FirstRunDialog::FirstRunDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Initial Scrape"));
    m_bar = new QProgressBar(this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_toggleButton = new QPushButton(tr("Show Log"), this);
    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setVisible(false);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_cancelButton);
    btnLayout->addWidget(m_toggleButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_bar);
    layout->addLayout(btnLayout);
    layout->addWidget(m_log);
    setLayout(layout);

    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        m_canceled = true;
        emit canceled();
    });

    connect(m_toggleButton, &QPushButton::clicked, this, [this]() {
        bool vis = !m_log->isVisible();
        m_log->setVisible(vis);
        m_toggleButton->setText(vis ? tr("Hide Log") : tr("Show Log"));
    });
}

bool FirstRunDialog::isCanceled() const
{
    return m_canceled;
}

void FirstRunDialog::onProgress(int done, int total)
{
    m_bar->setMaximum(total);
    m_bar->setValue(done);
}

void FirstRunDialog::onIssue(const IssueEntry &issue)
{
    m_log->appendPlainText(QString("%1|%2|%3|%4")
                               .arg(issue.store)
                               .arg(issue.item)
                               .arg(issue.date.toString(Qt::ISODate))
                               .arg(issue.error));
}

void FirstRunDialog::onFetchStarted()
{
    m_bar->setValue(0);
}
