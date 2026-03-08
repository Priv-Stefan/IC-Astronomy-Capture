#pragma once

#include <QDialog>
#include <QSettings>

class QComboBox;
class QDialogButtonBox;

/**
 * @brief OptionsDialog
 *
 * Application-wide settings dialog.
 * Currently provides:
 *   - Language selection (English / German)
 *
 * Additional option groups can be added as QGroupBox sections.
 */
class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);

    /// Returns the locale string selected by the user ("en" or "de").
    QString selectedLanguage() const;

private slots:
    void accept() override;

private:
    void loadSettings();
    void saveSettings();

    QComboBox        *m_comboLanguage = nullptr;
    QDialogButtonBox *m_buttonBox     = nullptr;

    QSettings m_settings;
};
