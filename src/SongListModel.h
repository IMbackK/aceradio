/*
 * Copyright Carl Philipp Klemm 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SONGLISTMODEL_H
#define SONGLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QRandomGenerator>
#include <cstdint>

#include "SongItem.h"

class SongListModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum Roles
	{
		CaptionRole = Qt::UserRole + 1,
		LyricsRole = Qt::UserRole + 2,
		VocalLanguageRole = Qt::UserRole + 3,
		IsPlayingRole = Qt::UserRole + 4
	};

	explicit SongListModel(QObject *parent = nullptr);

	// Basic functionality:
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	// Editable:
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	void updateSong(const QModelIndex &index, const SongItem& song);
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	// Add/remove songs
	void addSong(const SongItem &song);
	void removeSong(int index);
	SongItem getSong(int index) const;
	int findNextIndex(int currentIndex, bool shuffle = false) const;
	void clear();

	// Playing indicator
	void setPlayingIndex(int index);
	int playingIndex() const
	{
		return m_playingIndex;
	}

	// Find song by unique ID
	int findSongIndexById(uint64_t uniqueId) const;
	int songCount();
	bool empty();

private:
	QList<SongItem> songList;
	int m_playingIndex;
};

#endif // SONGLISTMODEL_H
