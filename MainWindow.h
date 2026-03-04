#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "SongListModel.h"
#include "AudioPlayer.h"
#include "AceStepWorker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_playButton_clicked();
    void on_pauseButton_clicked();
    void on_skipButton_clicked();
    void on_stopButton_clicked();
    void on_shuffleButton_clicked();
    void on_positionSlider_sliderMoved(int position);
    void updatePosition(int position);
    void updateDuration(int duration);
    void on_addSongButton_clicked();
    void on_removeSongButton_clicked();
    void on_advancedSettingsButton_clicked();
    
    void on_songListView_doubleClicked(const QModelIndex &index);
    
    void songGenerated(const QString &filePath);
    void playNextSong();
    void playbackStarted();
    void updatePlaybackStatus(bool playing);
    void generationFinished();
    void generationError(const QString &error);
    
    void on_actionSavePlaylist();
    void on_actionLoadPlaylist();
    
private:
    void startNextSongGeneration();
    
private:
    Ui::MainWindow *ui;
    SongListModel *songModel;
    AudioPlayer *audioPlayer;
    AceStepWorker *aceStepWorker;
    QTimer *playbackTimer;
    
    QString formatTime(int milliseconds);
    
    int currentSongIndex;
    bool isPlaying;
    bool isPaused;
    bool shuffleMode;
    bool isGeneratingNext;
    QString jsonTemplate;
    
    // Path settings
    QString aceStepPath;
    QString qwen3ModelPath;
    QString textEncoderModelPath;
    QString ditModelPath;
    QString vaeModelPath;
    
    // Pre-generated song file path
    QString nextSongFilePath;
    
private:
    void highlightCurrentSong();
    
    void loadSettings();
    void saveSettings();
    void loadPlaylist();
    void savePlaylist(const QString &filePath);
    void autoSavePlaylist();
    void autoLoadPlaylist();
    
    bool savePlaylistToJson(const QString &filePath, const QList<SongItem> &songs);
    bool loadPlaylistFromJson(const QString &filePath, QList<SongItem> &songs);
    
    void setupUI();
    void updateControls();
    void generateAndPlayNext();
};

#endif // MAINWINDOW_H
