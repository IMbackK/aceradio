// Copyright Carl Philipp Klemm 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AceStepWorker.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDebug>
#include <QRandomGenerator>

// acestep.cpp headers
#include "pipeline-lm.h"
#include "request.h"

AceStepWorker::AceStepWorker(QObject* parent)
	: QObject(parent)
{
}

AceStepWorker::~AceStepWorker()
{
	cancelGeneration();
	unloadModels();
}

void AceStepWorker::setModelPaths(QString lmPath, QString textEncoderPath, QString ditPath, QString vaePath)
{
	// Check if paths actually changed
	bool pathsChanged = (m_lmModelPath != lmPath || m_textEncoderPath != textEncoderPath ||
						m_ditPath != ditPath || m_vaePath != vaePath);
	
	m_lmModelPath = lmPath;
	m_textEncoderPath = textEncoderPath;
	m_ditPath = ditPath;
	m_vaePath = vaePath;
	
	// Cache as byte arrays to avoid dangling pointers
	m_lmModelPathBytes = lmPath.toUtf8();
	m_textEncoderPathBytes = textEncoderPath.toUtf8();
	m_ditPathBytes = ditPath.toUtf8();
	m_vaePathBytes = vaePath.toUtf8();
	
	// If paths changed and models are loaded, unload them so they'll be reloaded with new paths
	if (pathsChanged && m_modelsLoaded.load())
	{
		unloadModels();
	}
}

void AceStepWorker::setLowVramMode(bool enabled)
{
	m_lowVramMode = enabled;
}

void AceStepWorker::setFlashAttention(bool enabled)
{
	m_flashAttention = enabled;
}

bool AceStepWorker::isGenerating(SongItem* song)
{
	if (!m_busy.load() && song)
		*song = m_currentSong;
	return m_busy.load();
}

void AceStepWorker::cancelGeneration()
{
	m_cancelRequested.store(true);
}

bool AceStepWorker::requestGeneration(SongItem song, QString requestTemplate)
{
	if (m_busy.load())
	{
		qWarning() << "Dropping song:" << song.caption;
		return false;
	}

	m_busy.store(true);
	m_cancelRequested.store(false);
	m_currentSong = song;
	m_requestTemplate = requestTemplate;
	m_uid = QRandomGenerator::global()->generate();

	// Validate model paths
	if (m_lmModelPath.isEmpty() || m_textEncoderPath.isEmpty() ||
		m_ditPath.isEmpty() || m_vaePath.isEmpty())
	{
		emit generationError("Model paths not set. Call setModelPaths() first.");
		m_busy.store(false);
		return false;
	}

	// Validate model files exist
	if (!QFileInfo::exists(m_lmModelPath))
	{
		emit generationError("LM model not found: " + m_lmModelPath);
		m_busy.store(false);
		return false;
	}
	if (!QFileInfo::exists(m_textEncoderPath))
	{
		emit generationError("Text encoder model not found: " + m_textEncoderPath);
		m_busy.store(false);
		return false;
	}
	if (!QFileInfo::exists(m_ditPath))
	{
		emit generationError("DiT model not found: " + m_ditPath);
		m_busy.store(false);
		return false;
	}
	if (!QFileInfo::exists(m_vaePath))
	{
		emit generationError("VAE model not found: " + m_vaePath);
		m_busy.store(false);
		return false;
	}

	// Validate template
	QJsonParseError parseError;
	QJsonDocument templateDoc = QJsonDocument::fromJson(requestTemplate.toUtf8(), &parseError);
	if (!templateDoc.isObject())
	{
		emit generationError("Invalid JSON template: " + QString(parseError.errorString()));
		m_busy.store(false);
		return false;
	}

	// Run generation in the worker thread
	QMetaObject::invokeMethod(this, &AceStepWorker::runGeneration, Qt::QueuedConnection);
	return true;
}

bool AceStepWorker::checkCancel(void* data)
{
	AceStepWorker* worker = static_cast<AceStepWorker*>(data);
	return worker->m_cancelRequested.load();
}

