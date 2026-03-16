// Copyright Carl Philipp Klemm 2026
// SPDX-License-Identifier: GPL-3.0-or-later

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

AceStep::AceStep(QObject* parent): QObject(parent)
{
	connect(&qwenProcess, &QProcess::finished, this, &AceStep::qwenProcFinished);
	connect(&ditVaeProcess, &QProcess::finished, this, &AceStep::ditProcFinished);
}

bool AceStep::isGenerating(SongItem* song)
{
	if(!busy && song)
		*song = this->request.song;
	return busy;
}

void AceStep::cancelGeneration()
{
	qwenProcess.blockSignals(true);
	qwenProcess.terminate();
	qwenProcess.waitForFinished();
	qwenProcess.blockSignals(false);

	ditVaeProcess.blockSignals(true);
	ditVaeProcess.terminate();
	ditVaeProcess.waitForFinished();
	ditVaeProcess.blockSignals(false);

	progressUpdate(100);
	if(busy)
		generationCanceled(request.song);

	busy = false;
}

bool AceStep::requestGeneration(SongItem song, QString requestTemplate, QString aceStepPath,
							   QString qwen3ModelPath, QString textEncoderModelPath, QString ditModelPath,
							   QString vaeModelPath)
{
	if(busy)
	{
		qWarning()<<"Dropping song:"<<song.caption;
		return false;
	}
	busy = true;

	request = {song, QRandomGenerator::global()->generate(), aceStepPath, textEncoderModelPath, ditModelPath, vaeModelPath};

	QString qwen3Binary = aceStepPath + "/ace-qwen3";
	QFileInfo qwen3Info(qwen3Binary);
	if (!qwen3Info.exists() || !qwen3Info.isExecutable())
	{
		generationError("ace-qwen3 binary not found at: " + qwen3Binary);
		busy = false;
		return false;
	}
	if (!QFileInfo::exists(qwen3ModelPath))
	{
		generationError("Qwen3 model not found: " + qwen3ModelPath);
		busy = false;
		return false;
	}
	if (!QFileInfo::exists(textEncoderModelPath))
	{
		generationError("Text encoder model not found: " + textEncoderModelPath);
		busy = false;
		return false;
	}
	if (!QFileInfo::exists(ditModelPath))
	{
		generationError("DiT model not found: " + ditModelPath);
		busy = false;
		return false;
	}
	if (!QFileInfo::exists(vaeModelPath))
	{
		generationError("VAE model not found: " + vaeModelPath);
		busy = false;
		return false;
	}

	request.requestFilePath = tempDir + "/request_" + QString::number(request.uid) + ".json";

	QJsonParseError parseError;
	QJsonDocument templateDoc = QJsonDocument::fromJson(requestTemplate.toUtf8(), &parseError);
	if (!templateDoc.isObject())
	{
		generationError("Invalid JSON template: " + QString(parseError.errorString()));
		busy = false;
		return false;
	}

	QJsonObject requestObj = templateDoc.object();
	requestObj["caption"] = song.caption;

	if (!song.lyrics.isEmpty())
		requestObj["lyrics"] = song.lyrics;
	if (!song.vocalLanguage.isEmpty())
		requestObj["vocal_language"] = song.vocalLanguage;

	// Write the request file
	QFile requestFileHandle(request.requestFilePath);
	if (!requestFileHandle.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		emit generationError("Failed to create request file: " + requestFileHandle.errorString());
		busy = false;
		return false;
	}
	requestFileHandle.write(QJsonDocument(requestObj).toJson(QJsonDocument::Indented));
	requestFileHandle.close();

	QStringList qwen3Args;
	qwen3Args << "--request" << request.requestFilePath;
	qwen3Args << "--model" << qwen3ModelPath;

	progressUpdate(30);

	qwenProcess.start(qwen3Binary, qwen3Args);
	return true;
}

void AceStep::qwenProcFinished(int code, QProcess::ExitStatus status)
{
	QFile::remove(request.requestFilePath);
	if(code != 0)
	{
		QString errorOutput = qwenProcess.readAllStandardError();
		generationError("ace-qwen3 exited with code " + QString::number(code) + ": " + errorOutput);
		busy = false;
		return;
	}

	QString ditVaeBinary = request.aceStepPath + "/dit-vae";
	QFileInfo ditVaeInfo(ditVaeBinary);
	if (!ditVaeInfo.exists() || !ditVaeInfo.isExecutable())
	{
		generationError("dit-vae binary not found at: " + ditVaeBinary);
		busy = false;
		return;
	}

	request.requestLlmFilePath = tempDir + "/request_" + QString::number(request.uid) + "0.json";
	if (!QFileInfo::exists(request.requestLlmFilePath))
	{
		generationError("ace-qwen3 failed to create enhanced request file "+request.requestLlmFilePath);
		busy = false;
		return;
	}

	// Load lyrics from the enhanced request file
	QFile lmOutputFile(request.requestLlmFilePath);
	if (lmOutputFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QJsonParseError parseError;
		request.song.json = lmOutputFile.readAll();
		QJsonDocument doc = QJsonDocument::fromJson(request.song.json.toUtf8(), &parseError);
		lmOutputFile.close();

		if (doc.isObject() && !parseError.error)
		{
			QJsonObject obj = doc.object();
			if (obj.contains("lyrics") && obj["lyrics"].isString())
				request.song.lyrics = obj["lyrics"].toString();
		}
	}

	// Step 2: Run dit-vae to generate audio
	QStringList ditVaeArgs;
	ditVaeArgs << "--request"<<request.requestLlmFilePath;
	ditVaeArgs << "--text-encoder"<<request.textEncoderModelPath;
	ditVaeArgs << "--dit"<<request.ditModelPath;
	ditVaeArgs << "--vae"<<request.vaeModelPath;

	progressUpdate(60);

	ditVaeProcess.start(ditVaeBinary, ditVaeArgs);
}

void AceStep::ditProcFinished(int code, QProcess::ExitStatus status)
{
	QFile::remove(request.requestLlmFilePath);
	if (code != 0)
	{
		QString errorOutput = ditVaeProcess.readAllStandardError();
		generationError("dit-vae exited with code " + QString::number(code) + ": " + errorOutput);
		busy = false;
		return;
	}

	// Find the generated WAV file
	QString wavFile = tempDir+"/request_" + QString::number(request.uid) + "00.wav";
	if (!QFileInfo::exists(wavFile))
	{
		generationError("No WAV file generated at "+wavFile);
		busy = false;
		return;
	}
	busy = false;

	progressUpdate(100);
	request.song.file = wavFile;
	songGenerated(request.song);
}

