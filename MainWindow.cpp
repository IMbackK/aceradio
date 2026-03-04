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
      aceStepWorker(new AceStepWorker(this)),
      playbackTimer(new QTimer(this)),
      currentSongIndex(-1),
      isPlaying(false),
      isPaused(false),
      shuffleMode(false),
      isGeneratingNext(false)
{
    ui->setupUi(this);
    
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
    connect(ui->actionQuit, &QAction::triggered, this, [this](){close();});
    connect(audioPlayer, &AudioPlayer::playbackFinished, this, &MainWindow::playNextSong);
    connect(audioPlayer, &AudioPlayer::playbackStarted, this, &MainWindow::playbackStarted);
    connect(aceStepWorker, &AceStepWorker::songGenerated, this, &MainWindow::songGenerated);
    connect(aceStepWorker, &AceStepWorker::generationError, this, &MainWindow::generationError);
    connect(aceStepWorker, &AceStepWorker::progressUpdate, ui->progressBar, &QProgressBar::setValue);
    
    // Connect double-click on song list for editing (works with QTableView too)
    connect(ui->songListView, &QTableView::doubleClicked, this, &MainWindow::on_songListView_doubleClicked);
    
    // Connect audio player error signal
    connect(audioPlayer, &AudioPlayer::playbackError, [this](const QString &error) {
        QMessageBox::warning(this, "Playback Error", "Failed to play audio: " + error);
    });
}

MainWindow::~MainWindow()
{
    // Auto-save playlist before closing
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
    
    // Add some default songs
    SongItem defaultSong1("Upbeat pop rock anthem with driving electric guitars", "");
    SongItem defaultSong2("Chill electronic music with smooth synths and relaxing beats", "");
    SongItem defaultSong3("Jazz fusion with saxophone solos and complex rhythms", "");
    
    songModel->addSong(defaultSong1);
    songModel->addSong(defaultSong2);
    songModel->addSong(defaultSong3);
    
    // Select first item
    if (songModel->rowCount() > 0) {
        QModelIndex firstIndex = songModel->index(0, 0);
        ui->songListView->setCurrentIndex(firstIndex);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("MusicGenerator", "AceStepGUI");
    
    // Load JSON template (default to simple configuration)
    jsonTemplate = settings.value("jsonTemplate", "{\n\t\"inference_steps\": 8,\n\t\"shift\": 3.0,\n\t\"vocal_language\": \"en\"\n}").toString();
    
    // Load shuffle mode
    shuffleMode = settings.value("shuffleMode", false).toBool();
    ui->shuffleButton->setChecked(shuffleMode);
    
    // Load path settings with defaults based on application directory
    QString appDir = QCoreApplication::applicationDirPath();
    aceStepPath = settings.value("aceStepPath", appDir + "/acestep.cpp").toString();
    qwen3ModelPath = settings.value("qwen3ModelPath", appDir + "/acestep.cpp/models/acestep-5Hz-lm-4B-Q8_0.gguf").toString();
    textEncoderModelPath = settings.value("textEncoderModelPath", appDir + "/acestep.cpp/models/Qwen3-Embedding-0.6B-BF16.gguf").toString();
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
    if (milliseconds < 0) return "0:00";
    
    int seconds = milliseconds / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::updatePosition(int position)
{
    if (position < 0) return;
    
    // Update slider and time labels
    ui->positionSlider->setValue(position);
    ui->elapsedTimeLabel->setText(formatTime(position));
}

void MainWindow::updateDuration(int duration)
{
    if (duration <= 0) return;
    
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
    if (isPaused) {
        // Resume playback
        audioPlayer->play();
        isPaused = false;
        updateControls();
        return;
    }
    
    if (isPlaying) {
        audioPlayer->stop();
        isPlaying = false;
        isPaused = false;
        updateControls();
        return;
    }
    
    // Start playback from current song or first song
    int startIndex = ui->songListView->currentIndex().isValid() 
                     ? ui->songListView->currentIndex().row() 
                     : 0;
    
    currentSongIndex = startIndex;
    generateAndPlayNext();
}

void MainWindow::on_pauseButton_clicked()
{
    if (isPlaying && !isPaused) {
        // Pause playback
        audioPlayer->pause();
        isPaused = true;
        updateControls();
    }
}

void MainWindow::on_skipButton_clicked()
{
    if (isPlaying) {
        // Stop current playback and move to next song
        audioPlayer->stop();
        isPaused = false;
        playNextSong();
        
        // After playing the skipped-to song, start generating the next one
        // We'll do this in playNextSong by checking if we're already playing
    }
}

void MainWindow::on_stopButton_clicked()
{
    if (isPlaying) {
        // Stop current playback completely
        audioPlayer->stop();
        isPlaying = false;
        isPaused = false;
        updateControls();
    }
}

void MainWindow::on_shuffleButton_clicked()
{
    shuffleMode = ui->shuffleButton->isChecked();
    updateControls();
}

void MainWindow::on_addSongButton_clicked()
{
    SongDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString caption = dialog.getCaption();
        QString lyrics = dialog.getLyrics();
        
        SongItem newSong(caption, lyrics);
        songModel->addSong(newSong);
        
        // Select the new item
        QModelIndex newIndex = songModel->index(songModel->rowCount() - 1, 0);
        ui->songListView->setCurrentIndex(newIndex);
    }
}

void MainWindow::on_songListView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    
    // Temporarily disconnect the signal to prevent multiple invocations
    // This happens when the dialog closes and triggers another double-click event
    disconnect(ui->songListView, &QTableView::doubleClicked, this, &MainWindow::on_songListView_doubleClicked);
    
    int row = index.row();
    
    // Different behavior based on which column was clicked
    if (index.column() == 0) {
        // Column 0 (play indicator): Stop current playback and play this song
        if (isPlaying) {
            audioPlayer->stop();
        }
        
        // Set this as the current song and generate/play it
        currentSongIndex = row;
        generateAndPlayNext();
    } else if (index.column() == 1) {
        // Column 1 (caption): Edit the song
        SongItem song = songModel->getSong(row);
        
        SongDialog dialog(this, song.caption, song.lyrics);
        
        if (dialog.exec() == QDialog::Accepted) {
            QString caption = dialog.getCaption();
            QString lyrics = dialog.getLyrics();
            
            // Update the model - use column 1 for the song name
            songModel->setData(songModel->index(row, 1), caption, SongListModel::CaptionRole);
            songModel->setData(songModel->index(row, 1), lyrics, SongListModel::LyricsRole);
        }
    }
    
    // Reconnect the signal after dialog is closed
    connect(ui->songListView, &QTableView::doubleClicked, this, &MainWindow::on_songListView_doubleClicked);
}