std::shared_ptr<QByteArray> AceStepWorker::convertToWav(const AceAudio& audio)
{
	auto audioData = std::make_shared<QByteArray>();

	// Simple WAV header + stereo float data
	int numChannels = 2;
	int bitsPerSample = 16;
	int byteRate = audio.sample_rate * numChannels * (bitsPerSample / 8);
	int blockAlign = numChannels * (bitsPerSample / 8);
	int dataSize = audio.n_samples * numChannels * (bitsPerSample / 8);

	// RIFF header
	audioData->append("RIFF");
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&dataSize), 4));
	audioData->append("WAVE");

	// fmt chunk
	audioData->append("fmt ");
	int fmtSize = 16;
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&fmtSize), 4));
	short audioFormat = 1;  // PCM
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&audioFormat), 2));
	short numCh = numChannels;
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&numCh), 2));
	int sampleRate = audio.sample_rate;
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&sampleRate), 4));
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&byteRate), 4));
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&blockAlign), 2));
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&bitsPerSample), 2));

	// data chunk
	audioData->append("data");
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(&dataSize), 4));

	// Convert float samples to 16-bit and write
	QVector<short> interleaved(audio.n_samples * numChannels);
	for (int i = 0; i < audio.n_samples; i++)
	{
		float left = audio.samples[i];
		float right = audio.samples[i + audio.n_samples];
		// Clamp and convert to 16-bit
		left = std::max(-1.0f, std::min(1.0f, left));
		right = std::max(-1.0f, std::min(1.0f, right));
		interleaved[i * 2] = static_cast<short>(left * 32767.0f);
		interleaved[i * 2 + 1] = static_cast<short>(right * 32767.0f);
	}
	audioData->append(QByteArray::fromRawData(reinterpret_cast<const char*>(interleaved.data()), dataSize));
	return audioData;
}

void AceStepWorker::runGeneration()
{
	// Convert SongItem to AceRequest
	AceRequest req = songToRequest(m_currentSong, m_requestTemplate);
	AceRequest lmOutput;
	request_init(&lmOutput);

	emit progressUpdate(10);

	if (!loadLm())
	{
		m_busy.store(false);
		return;
	}

	emit progressUpdate(30);

	int lmResult = ace_lm_generate(m_lmContext, &req, 1, &lmOutput,
									nullptr, nullptr,
									checkCancel, this,
									LM_MODE_GENERATE);

	if (m_cancelRequested.load())
	{
		if(m_lowVramMode)
			unloadModels();
		emit generationCanceled(m_currentSong);
		m_busy.store(false);
		return;
	}

	if (lmResult != 0)
	{
		if(m_lowVramMode)
			unloadModels();
		emit generationError("LM generation failed or was canceled");
		m_busy.store(false);
		return;
	}

	m_currentSong.lyrics = QString::fromStdString(lmOutput.lyrics);

	if(m_lowVramMode)
		unloadLm();

	emit progressUpdate(50);

	if (!loadSynth())
	{
		m_busy.store(false);
		return;
	}

	emit progressUpdate(60);

	AceAudio outputAudio;
	outputAudio.samples = nullptr;
	outputAudio.n_samples = 0;
	outputAudio.sample_rate = 48000;

	int synthResult = ace_synth_generate(m_synthContext, &lmOutput,
										  nullptr, 0,  // no source audio
										  nullptr, 0,  // no reference audio
										  1, &outputAudio,
										  checkCancel, this);

	if(m_lowVramMode)
		unloadSynth();

	if (m_cancelRequested.load())
	{
		emit generationCanceled(m_currentSong);
		m_busy.store(false);
		return;
	}

	if (synthResult != 0)
	{
		emit generationError("Synthesis failed or was canceled");
		m_busy.store(false);
		return;
	}

	std::shared_ptr<QByteArray> audioData = convertToWav(outputAudio);
	ace_audio_free(&outputAudio);

	m_currentSong.json = QString::fromStdString(request_to_json(&lmOutput, true));
	m_currentSong.audioData = audioData;

	if (lmOutput.bpm > 0)
		m_currentSong.bpm = lmOutput.bpm;

	if (!lmOutput.keyscale.empty())
		m_currentSong.key = QString::fromStdString(lmOutput.keyscale);

	emit progressUpdate(100);
	emit songGenerated(m_currentSong);

	m_busy.store(false);
}

