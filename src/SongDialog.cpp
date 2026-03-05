#include "SongDialog.h"
#include "ui_SongDialog.h"
#include <QMessageBox>

SongDialog::SongDialog(QWidget *parent, const QString &caption, const QString &lyrics, const QString &vocalLanguage)
    : QDialog(parent),
      ui(new Ui::SongDialog)
{
    ui->setupUi(this);
    
    // Set initial values if provided
    if (!caption.isEmpty()) {
        ui->captionEdit->setPlainText(caption);
    }
    if (!lyrics.isEmpty()) {
        ui->lyricsEdit->setPlainText(lyrics);
    }
    
    // Setup vocal language combo box
    ui->vocalLanguageCombo->addItem("--", "");  // Unset
    ui->vocalLanguageCombo->addItem("English (en)", "en");
    ui->vocalLanguageCombo->addItem("German (de)", "de");
    ui->vocalLanguageCombo->addItem("French (fr)", "fr");
    ui->vocalLanguageCombo->addItem("Spanish (es)", "es");
    ui->vocalLanguageCombo->addItem("Japanese (ja)", "ja");
    ui->vocalLanguageCombo->addItem("Chinese (zh)", "zh");
    ui->vocalLanguageCombo->addItem("Italian (it)", "it");
    ui->vocalLanguageCombo->addItem("Portuguese (pt)", "pt");
    ui->vocalLanguageCombo->addItem("Russian (ru)", "ru");
    
    // Set current language if provided
    if (!vocalLanguage.isEmpty()) {
        int index = ui->vocalLanguageCombo->findData(vocalLanguage);
        if (index >= 0) {
            ui->vocalLanguageCombo->setCurrentIndex(index);
        }
    } else {
        ui->vocalLanguageCombo->setCurrentIndex(0); // Default to unset
    }
}

SongDialog::~SongDialog()
{
    delete ui;
}

QString SongDialog::getCaption() const
{
    return ui->captionEdit->toPlainText();
}

QString SongDialog::getLyrics() const
{
    return ui->lyricsEdit->toPlainText();
}

QString SongDialog::getVocalLanguage() const
{
    return ui->vocalLanguageCombo->currentData().toString();
}

void SongDialog::on_okButton_clicked()
{
    // Validate that caption is not empty
    QString caption = getCaption();
    if (caption.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Caption cannot be empty.");
        return;
    }
    
    accept();
}

void SongDialog::on_cancelButton_clicked()
{
    reject();
}