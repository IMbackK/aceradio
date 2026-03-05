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
}

AdvancedSettingsDialog::~AdvancedSettingsDialog()
{
    delete ui;
}

QString AdvancedSettingsDialog::getJsonTemplate() const
{
    return ui->jsonTemplateEdit->toPlainText();
}

QString AdvancedSettingsDialog::getAceStepPath() const
{
    return ui->aceStepPathEdit->text();
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

void AdvancedSettingsDialog::setJsonTemplate(const QString &templateStr)
{
    ui->jsonTemplateEdit->setPlainText(templateStr);
}

void AdvancedSettingsDialog::setAceStepPath(const QString &path)
{
    ui->aceStepPathEdit->setText(path);
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

void AdvancedSettingsDialog::on_aceStepBrowseButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select AceStep Build Directory", ui->aceStepPathEdit->text());
    if (!dir.isEmpty()) {
        ui->aceStepPathEdit->setText(dir);
    }
}

void AdvancedSettingsDialog::on_qwen3BrowseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Qwen3 Model", ui->qwen3ModelEdit->text(), "GGUF Files (*.gguf)");
    if (!file.isEmpty()) {
        ui->qwen3ModelEdit->setText(file);
    }
}

void AdvancedSettingsDialog::on_textEncoderBrowseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Text Encoder Model", ui->textEncoderEdit->text(), "GGUF Files (*.gguf)");
    if (!file.isEmpty()) {
        ui->textEncoderEdit->setText(file);
    }
}

void AdvancedSettingsDialog::on_ditBrowseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select DiT Model", ui->ditModelEdit->text(), "GGUF Files (*.gguf)");
    if (!file.isEmpty()) {
        ui->ditModelEdit->setText(file);
    }
}

void AdvancedSettingsDialog::on_vaeBrowseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select VAE Model", ui->vaeModelEdit->text(), "GGUF Files (*.gguf)");
    if (!file.isEmpty()) {
        ui->vaeModelEdit->setText(file);
    }
}