#include "SongDialog.h"
#include "ui_SongDialog.h"
#include <QMessageBox>

SongDialog::SongDialog(QWidget *parent, const QString &caption, const QString &lyrics, const QString &vocalLanguage)
	: QDialog(parent),
	  ui(new Ui::SongDialog)
{
	ui->setupUi(this);

	// Set initial values if provided
	if (!caption.isEmpty())
	{
		ui->captionEdit->setPlainText(caption);
	}
	if (!lyrics.isEmpty())
	{
		ui->lyricsEdit->setPlainText(lyrics);
	}

	// Setup vocal language combo box
	ui->vocalLanguageCombo->addItem("--", "");  // Unset
	ui->vocalLanguageCombo->addItem("English (en)", "en");
	ui->vocalLanguageCombo->addItem("Chinese (zh)", "zh");
	ui->vocalLanguageCombo->addItem("Japanese (ja)", "ja");
	ui->vocalLanguageCombo->addItem("Korean (ko)", "ko");
	ui->vocalLanguageCombo->addItem("Spanish (es)", "es");
	ui->vocalLanguageCombo->addItem("French (fr)", "fr");
	ui->vocalLanguageCombo->addItem("German (de)", "de");
	ui->vocalLanguageCombo->addItem("Portuguese (pt)", "pt");
	ui->vocalLanguageCombo->addItem("Russian (ru)", "ru");
	ui->vocalLanguageCombo->addItem("Italian (it)", "it");
	ui->vocalLanguageCombo->addItem("Arabic (ar)", "ar");
	ui->vocalLanguageCombo->addItem("Azerbaijani (az)", "az");
	ui->vocalLanguageCombo->addItem("Bulgarian (bg)", "bg");
	ui->vocalLanguageCombo->addItem("Bengali (bn)", "bn");
	ui->vocalLanguageCombo->addItem("Catalan (ca)", "ca");
	ui->vocalLanguageCombo->addItem("Czech (cs)", "cs");
	ui->vocalLanguageCombo->addItem("Danish (da)", "da");
	ui->vocalLanguageCombo->addItem("Greek (el)", "el");
	ui->vocalLanguageCombo->addItem("Persian (fa)", "fa");
	ui->vocalLanguageCombo->addItem("Finnish (fi)", "fi");
	ui->vocalLanguageCombo->addItem("Hebrew (he)", "he");
	ui->vocalLanguageCombo->addItem("Hindi (hi)", "hi");
	ui->vocalLanguageCombo->addItem("Croatian (hr)", "hr");
	ui->vocalLanguageCombo->addItem("Haitian Creole (ht)", "ht");
	ui->vocalLanguageCombo->addItem("Hungarian (hu)", "hu");
	ui->vocalLanguageCombo->addItem("Indonesian (id)", "id");
	ui->vocalLanguageCombo->addItem("Icelandic (is)", "is");
	ui->vocalLanguageCombo->addItem("Latin (la)", "la");
	ui->vocalLanguageCombo->addItem("Lithuanian (lt)", "lt");
	ui->vocalLanguageCombo->addItem("Malay (ms)", "ms");
	ui->vocalLanguageCombo->addItem("Nepali (ne)", "ne");
	ui->vocalLanguageCombo->addItem("Dutch (nl)", "nl");
	ui->vocalLanguageCombo->addItem("Norwegian (no)", "no");
	ui->vocalLanguageCombo->addItem("Punjabi (pa)", "pa");
	ui->vocalLanguageCombo->addItem("Polish (pl)", "pl");
	ui->vocalLanguageCombo->addItem("Romanian (ro)", "ro");
	ui->vocalLanguageCombo->addItem("Sanskrit (sa)", "sa");
	ui->vocalLanguageCombo->addItem("Slovak (sk)", "sk");
	ui->vocalLanguageCombo->addItem("Serbian (sr)", "sr");
	ui->vocalLanguageCombo->addItem("Swedish (sv)", "sv");
	ui->vocalLanguageCombo->addItem("Swahili (sw)", "sw");
	ui->vocalLanguageCombo->addItem("Tamil (ta)", "ta");
	ui->vocalLanguageCombo->addItem("Telugu (te)", "te");
	ui->vocalLanguageCombo->addItem("Thai (th)", "th");
	ui->vocalLanguageCombo->addItem("Tagalog (tl)", "tl");
	ui->vocalLanguageCombo->addItem("Turkish (tr)", "tr");
	ui->vocalLanguageCombo->addItem("Ukrainian (uk)", "uk");
	ui->vocalLanguageCombo->addItem("Urdu (ur)", "ur");
	ui->vocalLanguageCombo->addItem("Vietnamese (vi)", "vi");
	ui->vocalLanguageCombo->addItem("Cantonese (yue)", "yue");
	ui->vocalLanguageCombo->addItem("Unknown", "unknown");

	// Set current language if provided
	if (!vocalLanguage.isEmpty())
	{
		int index = ui->vocalLanguageCombo->findData(vocalLanguage);
		if (index >= 0)
		{
			ui->vocalLanguageCombo->setCurrentIndex(index);
		}
	}
	else
	{
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
	if (caption.trimmed().isEmpty())
	{
		QMessageBox::warning(this, "Invalid Input", "Caption cannot be empty.");
		return;
	}

	accept();
}

void SongDialog::on_cancelButton_clicked()
{
	reject();
}