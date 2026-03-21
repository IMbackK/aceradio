/*
 * Copyright Carl Philipp Klemm 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SONGDIALOG_H
#define SONGDIALOG_H

#include <QDialog>
#include <QString>

#include "SongItem.h"

namespace Ui
{
class SongDialog;
}

class SongDialog : public QDialog
{
	Q_OBJECT
	SongItem song;

public:
	explicit SongDialog(QWidget *parent = nullptr, const SongItem& song = SongItem());
	~SongDialog();

	const SongItem& getSong();

private slots:
	void on_okButton_clicked();
	void on_cancelButton_clicked();

private:
	Ui::SongDialog *ui;
};

#endif // SONGDIALOG_H