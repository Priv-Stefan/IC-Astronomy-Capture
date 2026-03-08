#include "optionsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QApplication>

OptionsDialog::OptionsDialog(QWidget *parent)
    : QDialog(parent)
    , m_settings(QApplication::organizationName(), QApplication::applicationName())
{
    setWindowTitle(tr("Options"));
    setMinimumWidth(340);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ---- Language group ----
    QGroupBox *grpLanguage = new QGroupBox(tr("Language / Sprache"), this);
    QFormLayout *frmLang = new QFormLayout(grpLanguage);

    m_comboLanguage = new QComboBox(grpLanguage);
    m_comboLanguage->addItem(tr("English"), "en");
    m_comboLanguage->addItem(tr("Deutsch"), "de");
    frmLang->addRow(tr("Application Language:"), m_comboLanguage);

    QLabel *lblNote = new QLabel(
        tr("<i>Restart the application for the language change to take effect.</i>"),
        grpLanguage);
    lblNote->setWordWrap(true);
    frmLang->addRow(lblNote);

    // ---- Button box ----
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OptionsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &OptionsDialog::reject);

    mainLayout->addWidget(grpLanguage);
    mainLayout->addStretch();
    mainLayout->addWidget(m_buttonBox);

    loadSettings();
}

QString OptionsDialog::selectedLanguage() const
{
    return m_comboLanguage->currentData().toString();
}

void OptionsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void OptionsDialog::loadSettings()
{
    QString lang = m_settings.value("options/language", "en").toString();
    int idx = m_comboLanguage->findData(lang);
    if (idx >= 0)
        m_comboLanguage->setCurrentIndex(idx);
}

void OptionsDialog::saveSettings()
{
    m_settings.setValue("options/language", selectedLanguage());
}
