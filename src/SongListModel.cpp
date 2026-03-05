#include "SongListModel.h"
#include <QApplication>
#include <QTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QFont>
#include <QUuid>

SongListModel::SongListModel(QObject *parent)
    : QAbstractTableModel(parent),
      m_playingIndex(-1)
{
}

int SongListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return songList.size();
}

int SongListModel::columnCount(const QModelIndex &parent) const
{
    // We have 3 columns: play indicator, song name, and vocal language (read-only)
    return 3;
}

QVariant SongListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= songList.size())
        return QVariant();
    
    const SongItem &song = songList[index.row()];
    
    switch (role) {
    case Qt::DisplayRole:
        // Column 0: Play indicator column
        if (index.column() == 0) {
            return index.row() == m_playingIndex ? "▶" : "";
        }
        // Column 1: Song name
        else if (index.column() == 1) {
            return song.caption;
        }
        // Column 2: Vocal language
        else if (index.column() == 2) {
            return !song.vocalLanguage.isEmpty() ? song.vocalLanguage : "--";
        }
        break;
    case Qt::FontRole:
        // Make play indicator bold and larger
        if (index.column() == 0 && index.row() == m_playingIndex) {
            QFont font = QApplication::font();
            font.setBold(true);
            return font;
        }
        break;
    case Qt::TextAlignmentRole:
        // Center align the play indicator
        if (index.column() == 0) {
            return Qt::AlignCenter;
        }
        break;
    case CaptionRole:
        return song.caption;
    case LyricsRole:
        return song.lyrics;
    case VocalLanguageRole:
        return song.vocalLanguage;
    case IsPlayingRole:
        return index.row() == m_playingIndex;
    default:
        return QVariant();
    }
    
    return QVariant();
}

bool SongListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= songList.size())
        return false;
    
    SongItem &song = songList[index.row()];
    
    switch (role) {
    case CaptionRole:
        song.caption = value.toString();
        break;
    case LyricsRole:
        song.lyrics = value.toString();
        break;
    case VocalLanguageRole:
        song.vocalLanguage = value.toString();
        break;
    default:
        return false;
    }
    
    emit dataChanged(index, index, {role});
    return true;
}

Qt::ItemFlags SongListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    
    // Remove ItemIsEditable to prevent inline editing and double-click issues
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SongListModel::addSong(const SongItem &song)
{
    beginInsertRows(QModelIndex(), songList.size(), songList.size());
    songList.append(song);
    endInsertRows();
}

void SongListModel::removeSong(int index)
{
    if (index >= 0 && index < songList.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        songList.removeAt(index);
        endRemoveRows();
    }
}

void SongListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, songList.size()-1);
    songList.clear();
    endRemoveRows();
}

bool SongListModel::empty()
{
    return songList.empty();
}

SongItem SongListModel::getSong(int index) const
{
    if (index >= 0 && index < songList.size()) {
        return songList[index];
    }
    return SongItem();
}

QVariant SongListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        // Hide headers since we don't need column titles
        return QVariant();
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void SongListModel::setPlayingIndex(int index)
{
    int oldPlayingIndex = m_playingIndex;
    m_playingIndex = index;
    
    // Update both the old and new playing indices to trigger UI updates
    if (oldPlayingIndex >= 0 && oldPlayingIndex < songList.size()) {
        emit dataChanged(this->index(oldPlayingIndex, 0), this->index(oldPlayingIndex, 0));
    }
    
    if (index >= 0 && index < songList.size()) {
        emit dataChanged(this->index(index, 0), this->index(index, 0));
    }
}

int SongListModel::songCount()
{
	return songList.count();
}

int SongListModel::findNextIndex(int currentIndex, bool shuffle) const
{
    if (songList.isEmpty())
        return -1;
    
    if (shuffle) {
        // Simple random selection for shuffle mode
        QRandomGenerator generator;
        return generator.bounded(songList.size());
    }
    
    // Sequential playback
    int nextIndex = currentIndex + 1;
    if (nextIndex >= songList.size()) {
        nextIndex = 0; // Loop back to beginning
    }
    
    return nextIndex;
}

int SongListModel::findSongIndexById(uint64_t uniqueId) const
{
    for (int i = 0; i < songList.size(); ++i) {
        if (songList[i].uniqueId == uniqueId) {
            return i;
        }
    }
    return -1; // Song not found
}