void MainWindow::on_removeSongButton_clicked()
{
    QModelIndex currentIndex = ui->songListView->currentIndex();
    if (!currentIndex.isValid()) return;
    
    // Get the row from the current selection (works with table view)
    int row = currentIndex.row();
    
    songModel->removeSong(row);

    // Select next item or previous if at end
    int newRow = qMin(row, songModel->rowCount() - 1);
    if (newRow >= 0) {
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
    
    if (dialog.exec() == QDialog::Accepted) {
        // Validate JSON template
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(dialog.getJsonTemplate().toUtf8(), &parseError);
        if (!doc.isObject()) {
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

void MainWindow::generateAndPlayNext()
{
    if (currentSongIndex < 0 || currentSongIndex >= songModel->rowCount()) {
        return;
    }
    
    SongItem song = songModel->getSong(currentSongIndex);
    
    // Show status
    ui->statusLabel->setText("Generating: " + song.caption);
    isPlaying = true;
    isGeneratingNext = false; // Reset the flag when starting a new generation
    updateControls();
    
    // Generate the song with configurable paths
    aceStepWorker->generateSong(song.caption, song.lyrics, jsonTemplate,
                               aceStepPath, qwen3ModelPath,
                               textEncoderModelPath, ditModelPath,
                               vaeModelPath);
}

void MainWindow::startNextSongGeneration()
{
    // Start generating the next song if we're playing and not already generating
    if (isPlaying && !isGeneratingNext) {
        isGeneratingNext = true;
        
        // Find and generate the next song
        int nextIndex = songModel->findNextIndex(currentSongIndex, shuffleMode);
        if (nextIndex >= 0 && nextIndex < songModel->rowCount()) {
            SongItem nextSong = songModel->getSong(nextIndex);
            
            // Generate the next song in the background
            aceStepWorker->generateSong(nextSong.caption, nextSong.lyrics, jsonTemplate,
                                       aceStepPath, qwen3ModelPath,
                                       textEncoderModelPath, ditModelPath,
                                       vaeModelPath);
        }
    }
}

void MainWindow::playbackStarted()
{
    // When playback starts, immediately start generating the next song
    startNextSongGeneration();
}

void MainWindow::highlightCurrentSong()
{
    if (currentSongIndex >= 0 && currentSongIndex < songModel->rowCount()) {
        // Update the model to show play icon for current song
        songModel->setPlayingIndex(currentSongIndex);
    }
}

void MainWindow::songGenerated(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        generationError("Generated file not found: " + filePath);
        return;
    }
    
    // If we're in the middle of playback, this is a pre-generated next song
    if (isPlaying && audioPlayer->isPlaying()) {
        // Store the generated file path for when playback finishes
        nextSongFilePath = filePath;
        return;
    }
    
    ui->statusLabel->setText("");
    
    // Play the generated song
    audioPlayer->play(filePath);
    
    // Highlight the current song in the list
    highlightCurrentSong();
    
    // Connect position and duration updates for the slider
    connect(audioPlayer, &AudioPlayer::positionChanged, this, &MainWindow::updatePosition);
    connect(audioPlayer, &AudioPlayer::durationChanged, this, &MainWindow::updateDuration);
}

void MainWindow::playNextSong()
{
    if (!isPlaying) return;
    
    // Check if we have a pre-generated next song
    if (!nextSongFilePath.isEmpty()) {
        audioPlayer->play(nextSongFilePath);
        nextSongFilePath.clear();
        
        // Update current index to the next song
        int nextIndex = songModel->findNextIndex(currentSongIndex, shuffleMode);
        if (nextIndex >= 0 && nextIndex < songModel->rowCount()) {
            currentSongIndex = nextIndex;
        }
        
        // Highlight the current song and start generating the next one
        highlightCurrentSong();
        startNextSongGeneration();
    } else {
        // Find next song index and generate it
        int nextIndex = songModel->findNextIndex(currentSongIndex, shuffleMode);
        
        if (nextIndex >= 0 && nextIndex < songModel->rowCount()) {
            currentSongIndex = nextIndex;
            generateAndPlayNext();
        } else {
            // No more songs
            isPlaying = false;
            isPaused = false;
            updateControls();
        }
    }
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

void MainWindow::generationFinished()
{
    // Reset the generation flag when a generation completes
    isGeneratingNext = false;
}

void MainWindow::updatePlaybackStatus(bool playing)
{
    isPlaying = playing;
    updateControls();
}

void MainWindow::on_positionSlider_sliderMoved(int position)
{
    if (isPlaying && audioPlayer->isPlaying()) {
        // Seek to the new position when slider is moved
        audioPlayer->setPosition(position);
    }
}

// Playlist save/load methods
void MainWindow::on_actionSavePlaylist()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Playlist", 
                                                     QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/playlist.json",
                                                     "JSON Files (*.json);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        savePlaylist(filePath);
    }
}

void MainWindow::on_actionLoadPlaylist()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load Playlist",
                                                     QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                     "JSON Files (*.json);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        loadPlaylist();
    }
}

