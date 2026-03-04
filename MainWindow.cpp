#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QTextEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTabWidget>
#include <QLineEdit>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      songModel(new SongListModel(this)),
      audioPlayer(new AudioPlayer(this)),
      aceStepWorker(new AceStepWorker(this)),
      playbackTimer(new QTimer(this)),
      currentSongIndex(-1),
      isPlaying(false),
      shuffleMode(false)
{
    ui->setupUi(this);
    
    // Setup UI
    setupUI();
    
    // Load settings
    loadSettings();
    
    // Connect signals and slots
    connect(ui->actionAdvancedSettings, &QAction::triggered, this, &MainWindow::on_advancedSettingsButton_clicked);
    connect(audioPlayer, &AudioPlayer::playbackFinished, this, &MainWindow::playNextSong);
    connect(aceStepWorker, &AceStepWorker::songGenerated, this, &MainWindow::songGenerated);
    connect(aceStepWorker, &AceStepWorker::generationError, this, &MainWindow::generationError);
    connect(aceStepWorker, &AceStepWorker::progressUpdate, ui->progressBar, &QProgressBar::setValue);
    
    // Connect audio player error signal
    connect(audioPlayer, &AudioPlayer::playbackError, [this](const QString &error) {
        QMessageBox::warning(this, "Playback Error", "Failed to play audio: " + error);
    });
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupUI()
{
    // Setup song list view
    ui->songListView->setModel(songModel);
    
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
    jsonTemplate = settings.value("jsonTemplate", 
        "{\"inference_steps\": 8, \"shift\": 3.0, \"vocal_language\": \"en\"}").toString();
    
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

void MainWindow::updateControls()
{
    bool hasSongs = songModel->rowCount() > 0;
    
    ui->playButton->setEnabled(hasSongs && !isPlaying);
    ui->skipButton->setEnabled(isPlaying);
    ui->stopButton->setEnabled(isPlaying);
    ui->addSongButton->setEnabled(true);
    ui->editSongButton->setEnabled(hasSongs && ui->songListView->currentIndex().isValid());
    ui->removeSongButton->setEnabled(hasSongs && ui->songListView->currentIndex().isValid());
}

void MainWindow::on_playButton_clicked()
{
    if (isPlaying) {
        audioPlayer->stop();
        isPlaying = false;
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

void MainWindow::on_skipButton_clicked()
{
    if (isPlaying) {
        // Stop current playback and move to next song
        audioPlayer->stop();
        playNextSong();
    }
}

void MainWindow::on_stopButton_clicked()
{
    if (isPlaying) {
        // Stop current playback completely
        audioPlayer->stop();
        isPlaying = false;
        ui->statusLabel->setText("Stopped");
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
    bool ok;
    QString caption = QInputDialog::getText(this, "Add Song", "Enter song caption:", QLineEdit::Normal, "", &ok);
    
    if (ok && !caption.isEmpty()) {
        QString lyrics = QInputDialog::getMultiLineText(this, "Add Song", "Enter lyrics (optional):", "", &ok);
        
        if (ok) {
            SongItem newSong(caption, lyrics);
            songModel->addSong(newSong);
            
            // Select the new item
            QModelIndex newIndex = songModel->index(songModel->rowCount() - 1, 0);
            ui->songListView->setCurrentIndex(newIndex);
        }
    }
}

void MainWindow::on_editSongButton_clicked()
{
    QModelIndex currentIndex = ui->songListView->currentIndex();
    if (!currentIndex.isValid()) return;
    
    int row = currentIndex.row();
    SongItem song = songModel->getSong(row);
    
    bool ok;
    QString caption = QInputDialog::getText(this, "Edit Song", "Enter song caption:", QLineEdit::Normal, song.caption, &ok);
    
    if (ok && !caption.isEmpty()) {
        QString lyrics = QInputDialog::getMultiLineText(this, "Edit Song", "Enter lyrics (optional):", song.lyrics, &ok);
        
        if (ok) {
            SongItem editedSong(caption, lyrics);
            // Update the model
            songModel->setData(songModel->index(row, 0), caption, SongListModel::CaptionRole);
            songModel->setData(songModel->index(row, 0), lyrics, SongListModel::LyricsRole);
        }
    }
}

void MainWindow::on_removeSongButton_clicked()
{
    QModelIndex currentIndex = ui->songListView->currentIndex();
    if (!currentIndex.isValid()) return;
    
    int row = currentIndex.row();
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Remove Song", "Are you sure you want to remove this song?",
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        songModel->removeSong(row);
        
        // Select next item or previous if at end
        int newRow = qMin(row, songModel->rowCount() - 1);
        if (newRow >= 0) {
            QModelIndex newIndex = songModel->index(newRow, 0);
            ui->songListView->setCurrentIndex(newIndex);
        }
    }
}

void MainWindow::on_advancedSettingsButton_clicked()
{
    // Create a dialog for advanced settings
    QDialog dialog(this);
    dialog.setWindowTitle("Advanced Settings");
    dialog.resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    // Tab widget for organized settings
    QTabWidget *tabWidget = new QTabWidget(&dialog);
    layout->addWidget(tabWidget);
    
    // JSON Template tab
    QWidget *jsonTab = new QWidget();
    QVBoxLayout *jsonLayout = new QVBoxLayout(jsonTab);
    
    QLabel *jsonLabel = new QLabel("JSON Template for AceStep generation:");
    jsonLabel->setWordWrap(true);
    jsonLayout->addWidget(jsonLabel);
    
    QTextEdit *jsonTemplateEdit = new QTextEdit();
    jsonTemplateEdit->setPlainText(jsonTemplate);
    jsonTemplateEdit->setMinimumHeight(200);
    jsonLayout->addWidget(jsonTemplateEdit);
    
    QLabel *fieldsLabel = new QLabel("Available fields: caption, lyrics, instrumental, bpm, duration, keyscale, timesignature,\nvocal_language, seed, lm_temperature, lm_cfg_scale, lm_top_p, lm_top_k, lm_negative_prompt,\naudio_codes, inference_steps, guidance_scale, shift");
    fieldsLabel->setWordWrap(true);
    jsonLayout->addWidget(fieldsLabel);
    
    tabWidget->addTab(jsonTab, "JSON Template");
    
    // Path Settings tab
    QWidget *pathsTab = new QWidget();
    QFormLayout *pathsLayout = new QFormLayout(pathsTab);
    pathsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    
    QLineEdit *aceStepPathEdit = new QLineEdit(aceStepPath);
    QPushButton *aceStepBrowseBtn = new QPushButton("Browse...");
    connect(aceStepBrowseBtn, &QPushButton::clicked, [this, aceStepPathEdit]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select AceStep Build Directory", aceStepPathEdit->text());
        if (!dir.isEmpty()) {
            aceStepPathEdit->setText(dir);
        }
    });
    
    QHBoxLayout *aceStepLayout = new QHBoxLayout();
    aceStepLayout->addWidget(aceStepPathEdit);
    aceStepLayout->addWidget(aceStepBrowseBtn);
    pathsLayout->addRow("AceStep Path:", aceStepLayout);
    
    QLineEdit *qwen3ModelEdit = new QLineEdit(qwen3ModelPath);
    QPushButton *qwen3BrowseBtn = new QPushButton("Browse...");
    connect(qwen3BrowseBtn, &QPushButton::clicked, [this, qwen3ModelEdit]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Qwen3 Model", qwen3ModelEdit->text(), "GGUF Files (*.gguf)");
        if (!file.isEmpty()) {
            qwen3ModelEdit->setText(file);
        }
    });
    
    QHBoxLayout *qwen3Layout = new QHBoxLayout();
    qwen3Layout->addWidget(qwen3ModelEdit);
    qwen3Layout->addWidget(qwen3BrowseBtn);
    pathsLayout->addRow("Qwen3 Model:", qwen3Layout);
    
    QLineEdit *textEncoderEdit = new QLineEdit(textEncoderModelPath);
    QPushButton *textEncoderBrowseBtn = new QPushButton("Browse...");
    connect(textEncoderBrowseBtn, &QPushButton::clicked, [this, textEncoderEdit]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Text Encoder Model", textEncoderEdit->text(), "GGUF Files (*.gguf)");
        if (!file.isEmpty()) {
            textEncoderEdit->setText(file);
        }
    });
    
    QHBoxLayout *textEncoderLayout = new QHBoxLayout();
    textEncoderLayout->addWidget(textEncoderEdit);
    textEncoderLayout->addWidget(textEncoderBrowseBtn);
    pathsLayout->addRow("Text Encoder Model:", textEncoderLayout);
    
    QLineEdit *ditModelEdit = new QLineEdit(ditModelPath);
    QPushButton *ditModelBrowseBtn = new QPushButton("Browse...");
    connect(ditModelBrowseBtn, &QPushButton::clicked, [this, ditModelEdit]() {
        QString file = QFileDialog::getOpenFileName(this, "Select DiT Model", ditModelEdit->text(), "GGUF Files (*.gguf)");
        if (!file.isEmpty()) {
            ditModelEdit->setText(file);
        }
    });
    
    QHBoxLayout *ditModelLayout = new QHBoxLayout();
    ditModelLayout->addWidget(ditModelEdit);
    ditModelLayout->addWidget(ditModelBrowseBtn);
    pathsLayout->addRow("DiT Model:", ditModelLayout);
    
    QLineEdit *vaeModelEdit = new QLineEdit(vaeModelPath);
    QPushButton *vaeModelBrowseBtn = new QPushButton("Browse...");
    connect(vaeModelBrowseBtn, &QPushButton::clicked, [this, vaeModelEdit]() {
        QString file = QFileDialog::getOpenFileName(this, "Select VAE Model", vaeModelEdit->text(), "GGUF Files (*.gguf)");
        if (!file.isEmpty()) {
            vaeModelEdit->setText(file);
        }
    });
    
    QHBoxLayout *vaeModelLayout = new QHBoxLayout();
    vaeModelLayout->addWidget(vaeModelEdit);
    vaeModelLayout->addWidget(vaeModelBrowseBtn);
    pathsLayout->addRow("VAE Model:", vaeModelLayout);
    
    tabWidget->addTab(pathsTab, "Model Paths");
    
    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, [&dialog, this, jsonTemplateEdit, aceStepPathEdit, qwen3ModelEdit, textEncoderEdit, ditModelEdit, vaeModelEdit]() {
        // Validate JSON template
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonTemplateEdit->toPlainText().toUtf8(), &parseError);
        if (!doc.isObject()) {
            QMessageBox::warning(this, "Invalid JSON", "Please enter valid JSON: " + QString(parseError.errorString()));
            return;
        }
        
        // Update settings
        jsonTemplate = jsonTemplateEdit->toPlainText();
        aceStepPath = aceStepPathEdit->text();
        qwen3ModelPath = qwen3ModelEdit->text();
        textEncoderModelPath = textEncoderEdit->text();
        ditModelPath = ditModelEdit->text();
        vaeModelPath = vaeModelEdit->text();
        
        saveSettings();
        dialog.accept();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    // Show the dialog
    if (dialog.exec() == QDialog::Accepted) {
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
    updateControls();
    
    // Generate the song with configurable paths
    aceStepWorker->generateSong(song.caption, song.lyrics, jsonTemplate,
                               aceStepPath, qwen3ModelPath,
                               textEncoderModelPath, ditModelPath,
                               vaeModelPath);
}

void MainWindow::songGenerated(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        generationError("Generated file not found: " + filePath);
        return;
    }
    
    ui->statusLabel->setText("Playing: " + QFileInfo(filePath).baseName());
    
    // Play the generated song
    audioPlayer->play(filePath);
}

void MainWindow::playNextSong()
{
    if (!isPlaying) return;
    
    // Find next song index
    int nextIndex = songModel->findNextIndex(currentSongIndex, shuffleMode);
    
    if (nextIndex >= 0 && nextIndex < songModel->rowCount()) {
        currentSongIndex = nextIndex;
        generateAndPlayNext();
    } else {
        // No more songs
        isPlaying = false;
        ui->statusLabel->setText("Finished playback");
        updateControls();
    }
}

void MainWindow::generationError(const QString &error)
{
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
    updateControls();
}

void MainWindow::generationFinished()
{
    // This slot is declared but not used in the current implementation
    // It's here for potential future use
}

void MainWindow::updatePlaybackStatus(bool playing)
{
    isPlaying = playing;
    updateControls();
}
