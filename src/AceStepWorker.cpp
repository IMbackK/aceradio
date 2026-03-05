#include "AceStepWorker.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <cstdint>

AceStepWorker::AceStepWorker(QObject *parent)
    : QObject(parent),
      currentWorker(nullptr)
{
}

AceStepWorker::~AceStepWorker()
{
    cancelGeneration();
}

void AceStepWorker::generateSong(const SongItem& song, const QString &jsonTemplate,
                                const QString &aceStepPath, const QString &qwen3ModelPath,
                                const QString &textEncoderModelPath, const QString &ditModelPath,
                                const QString &vaeModelPath)
{
    // Cancel any ongoing generation
    cancelGeneration();
    
    // Create worker and start it
    currentWorker = new Worker(this, song, jsonTemplate, aceStepPath, qwen3ModelPath,
                               textEncoderModelPath, ditModelPath, vaeModelPath);
	currentWorker->setAutoDelete(true);
    QThreadPool::globalInstance()->start(currentWorker);
}

void AceStepWorker::cancelGeneration()
{
    currentWorker = nullptr;
}

bool AceStepWorker::songGenerateing(SongItem* song)
{
	workerMutex.lock();
	if(!currentWorker) {
		workerMutex.unlock();
		return false;
	}
	else {
		SongItem workerSong = currentWorker->getSong();
		workerMutex.unlock();
		if(song)
			*song = workerSong;
		return true;
	}
}

