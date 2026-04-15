/*
 * Copyright Carl Philipp Klemm 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ACESTEPWORKER_H
#define ACESTEPWORKER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QStandardPaths>
#include <atomic>

#include "SongItem.h"

// acestep.cpp headers
#include "request.h"

struct AceLm;
struct AceSynth;

class AceStepWorker : public QObject
{
	Q_OBJECT

public:
	explicit AceStepWorker(QObject* parent = nullptr);
	~AceStepWorker();

	bool isGenerating(SongItem* song = nullptr);
	void cancelGeneration();

	// Model paths - set these before first generation
	void setModelPaths(QString lmPath, QString textEncoderPath, QString ditPath, QString vaePath);

	// Low VRAM mode: unload models between phases to save VRAM
	void setLowVramMode(bool enabled);
	bool isLowVramMode() const { return m_lowVramMode; }

	// Flash attention mode
	void setFlashAttention(bool enabled);
	bool isFlashAttention() const { return m_flashAttention; }

	// Request a new song generation
	bool requestGeneration(SongItem song, QString requestTemplate);

signals:
	void songGenerated(SongItem song);
	void generationCanceled(SongItem song);
	void generationError(QString error);
	void progressUpdate(int progress);

private slots:
	void runGeneration();

private:
	// Check if cancellation was requested
	static bool checkCancel(void* data);

	// Load models if not already loaded
	bool loadModels();
	void unloadModels();

	// Individual model load/unload for low VRAM mode
	bool loadLm();
	void unloadLm();
	bool loadSynth();
	void unloadSynth();

	// Convert SongItem to AceRequest
	AceRequest songToRequest(const SongItem& song, const QString& templateJson);

	// Convert AceRequest back to SongItem
	SongItem requestToSong(const AceRequest& req, const QString& json);

	// Generation state
	std::atomic<bool> m_busy{false};
	std::atomic<bool> m_cancelRequested{false};
	std::atomic<bool> m_modelsLoaded{false};
	bool m_lowVramMode = false;
	bool m_flashAttention = true;

	// Current request data
	SongItem m_currentSong;
	QString m_requestTemplate;
	uint64_t m_uid;

	// Model paths
	QString m_lmModelPath;
	QString m_textEncoderPath;
	QString m_ditPath;
	QString m_vaePath;

	// Loaded models (accessed from worker thread only)
	AceLm* m_lmContext = nullptr;
	AceSynth* m_synthContext = nullptr;

	// Cached model paths as byte arrays (to avoid dangling pointers)
	QByteArray m_lmModelPathBytes;
	QByteArray m_textEncoderPathBytes;
	QByteArray m_ditPathBytes;
	QByteArray m_vaePathBytes;

	const QString m_tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
};

#endif // ACESTEPWORKER_H