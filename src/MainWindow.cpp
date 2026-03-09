#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SongDialog.h"
#include "AdvancedSettingsDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFile>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	  ui(new Ui::MainWindow),
	  songModel(new SongListModel(this)),
	  audioPlayer(new AudioPlayer(this)),
	  aceStep(new AceStep(this)),
	  playbackTimer(new QTimer(this)),
	  isPlaying(false),
	  isPaused(false),
	  shuffleMode(false),
	  isGeneratingNext(false)
{
	aceStep->moveToThread(&aceThread);

	ui->setupUi(this);

	// Setup lyrics display
	ui->lyricsTextEdit->setReadOnly(true);

	// Setup UI
	setupUI();

	// Load settings
	loadSettings();

	// Auto-load playlist from config location on startup
	autoLoadPlaylist();

	// Connect signals and slots
	connect(ui->actionAdvancedSettings, &QAction::triggered, this, &MainWindow::on_advancedSettingsButton_clicked);
	connect(ui->actionSavePlaylist, &QAction::triggered, this, &MainWindow::on_actionSavePlaylist);
	connect(ui->actionLoadPlaylist, &QAction::triggered, this, &MainWindow::on_actionLoadPlaylist);
	connect(ui->actionAppendPlaylist, &QAction::triggered, this, &MainWindow::on_actionAppendPlaylist);
	connect(ui->actionSaveSong, &QAction::triggered, this, &MainWindow::on_actionSaveSong);
	connect(ui->actionQuit, &QAction::triggered, this, [this]()
	{
		close();
	});
	connect(ui->actionClearPlaylist, &QAction::triggered, this, [this]()
	{
		songModel->clear();
	});
	connect(audioPlayer, &AudioPlayer::playbackFinished, this, &MainWindow::playNextSong);
	connect(audioPlayer, &AudioPlayer::playbackStarted, this, &MainWindow::playbackStarted);
	connect(audioPlayer, &AudioPlayer::positionChanged, this, &MainWindow::updatePosition);
	connect(audioPlayer, &AudioPlayer::durationChanged, this, &MainWindow::updateDuration);
	connect(aceStep, &AceStep::songGenerated, this, &MainWindow::songGenerated);
	connect(aceStep, &AceStep::generationCancled, this, &MainWindow::generationCanceld);
	connect(aceStep, &AceStep::generationError, this, &MainWindow::generationError);
	connect(aceStep, &AceStep::progressUpdate, ui->progressBar, &QProgressBar::setValue);

	// Connect double-click on song list for editing (works with QTableView too)
	connect(ui->songListView, &QTableView::doubleClicked, this, &MainWindow::on_songListView_doubleClicked);

	// Connect audio player error signal
	connect(audioPlayer, &AudioPlayer::playbackError, [this](const QString &error)
	{
		QMessageBox::warning(this, "Playback Error", "Failed to play audio: " + error);
	});

	// Add some default songs
	if(songModel->songCount() == 0)
	{
		SongItem defaultSong1("Upbeat pop rock anthem with driving electric guitars", "");
		SongItem defaultSong2("Chill electronic music with smooth synths and relaxing beats", "");
		SongItem defaultSong3("Jazz fusion with saxophone solos and complex rhythms", "");

		songModel->addSong(defaultSong1);
		songModel->addSong(defaultSong2);
		songModel->addSong(defaultSong3);
	}

	// Select first item
	if (songModel->rowCount() > 0)
	{
		QModelIndex firstIndex = songModel->index(0, 0);
		ui->songListView->setCurrentIndex(firstIndex);
	}

	ui->nowPlayingLabel->setText("Now Playing:");

	currentSong = songModel->getSong(0);
}

MainWindow::~MainWindow()
{
	aceStep->cancleGenerateion();

	autoSavePlaylist();
	saveSettings();
	delete ui;
}