// Worker implementation
void AceStepWorker::Worker::run()
{
    uint64_t uid = QRandomGenerator::global()->generate();
    // Create temporary JSON file for the request
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString requestFile = tempDir + "/request_" + QString::number(uid) + ".json";
    
    // Parse and modify the template
    QJsonParseError parseError;
    QJsonDocument templateDoc = QJsonDocument::fromJson(jsonTemplate.toUtf8(), &parseError);
    if (!templateDoc.isObject()) {
        emit parent->generationError("Invalid JSON template: " + QString(parseError.errorString()));
        return;
    }
    
    QJsonObject requestObj = templateDoc.object();
	requestObj["caption"] = song.caption;
    
	if (!song.lyrics.isEmpty()) {
		requestObj["lyrics"] = song.lyrics;
    } else {
        // Remove lyrics field if empty to let the LLM generate them
        requestObj.remove("lyrics");
    }
    
    // Apply vocal language override if set
    if (!song.vocalLanguage.isEmpty()) {
        requestObj["vocal_language"] = song.vocalLanguage;
    }
    
    // Write the request file
    QFile requestFileHandle(requestFile);
    if (!requestFileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit parent->generationError("Failed to create request file: " + requestFileHandle.errorString());
        return;
    }
    
    requestFileHandle.write(QJsonDocument(requestObj).toJson(QJsonDocument::Indented));
    requestFileHandle.close();
    
    // Use provided paths for acestep.cpp binaries
    QString qwen3Binary = this->aceStepPath + "/ace-qwen3";
    QString ditVaeBinary = this->aceStepPath + "/dit-vae";
    
    // Check if binaries exist
    QFileInfo qwen3Info(qwen3Binary);
    QFileInfo ditVaeInfo(ditVaeBinary);
    
    if (!qwen3Info.exists() || !qwen3Info.isExecutable()) {
        emit parent->generationError("ace-qwen3 binary not found at: " + qwen3Binary);
        return;
    }
    
    if (!ditVaeInfo.exists() || !ditVaeInfo.isExecutable()) {
        emit parent->generationError("dit-vae binary not found at: " + ditVaeBinary);
        return;
    }
    
    // Use provided model paths
    QString qwen3Model = this->qwen3ModelPath;
    QString textEncoderModel = this->textEncoderModelPath;
    QString ditModel = this->ditModelPath;
    QString vaeModel = this->vaeModelPath;
    
    if (!QFileInfo::exists(qwen3Model)) {
        emit parent->generationError("Qwen3 model not found: " + qwen3Model);
        return;
    }
    
    if (!QFileInfo::exists(textEncoderModel)) {
        emit parent->generationError("Text encoder model not found: " + textEncoderModel);
        return;
    }
    
    if (!QFileInfo::exists(ditModel)) {
        emit parent->generationError("DiT model not found: " + ditModel);
        return;
    }
    
    if (!QFileInfo::exists(vaeModel)) {
        emit parent->generationError("VAE model not found: " + vaeModel);
        return;
    }
    
    // Step 1: Run ace-qwen3 to generate lyrics and audio codes
    QProcess qwen3Process;
    QStringList qwen3Args;
    qwen3Args << "--request" << requestFile;
    qwen3Args << "--model" << qwen3Model;
    
    emit parent->progressUpdate(20);
    
    qwen3Process.start(qwen3Binary, qwen3Args);
    if (!qwen3Process.waitForStarted()) {
        emit parent->generationError("Failed to start ace-qwen3: " + qwen3Process.errorString());
        return;
    }
    
    if (!qwen3Process.waitForFinished(60000)) { // 60 second timeout
        qwen3Process.terminate();
        qwen3Process.waitForFinished(5000);
        emit parent->generationError("ace-qwen3 timed out after 60 seconds");
        return;
    }
    
    int exitCode = qwen3Process.exitCode();
    if (exitCode != 0) {
        QString errorOutput = qwen3Process.readAllStandardError();
        emit parent->generationError("ace-qwen3 exited with code " + QString::number(exitCode) + ": " + errorOutput);
        return;
    }

    QString requestLmOutputFile = tempDir + "/request_" + QString::number(uid) + "0.json";
    if (!QFileInfo::exists(requestLmOutputFile)) {
        emit parent->generationError("ace-qwen3 failed to create enhaced request file "+requestLmOutputFile);
        return;
    }
    
    // Load lyrics from the enhanced request file
    QFile lmOutputFile(requestLmOutputFile);
    if (lmOutputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonParseError parseError;
        song.json = lmOutputFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(song.json.toUtf8(), &parseError);
        lmOutputFile.close();
        
        if (doc.isObject() && !parseError.error) {
            QJsonObject obj = doc.object();
            if (obj.contains("lyrics") && obj["lyrics"].isString()) {
                song.lyrics = obj["lyrics"].toString();
            }
        }
    }
    
    emit parent->progressUpdate(50);
    
    // Step 2: Run dit-vae to generate audio
    QProcess ditVaeProcess;
    QStringList ditVaeArgs;
    ditVaeArgs << "--request" << requestLmOutputFile;
    ditVaeArgs << "--text-encoder" << textEncoderModel;
    ditVaeArgs << "--dit" << ditModel;
    ditVaeArgs << "--vae" << vaeModel;
    
    emit parent->progressUpdate(60);
    
    ditVaeProcess.start(ditVaeBinary, ditVaeArgs);
    if (!ditVaeProcess.waitForStarted()) {
        emit parent->generationError("Failed to start dit-vae: " + ditVaeProcess.errorString());
        return;
    }
    
    if (!ditVaeProcess.waitForFinished(120000)) { // 2 minute timeout
        ditVaeProcess.terminate();
        ditVaeProcess.waitForFinished(5000);
        emit parent->generationError("dit-vae timed out after 2 minutes");
        return;
    }
    
    exitCode = ditVaeProcess.exitCode();
    if (exitCode != 0) {
        QString errorOutput = ditVaeProcess.readAllStandardError();
        emit parent->generationError("dit-vae exited with code " + QString::number(exitCode) + ": " + errorOutput);
        return;
    }
    
    emit parent->progressUpdate(90);
    
    // Find the generated WAV file
    QString wavFile = QFileInfo(requestFile).absolutePath()+"/request_" + QString::number(uid) + "00.wav";
    if (!QFileInfo::exists(wavFile)) {
        emit parent->generationError("No WAV file generated at "+wavFile);
        return;
    }
    
    // Clean up temporary files
    QFile::remove(requestLmOutputFile);
    QFile::remove(requestFile);
    
    emit parent->progressUpdate(100);
    song.file = wavFile;
    emit parent->songGenerated(song);
	parent->workerMutex.lock();
	parent->currentWorker = nullptr;
	parent->workerMutex.unlock();
}

const SongItem& AceStepWorker::Worker::getSong()
{
	return song;
}
