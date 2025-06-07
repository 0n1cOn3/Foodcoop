#pragma once
#include <QDialog>
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const QString &version, QWidget *parent = nullptr);
    QString offlinePath() const;
    QString language() const;
    void setOfflinePath(const QString &path);
    void setLanguage(const QString &lang);
private slots:
    void browse();
private:
    QLineEdit *m_pathEdit;
    QPushButton *m_browseButton;
    QComboBox *m_languageCombo;
    QLabel *m_versionLabel;
};
