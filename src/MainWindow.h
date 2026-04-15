/*
 * Copyright Carl Philipp Klemm 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QQueue>
#include <QPair>
#include <QThread>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "SongListModel.h"
#include "AudioPlayer.h"
#include "AceStepWorker.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

	Ui::MainWindow *ui;
	SongListModel *songModel;
	AudioPlayer *audioPlayer;
	QThread aceThread;
	AceStepWorker *aceStep;
	QTimer *playbackTimer;

	QString formatTime(int milliseconds);

	SongItem currentSong;
	bool isPlaying;
	bool isPaused;
	bool shuffleMode;
	bool isGeneratingNext;
	bool isFirstRun;
	QString jsonTemplate;

	// Path settings
	QString qwen3ModelPath;
	QString textEncoderModelPath;
	QString ditModelPath;
	QString vaeModelPath;

	// Queue for generated songs
	static constexpr int generationTresh = 2;
	QQueue<SongItem> generatedSongQueue;

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

public slots:
	void show();

private slots:
	void onPlayButtonClicked();
	void onPauseButtonClicked();
	void onSkipButtonClicked();
	void onStopButtonClicked();
	void onShuffleButtonClicked();
	void onPositionSliderSliderMoved(int position);
	void onVolumeSliderValueChanged(int value);
	void updatePosition(int position);
	void updateDuration(int duration);
	void onAddSongButtonClicked();
	void onRemoveSongButtonClicked();
	void onAdvancedSettingsButtonClicked();

	void onSongListViewDoubleClicked(const QModelIndex &index);

	void songGenerated(const SongItem& song);
	void generationCanceld(const SongItem& song);
	void playNextSong();
	void playbackStarted();
	void updatePlaybackStatus(bool playing);
	void generationError(const QString &error);

	void onActionSavePlaylist();
	void onActionLoadPlaylist();
	void onActionAppendPlaylist();
	void onActionSaveSong();

private:
	void loadSettings();
	void saveSettings();
	void loadPlaylist(const QString &filePath);
	void savePlaylist(const QString &filePath);
	void autoSavePlaylist();
	void autoLoadPlaylist();

	void playSong(const SongItem& song);

	bool savePlaylistToJson(const QString &filePath, const QList<SongItem> &songs);
	bool loadPlaylistFromJson(const QString &filePath, QList<SongItem> &songs);

	void setupUI();
	void updateControls();
	void ensureSongsInQueue(bool enqeueCurrent = false);
	void flushGenerationQueue();
};

#endif // MAINWINDOW_H
