#ifndef SONGLISTMODEL_H
#define SONGLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>

class SongItem {
public:
    QString caption;
    QString lyrics;
    
    SongItem(const QString &caption = "", const QString &lyrics = "")
        : caption(caption), lyrics(lyrics) {}
};

class SongListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Roles {
        CaptionRole = Qt::UserRole + 1,
        LyricsRole = Qt::UserRole + 2,
        IsPlayingRole = Qt::UserRole + 3
    };
    
    explicit SongListModel(QObject *parent = nullptr);
    
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    
    // Add/remove songs
    void addSong(const SongItem &song);
    void removeSong(int index);
    SongItem getSong(int index) const;
    int findNextIndex(int currentIndex, bool shuffle = false) const;
    
    // Playing indicator
    void setPlayingIndex(int index);
    int playingIndex() const { return m_playingIndex; }
    
private:
    QList<SongItem> songList;
    int m_playingIndex;
};

#endif // SONGLISTMODEL_H