void MainWindow::setupUI()
{
	// Setup song list view
	ui->songListView->setModel(songModel);

	// Make sure the table view is read-only (no inline editing)
	ui->songListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// Hide headers for cleaner appearance
	ui->songListView->horizontalHeader()->hide();
	ui->songListView->verticalHeader()->hide();

	// Configure column sizes
	ui->songListView->setColumnWidth(0, 40); // Fixed width for play indicator column
	ui->songListView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Expand caption column

	// Enable row selection and disable column selection
	ui->songListView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void MainWindow::loadSettings()
{
	QSettings settings("MusicGenerator", "AceStepGUI");

	// Load JSON template (default to simple configuration)
	jsonTemplate = settings.value("jsonTemplate",
	                              "{\n\t\"inference_steps\": 8,\n\t\"shift\": 3.0,\n\t\"vocal_language\": \"en\"\n}").toString();

	// Load shuffle mode
	shuffleMode = settings.value("shuffleMode", false).toBool();
	ui->shuffleButton->setChecked(shuffleMode);

	// Load path settings with defaults based on application directory
	QString appDir = QCoreApplication::applicationDirPath();
	aceStepPath = settings.value("aceStepPath", appDir + "/acestep.cpp").toString();
	qwen3ModelPath = settings.value("qwen3ModelPath",
	                                appDir + "/acestep.cpp/models/acestep-5Hz-lm-4B-Q8_0.gguf").toString();
	textEncoderModelPath = settings.value("textEncoderModelPath",
	                                      appDir + "/acestep.cpp/models/Qwen3-Embedding-0.6B-BF16.gguf").toString();
	ditModelPath = settings.value("ditModelPath", appDir + "/acestep.cpp/models/acestep-v15-turbo-Q8_0.gguf").toString();
	vaeModelPath = settings.value("vaeModelPath", appDir + "/acestep.cpp/models/vae-BF16.gguf").toString();
}

void MainWindow::saveSettings()
{
	QSettings settings("MusicGenerator", "AceStepGUI");

	// Save JSON template
	settings.setValue("jsonTemplate", jsonTemplate);

	// Save shuffle mode
	settings.setValue("shuffleMode", shuffleMode);

	// Save path settings
	settings.setValue("aceStepPath", aceStepPath);
	settings.setValue("qwen3ModelPath", qwen3ModelPath);
	settings.setValue("textEncoderModelPath", textEncoderModelPath);
	settings.setValue("ditModelPath", ditModelPath);
	settings.setValue("vaeModelPath", vaeModelPath);
}

QString MainWindow::formatTime(int milliseconds)
{
	if (milliseconds < 0)
		return "0:00";

	int seconds = milliseconds / 1000;
	int minutes = seconds / 60;
	seconds = seconds % 60;

	return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::updatePosition(int position)
{
	if (position < 0)
		return;

	// Update slider and time labels
	ui->positionSlider->setValue(position);
	ui->elapsedTimeLabel->setText(formatTime(position));
}

void MainWindow::updateDuration(int duration)
{
	if (duration <= 0)
		return;

	// Set slider range and update duration label
	ui->positionSlider->setRange(0, duration);
	ui->durationLabel->setText(formatTime(duration));
}

void MainWindow::updateControls()
{
	bool hasSongs = songModel->rowCount() > 0;

	// Play button is enabled when not playing, or can be used to resume when paused
	ui->playButton->setEnabled(hasSongs && (!isPlaying || isPaused));
	ui->pauseButton->setEnabled(isPlaying && !isPaused);
	ui->skipButton->setEnabled(isPlaying);
	ui->stopButton->setEnabled(isPlaying);
	ui->addSongButton->setEnabled(true);
	ui->removeSongButton->setEnabled(hasSongs && ui->songListView->currentIndex().isValid());
}

void MainWindow::on_playButton_clicked()
{
	if (isPaused)
	{
		// Resume playback
		audioPlayer->play();
		isPaused = false;
		updateControls();
		return;
	}

	if(songModel->empty())
		return;

	isPlaying = true;
	ui->nowPlayingLabel->setText("Now Playing: Waiting for generation...");
	flushGenerationQueue();
	ensureSongsInQueue(true);
	updateControls();
}

void MainWindow::on_pauseButton_clicked()
{
	if (isPlaying && !isPaused)
	{
		// Pause playback
		audioPlayer->pause();
		isPaused = true;
		updateControls();
	}
}

void MainWindow::on_skipButton_clicked()
{
	if (isPlaying)
	{
		audioPlayer->stop();
		isPaused = false;
		playNextSong();
	}
}

void MainWindow::on_stopButton_clicked()
{
	if (isPlaying)
	{
		// Stop current playback completely
		audioPlayer->stop();
		ui->nowPlayingLabel->setText("Now Playing:");
		isPlaying = false;
		isPaused = false;
		updateControls();
		flushGenerationQueue();
	}
}

void MainWindow::on_shuffleButton_clicked()
{
	shuffleMode = ui->shuffleButton->isChecked();
	updateControls();

	flushGenerationQueue();
	if(isPlaying)
		ensureSongsInQueue();
}

void MainWindow::on_addSongButton_clicked()
{
	SongDialog dialog(this);

	if (dialog.exec() == QDialog::Accepted)
	{
		QString caption = dialog.getCaption();
		QString lyrics = dialog.getLyrics();
		QString vocalLanguage = dialog.getVocalLanguage();

		SongItem newSong(caption, lyrics);
		newSong.vocalLanguage = vocalLanguage;
		songModel->addSong(newSong);

		// Select the new item
		QModelIndex newIndex = songModel->index(songModel->rowCount() - 1, 0);
		ui->songListView->setCurrentIndex(newIndex);
	}
}

void MainWindow::on_songListView_doubleClicked(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	disconnect(ui->songListView, &QTableView::doubleClicked, this, &MainWindow::on_songListView_doubleClicked);

	int row = index.row();

	if (index.column() == 0)
	{
		if (isPlaying)
		{
			audioPlayer->stop();
		}
		else
		{
			isPlaying = true;
			updateControls();
		}

		flushGenerationQueue();
		ui->nowPlayingLabel->setText("Now Playing: Waiting for generation...");
		currentSong = songModel->getSong(row);
		ensureSongsInQueue(true);
	}
	else if (index.column() == 1 || index.column() == 2)
	{
		SongItem song = songModel->getSong(row);

		SongDialog dialog(this, song.caption, song.lyrics, song.vocalLanguage);

		if (dialog.exec() == QDialog::Accepted)
		{
			QString caption = dialog.getCaption();
			QString lyrics = dialog.getLyrics();
			QString vocalLanguage = dialog.getVocalLanguage();

			songModel->setData(songModel->index(row, 1), caption, SongListModel::CaptionRole);
			songModel->setData(songModel->index(row, 1), lyrics, SongListModel::LyricsRole);
			songModel->setData(songModel->index(row, 1), vocalLanguage, SongListModel::VocalLanguageRole);
		}
	}

	connect(ui->songListView, &QTableView::doubleClicked, this, &MainWindow::on_songListView_doubleClicked);
}

void MainWindow::on_removeSongButton_clicked()
{
	QModelIndex currentIndex = ui->songListView->currentIndex();
	if (!currentIndex.isValid())
		return;

	// Get the row from the current selection (works with table view)
	int row = currentIndex.row();

	songModel->removeSong(row);

	// Select next item or previous if at end
	int newRow = qMin(row, songModel->rowCount() - 1);
	if (newRow >= 0)
	{
		QModelIndex newIndex = songModel->index(newRow, 0);
		ui->songListView->setCurrentIndex(newIndex);
	}
}

void MainWindow::on_advancedSettingsButton_clicked()
{
	AdvancedSettingsDialog dialog(this);

	// Set current values
	dialog.setJsonTemplate(jsonTemplate);
	dialog.setAceStepPath(aceStepPath);
	dialog.setQwen3ModelPath(qwen3ModelPath);
	dialog.setTextEncoderModelPath(textEncoderModelPath);
	dialog.setDiTModelPath(ditModelPath);
	dialog.setVAEModelPath(vaeModelPath);

	if (dialog.exec() == QDialog::Accepted)
	{
		// Validate JSON template
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(dialog.getJsonTemplate().toUtf8(), &parseError);
		if (!doc.isObject())
		{
			QMessageBox::warning(this, "Invalid JSON", "Please enter valid JSON: " + QString(parseError.errorString()));
			return;
		}

		// Update settings
		jsonTemplate = dialog.getJsonTemplate();
		aceStepPath = dialog.getAceStepPath();
		qwen3ModelPath = dialog.getQwen3ModelPath();
		textEncoderModelPath = dialog.getTextEncoderModelPath();
		ditModelPath = dialog.getDiTModelPath();
		vaeModelPath = dialog.getVAEModelPath();

		saveSettings();
		QMessageBox::information(this, "Settings Saved", "Advanced settings have been saved successfully.");
	}
}

void MainWindow::playbackStarted()
{
	ensureSongsInQueue();
}

void MainWindow::playSong(const SongItem& song)
{
	currentSong = song;
	audioPlayer->play(song.file);
	songModel->setPlayingIndex(songModel->findSongIndexById(song.uniqueId));
	ui->nowPlayingLabel->setText("Now Playing: " + song.caption);
	ui->lyricsTextEdit->setPlainText(song.lyrics);
	ui->jsonTextEdit->setPlainText(song.json);
}

void MainWindow::songGenerated(const SongItem& song)
{
	isGeneratingNext = false;

	if (!isPaused && isPlaying && !audioPlayer->isPlaying())
	{
		playSong(song);
	}
	else
	{
		generatedSongQueue.enqueue(song);
	}
	ui->statusbar->showMessage("idle");

	ensureSongsInQueue();
}

void MainWindow::generationCanceld(const SongItem& song)
{
	ui->statusbar->showMessage("Geneartion cancled: " + song.caption);
}

void MainWindow::playNextSong()
{
	if (!isPlaying)
		return;

	// Check if we have a pre-generated next song in the queue
	if (!generatedSongQueue.isEmpty())
	{
		SongItem generatedSong = generatedSongQueue.dequeue();
		playSong(generatedSong);
	}
	else
	{
		ui->nowPlayingLabel->setText("Now Playing: Waiting for generation...");
	}

	// Ensure we have songs in the queue for smooth playback
	ensureSongsInQueue();
}

void MainWindow::generationError(const QString &error)
{
	// Reset the generation flag on error
	isGeneratingNext = false;

	// Show detailed error in a dialog with QTextEdit
	QDialog dialog(this);
	dialog.setWindowTitle("Generation Error");
	dialog.resize(600, 400);

	QVBoxLayout *layout = new QVBoxLayout(&dialog);

	QLabel *errorLabel = new QLabel("Error occurred during music generation:");
	errorLabel->setStyleSheet("font-weight: bold; color: darkred;");
	layout->addWidget(errorLabel);

	QTextEdit *errorTextEdit = new QTextEdit();
	errorTextEdit->setReadOnly(true);
	errorTextEdit->setPlainText(error);
	errorTextEdit->setLineWrapMode(QTextEdit::NoWrap);
	errorTextEdit->setFontFamily("Monospace");
	layout->addWidget(errorTextEdit);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

	dialog.exec();

	isPlaying = false;
	isPaused = false;
	updateControls();
}

void MainWindow::updatePlaybackStatus(bool playing)
{
	isPlaying = playing;
	updateControls();
}

void MainWindow::on_positionSlider_sliderMoved(int position)
{
	if (isPlaying && audioPlayer->isPlaying())
	{
		audioPlayer->setPosition(position);
	}
}

void MainWindow::ensureSongsInQueue(bool enqeueCurrent)
{
	// Only generate more songs if we're playing and not already at capacity
	if (!isPlaying || isGeneratingNext || generatedSongQueue.size() >= generationTresh)
	{
		return;
	}

	SongItem lastSong;
	SongItem workerSong;
	if(aceStep->isGenerateing(&workerSong))
		lastSong = workerSong;
	else if(!generatedSongQueue.empty())
		lastSong = generatedSongQueue.last();
	else
		lastSong = currentSong;

	SongItem nextSong;
	if(enqeueCurrent)
	{
		nextSong = lastSong;
	}
	else
	{
		int nextIndex = songModel->findNextIndex(songModel->findSongIndexById(lastSong.uniqueId), shuffleMode);
		nextSong = songModel->getSong(nextIndex);
	}

	isGeneratingNext = true;

	ui->statusbar->showMessage("Generateing: "+nextSong.caption);
	aceStep->requestGeneration(nextSong, jsonTemplate,
	                            aceStepPath, qwen3ModelPath,
	                            textEncoderModelPath, ditModelPath,
	                            vaeModelPath);
}

void MainWindow::flushGenerationQueue()
{
	generatedSongQueue.clear();
	aceStep->cancleGenerateion();
	isGeneratingNext = false;
}

// Playlist save/load methods
void MainWindow::on_actionSavePlaylist()
{
	QString filePath = QFileDialog::getSaveFileName(this, "Save Playlist",
	                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/playlist.json",
	                   "JSON Files (*.json);;All Files (*)");

	if (!filePath.isEmpty())
	{
		savePlaylist(filePath);
	}
}

void MainWindow::on_actionLoadPlaylist()
{
	QString filePath = QFileDialog::getOpenFileName(this, "Load Playlist",
	                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
	                   "JSON Files (*.json);;All Files (*)");
	if (!filePath.isEmpty())
	{
		songModel->clear();
		flushGenerationQueue();
		loadPlaylist(filePath);
	}
}

void MainWindow::on_actionAppendPlaylist()
{
	QString filePath = QFileDialog::getOpenFileName(this, "Load Playlist",
	                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
	                   "JSON Files (*.json);;All Files (*)");
	if (!filePath.isEmpty())
	{
		loadPlaylist(filePath);
	}
}

void MainWindow::on_actionSaveSong()
{
	QString filePath = QFileDialog::getSaveFileName(this, "Save Playlist",
	                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/song.json",
	                   "JSON Files (*.json);;All Files (*)");
	if (!filePath.isEmpty())
	{
		QJsonArray songsArray;
		QJsonParseError parseError;
		QJsonDocument songDoc = QJsonDocument::fromJson(currentSong.json.toUtf8(), &parseError);
		if(parseError.error)
			return;
		songsArray.append(songDoc.object());

		QJsonObject rootObj;
		rootObj["songs"] = songsArray;
		rootObj["version"] = "1.0";

		QJsonDocument doc(rootObj);
		QByteArray jsonData = doc.toJson();

		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return;
		QFile::copy(currentSong.file, filePath + ".wav");
		file.write(jsonData);
		file.close();
	}
}

void MainWindow::savePlaylist(const QString &filePath)
{
	// Get current songs from the model
	QList<SongItem> songs;
	for (int i = 0; i < songModel->rowCount(); ++i)
	{
		songs.append(songModel->getSong(i));
	}

	savePlaylistToJson(filePath, songs);
}

void MainWindow::loadPlaylist(const QString& filePath)
{
	QList<SongItem> songs;
	if (loadPlaylistFromJson(filePath, songs))
	{
		// Add loaded songs
		for (const SongItem &song : songs)
		{
			songModel->addSong(song);
		}
	}
}

void MainWindow::autoSavePlaylist()
{
	QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	QString appConfigPath = configPath + "/MusicGenerator/AceStepGUI";

	// Create directory if it doesn't exist
	QDir().mkpath(appConfigPath);

	QString filePath = appConfigPath + "/playlist.json";

	// Get current songs from the model
	QList<SongItem> songs;
	for (int i = 0; i < songModel->rowCount(); ++i)
	{
		songs.append(songModel->getSong(i));
	}

	savePlaylistToJson(filePath, songs);
}

void MainWindow::autoLoadPlaylist()
{
	QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	QString appConfigPath = configPath + "/MusicGenerator/AceStepGUI";
	QString filePath = appConfigPath + "/playlist.json";

	// Check if the auto-save file exists
	if (QFile::exists(filePath))
	{
		QList<SongItem> songs;
		if (loadPlaylistFromJson(filePath, songs))
		{
			songModel->clear();
			for (const SongItem &song : songs)
				songModel->addSong(song);
		}
	}
}

bool MainWindow::savePlaylistToJson(const QString &filePath, const QList<SongItem> &songs)
{
	QJsonArray songsArray;

	for (const SongItem &song : songs)
	{
		QJsonObject songObj;
		songObj["caption"] = song.caption;
		songObj["lyrics"] = song.lyrics;
		songObj["vocalLanguage"] = song.vocalLanguage;
		songObj["uniqueId"] = static_cast<qint64>(song.uniqueId); // Store as qint64 for JSON compatibility
		songsArray.append(songObj);
	}

	QJsonObject rootObj;
	rootObj["songs"] = songsArray;
	rootObj["version"] = "1.0";

	QJsonDocument doc(rootObj);
	QByteArray jsonData = doc.toJson();

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "Could not open file for writing:" << filePath;
		return false;
	}

	file.write(jsonData);
	file.close();

	return true;
}

bool MainWindow::loadPlaylistFromJson(const QString &filePath, QList<SongItem> &songs)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Could not open file for reading:" << filePath;
		return false;
	}

	qDebug()<<"Loading from"<<filePath;

	QByteArray jsonData = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

	if (parseError.error != QJsonParseError::NoError)
	{
		qWarning() << "JSON parse error:" << parseError.errorString();
		return false;
	}

	if (!doc.isObject())
	{
		qWarning() << "JSON root is not an object";
		return false;
	}

	QJsonObject rootObj = doc.object();

	// Check for version compatibility
	if (rootObj.contains("version") && rootObj["version"].toString() != "1.0")
	{
		qWarning() << "Unsupported playlist version:" << rootObj["version"].toString();
		return false;
	}

	if (!rootObj.contains("songs") || !rootObj["songs"].isArray())
	{
		qWarning() << "Invalid playlist format: missing songs array";
		return false;
	}

	QJsonArray songsArray = rootObj["songs"].toArray();

	qDebug()<<"Loading"<<songsArray.size()<<"songs";

	for (const QJsonValue &value : songsArray)
	{
		if (!value.isObject())
			continue;

		QJsonObject songObj = value.toObject();
		SongItem song;

		if (songObj.contains("caption"))
		{
			song.caption = songObj["caption"].toString();
		}

		if (songObj.contains("lyrics"))
		{
			song.lyrics = songObj["lyrics"].toString();
		}

		// Load vocalLanguage if present
		if (songObj.contains("vocalLanguage"))
		{
			song.vocalLanguage = songObj["vocalLanguage"].toString();
		}

		// Load uniqueId if present (for backward compatibility)
		if (songObj.contains("uniqueId"))
		{
			song.uniqueId = static_cast<uint64_t>(songObj["uniqueId"].toInteger());
		}
		else
		{
			// Generate new ID for old playlists without uniqueId
			song.uniqueId = QRandomGenerator::global()->generate64();
		}

		songs.append(song);
	}

	return true;
}