bool AceStepWorker::loadModels()
{
	bool ret = loadSynth();
	if(!ret)
		return false;

	ret = loadLm();
	if(!ret)
		return false;
	return true;
}

void AceStepWorker::unloadModels()
{
	if (m_synthContext)
	{
		ace_synth_free(m_synthContext);
		m_synthContext = nullptr;
	}
	if (m_lmContext)
	{
		ace_lm_free(m_lmContext);
		m_lmContext = nullptr;
	}
	m_modelsLoaded.store(false);
}

bool AceStepWorker::loadLm()
{
	if (m_lmContext)
		return true;

	AceLmParams lmParams;
	ace_lm_default_params(&lmParams);
	lmParams.model_path = m_lmModelPathBytes.constData();
	lmParams.use_fsm = true;
	lmParams.use_fa = m_flashAttention;

	m_lmContext = ace_lm_load(&lmParams);
	if (!m_lmContext)
	{
		emit generationError("Failed to load LM model: " + m_lmModelPath);
		return false;
	}
	return true;
}

void AceStepWorker::unloadLm()
{
	if (m_lmContext)
	{
		ace_lm_free(m_lmContext);
		m_lmContext = nullptr;
	}
}

bool AceStepWorker::loadSynth()
{
	if (m_synthContext)
		return true;

	AceSynthParams synthParams;
	ace_synth_default_params(&synthParams);
	synthParams.text_encoder_path = m_textEncoderPathBytes.constData();
	synthParams.dit_path = m_ditPathBytes.constData();
	synthParams.vae_path = m_vaePathBytes.constData();
	synthParams.use_fa = m_flashAttention;

	m_synthContext = ace_synth_load(&synthParams);
	if (!m_synthContext)
	{
		emit generationError("Failed to load synthesis models");
		return false;
	}
	return true;
}

void AceStepWorker::unloadSynth()
{
	if (m_synthContext)
	{
		ace_synth_free(m_synthContext);
		m_synthContext = nullptr;
	}
}

AceRequest AceStepWorker::songToRequest(const SongItem& song, const QString& templateJson)
{
	AceRequest req;
	request_init(&req);

	req.caption = song.caption.toStdString();
	req.lyrics = song.lyrics.toStdString();
	req.use_cot_caption = song.cotCaption;

	// Parse template and override defaults
	QJsonParseError parseError;
	QJsonDocument templateDoc = QJsonDocument::fromJson(templateJson.toUtf8(), &parseError);
	if (templateDoc.isObject())
	{
		QJsonObject obj = templateDoc.object();
		if (obj.contains("inference_steps"))
			req.inference_steps = obj["inference_steps"].toInt(8);
		if (obj.contains("shift"))
			req.shift = obj["shift"].toDouble(3.0);
		if (obj.contains("vocal_language"))
			req.vocal_language = obj["vocal_language"].toString().toStdString();
		if (obj.contains("bpm"))
			req.bpm = obj["bpm"].toInt(120);
		if (obj.contains("duration"))
			req.duration = obj["duration"].toDouble(180.0);
		if (obj.contains("keyscale"))
			req.keyscale = obj["keyscale"].toString().toStdString();
		if (obj.contains("lm_temperature"))
			req.lm_temperature = obj["lm_temperature"].toDouble(0.85);
		if (obj.contains("lm_cfg_scale"))
			req.lm_cfg_scale = obj["lm_cfg_scale"].toDouble(2.0);
	}

	// Generate a seed for reproducibility
	req.seed = static_cast<int64_t>(QRandomGenerator::global()->generate());

	return req;
}

SongItem AceStepWorker::requestToSong(const AceRequest& req, const QString& json)
{
	SongItem song;
	song.caption = QString::fromStdString(req.caption);
	song.lyrics = QString::fromStdString(req.lyrics);
	song.cotCaption = req.use_cot_caption;

	if (req.bpm > 0)
		song.bpm = req.bpm;
	if (!req.keyscale.empty())
		song.key = QString::fromStdString(req.keyscale);
	if (!req.vocal_language.empty())
		song.vocalLanguage = QString::fromStdString(req.vocal_language);

	song.json = json;
	return song;
}