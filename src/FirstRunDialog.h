#pragma once
#include <QDialog>
#include <QProgressBar>
#include <QPushButton>
#include <QPlainTextEdit>
#include "PriceFetcher.h"

class FirstRunDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FirstRunDialog(QWidget *parent = nullptr);
    bool isCanceled() const;

public slots:
    void onProgress(int done, int total);
    void onIssue(const IssueEntry &issue);
    void onFetchStarted();

signals:
    void canceled();

private:
    QProgressBar *m_bar;
    QPushButton *m_cancelButton;
    QPushButton *m_toggleButton;
    QPlainTextEdit *m_log;
    bool m_canceled = false;
};
