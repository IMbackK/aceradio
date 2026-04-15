/*
 * Copyright Carl Philipp Klemm 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ADVANCEDSETTINGSDIALOG_H
#define ADVANCEDSETTINGSDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui
{
class AdvancedSettingsDialog;
}

class AdvancedSettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AdvancedSettingsDialog(QWidget *parent = nullptr);
	~AdvancedSettingsDialog();

	// Getters for settings
	QString getJsonTemplate() const;
	QString getQwen3ModelPath() const;
	QString getTextEncoderModelPath() const;
	QString getDiTModelPath() const;
	QString getVAEModelPath() const;
	bool getLowVramMode() const;
	bool getFlashAttention() const;

	// Setters for settings
	void setJsonTemplate(const QString &templateStr);
	void setQwen3ModelPath(const QString &path);
	void setTextEncoderModelPath(const QString &path);
	void setDiTModelPath(const QString &path);
	void setVAEModelPath(const QString &path);
	void setLowVramMode(bool enabled);
	void setFlashAttention(bool enabled);

private slots:
	void on_qwen3BrowseButton_clicked();
	void on_textEncoderBrowseButton_clicked();
	void on_ditBrowseButton_clicked();
	void on_vaeBrowseButton_clicked();

private:
	Ui::AdvancedSettingsDialog *ui;
};

#endif // ADVANCEDSETTINGSDIALOG_H