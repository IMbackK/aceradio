// Copyright Carl Philipp Klemm 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AdvancedSettingsDialog.h"
#include "ui_AdvancedSettingsDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonParseError>

AdvancedSettingsDialog::AdvancedSettingsDialog(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::AdvancedSettingsDialog)
{
	ui->setupUi(this);

	// Connect signals and slots explicitly
	connect(ui->qwen3BrowseButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::onQwen3BrowseButtonClicked);
	connect(ui->textEncoderBrowseButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::onTextEncoderBrowseButtonClicked);
	connect(ui->ditBrowseButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::onDiTBrowseButtonClicked);
	connect(ui->vaeBrowseButton, &QPushButton::clicked, this, &AdvancedSettingsDialog::onVAEBrowseButtonClicked);
}

AdvancedSettingsDialog::~AdvancedSettingsDialog()
{
	delete ui;
}

QString AdvancedSettingsDialog::getJsonTemplate() const
{
	return ui->jsonTemplateEdit->toPlainText();
}

QString AdvancedSettingsDialog::getQwen3ModelPath() const
{
	return ui->qwen3ModelEdit->text();
}

QString AdvancedSettingsDialog::getTextEncoderModelPath() const
{
	return ui->textEncoderEdit->text();
}

QString AdvancedSettingsDialog::getDiTModelPath() const
{
	return ui->ditModelEdit->text();
}

QString AdvancedSettingsDialog::getVAEModelPath() const
{
	return ui->vaeModelEdit->text();
}

bool AdvancedSettingsDialog::getLowVramMode() const
{
	return ui->lowVramCheckBox->isChecked();
}

bool AdvancedSettingsDialog::getFlashAttention() const
{
	return ui->flashAttentionCheckBox->isChecked();
}

void AdvancedSettingsDialog::setJsonTemplate(const QString &templateStr)
{
	ui->jsonTemplateEdit->setPlainText(templateStr);
}

void AdvancedSettingsDialog::setQwen3ModelPath(const QString &path)
{
	ui->qwen3ModelEdit->setText(path);
}

void AdvancedSettingsDialog::setTextEncoderModelPath(const QString &path)
{
	ui->textEncoderEdit->setText(path);
}

void AdvancedSettingsDialog::setDiTModelPath(const QString &path)
{
	ui->ditModelEdit->setText(path);
}

void AdvancedSettingsDialog::setVAEModelPath(const QString &path)
{
	ui->vaeModelEdit->setText(path);
}

void AdvancedSettingsDialog::setLowVramMode(bool enabled)
{
	ui->lowVramCheckBox->setChecked(enabled);
}

void AdvancedSettingsDialog::setFlashAttention(bool enabled)
{
	ui->flashAttentionCheckBox->setChecked(enabled);
}

void AdvancedSettingsDialog::onQwen3BrowseButtonClicked()
{
	QString file = QFileDialog::getOpenFileName(this, "Select Qwen3 Model", ui->qwen3ModelEdit->text(),
	               "GGUF Files (*.gguf)");
	if (!file.isEmpty())
	{
		ui->qwen3ModelEdit->setText(file);
	}
}

void AdvancedSettingsDialog::onTextEncoderBrowseButtonClicked()
{
	QString file = QFileDialog::getOpenFileName(this, "Select Text Encoder Model", ui->textEncoderEdit->text(),
	               "GGUF Files (*.gguf)");
	if (!file.isEmpty())
	{
		ui->textEncoderEdit->setText(file);
	}
}

void AdvancedSettingsDialog::onDiTBrowseButtonClicked()
{
	QString file = QFileDialog::getOpenFileName(this, "Select DiT Model", ui->ditModelEdit->text(), "GGUF Files (*.gguf)");
	if (!file.isEmpty())
	{
		ui->ditModelEdit->setText(file);
	}
}

void AdvancedSettingsDialog::onVAEBrowseButtonClicked()
{
	QString file = QFileDialog::getOpenFileName(this, "Select VAE Model", ui->vaeModelEdit->text(), "GGUF Files (*.gguf)");
	if (!file.isEmpty())
	{
		ui->vaeModelEdit->setText(file);
	}
}