void MainWindow::savePlaylist(const QString &filePath)
{
    // Get current songs from the model
    QList<SongItem> songs;
    for (int i = 0; i < songModel->rowCount(); ++i) {
        songs.append(songModel->getSong(i));
    }
    
    savePlaylistToJson(filePath, songs);
}

void MainWindow::loadPlaylist()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load Playlist",
                                                     QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                     "JSON Files (*.json);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        QList<SongItem> songs;
        if (loadPlaylistFromJson(filePath, songs)) {
            // Clear current playlist
            while (songModel->rowCount() > 0) {
                songModel->removeSong(0);
            }
            
            // Add loaded songs
            for (const SongItem &song : songs) {
                songModel->addSong(song);
            }
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
    for (int i = 0; i < songModel->rowCount(); ++i) {
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
    if (QFile::exists(filePath)) {
        QList<SongItem> songs;
        if (loadPlaylistFromJson(filePath, songs)) {
            // Clear default songs and add loaded ones
            while (songModel->rowCount() > 0) {
                songModel->removeSong(0);
            }
            
            for (const SongItem &song : songs) {
                songModel->addSong(song);
            }
        }
    }
}

bool MainWindow::savePlaylistToJson(const QString &filePath, const QList<SongItem> &songs)
{
    QJsonArray songsArray;
    
    for (const SongItem &song : songs) {
        QJsonObject songObj;
        songObj["caption"] = song.caption;
        songObj["lyrics"] = song.lyrics;
        songsArray.append(songObj);
    }
    
    QJsonObject rootObj;
    rootObj["songs"] = songsArray;
    rootObj["version"] = "1.0";
    
    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson();
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for reading:" << filePath;
        return false;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        qWarning() << "JSON root is not an object";
        return false;
    }
    
    QJsonObject rootObj = doc.object();
    
    // Check for version compatibility
    if (rootObj.contains("version") && rootObj["version"].toString() != "1.0") {
        qWarning() << "Unsupported playlist version:" << rootObj["version"].toString();
        return false;
    }
    
    if (!rootObj.contains("songs") || !rootObj["songs"].isArray()) {
        qWarning() << "Invalid playlist format: missing songs array";
        return false;
    }
    
    QJsonArray songsArray = rootObj["songs"].toArray();
    
    for (const QJsonValue &value : songsArray) {
        if (!value.isObject()) continue;
        
        QJsonObject songObj = value.toObject();
        SongItem song;
        
        if (songObj.contains("caption")) {
            song.caption = songObj["caption"].toString();
        }
        
        if (songObj.contains("lyrics")) {
            song.lyrics = songObj["lyrics"].toString();
        }
        
        songs.append(song);
    }
    
    return true;
}
