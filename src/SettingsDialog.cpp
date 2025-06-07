#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>

SettingsDialog::SettingsDialog(const QString &version, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);
    m_browseButton = new QPushButton(tr("Select Folder"), this);
    pathLayout->addWidget(new QLabel(tr("Offline Folder:"), this));
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_browseButton);
    layout->addLayout(pathLayout);

    QHBoxLayout *langLayout = new QHBoxLayout();
    langLayout->addWidget(new QLabel(tr("Language:"), this));
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem("Deutsch", "de");
    m_languageCombo->addItem("Français", "fr");
    m_languageCombo->addItem("Italiano", "it");
    langLayout->addWidget(m_languageCombo);
    layout->addLayout(langLayout);

    m_versionLabel = new QLabel(tr("Version: %1").arg(version), this);
    layout->addWidget(m_versionLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *ok = new QPushButton(tr("OK"), this);
    QPushButton *cancel = new QPushButton(tr("Cancel"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(ok);
    btnLayout->addWidget(cancel);
    layout->addLayout(btnLayout);

    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_browseButton, &QPushButton::clicked, this, &SettingsDialog::browse);
}

QString SettingsDialog::offlinePath() const
{
    return m_pathEdit->text();
}

QString SettingsDialog::language() const
{
    return m_languageCombo->currentData().toString();
}

void SettingsDialog::setOfflinePath(const QString &path)
{
    m_pathEdit->setText(path);
}

void SettingsDialog::setLanguage(const QString &lang)
{
    int idx = m_languageCombo->findData(lang);
    if (idx >= 0)
        m_languageCombo->setCurrentIndex(idx);
}

void SettingsDialog::browse()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Folder"), m_pathEdit->text());
    if (!dir.isEmpty())
        m_pathEdit->setText(dir);
}
