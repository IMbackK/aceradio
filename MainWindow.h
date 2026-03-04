#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <QTimer>
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
    void on_skipButton_clicked();
    void on_stopButton_clicked();
    void on_shuffleButton_clicked();
    void on_addSongButton_clicked();
    void on_editSongButton_clicked();
    void on_removeSongButton_clicked();
    void on_advancedSettingsButton_clicked();
    
    void songGenerated(const QString &filePath);
    void playNextSong();
    void updatePlaybackStatus(bool playing);
    void generationFinished();
    void generationError(const QString &error);
    
private:
    Ui::MainWindow *ui;
    SongListModel *songModel;
    AudioPlayer *audioPlayer;
    AceStepWorker *aceStepWorker;
    QTimer *playbackTimer;
    
    int currentSongIndex;
    bool isPlaying;
    bool shuffleMode;
    QString jsonTemplate;
    
    // Path settings
    QString aceStepPath;
    QString qwen3ModelPath;
    QString textEncoderModelPath;
    QString ditModelPath;
    QString vaeModelPath;
    
    void loadSettings();
    void saveSettings();
    void setupUI();
    void updateControls();
    void generateAndPlayNext();
};

#endif // MAINWINDOW_H
