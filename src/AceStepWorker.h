#ifndef ACESTEPWORKER_H
#define ACESTEPWORKER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QStandardPaths>

#include "SongItem.h"

class AceStep : public QObject
{
	Q_OBJECT
	QProcess qwenProcess;
	QProcess ditVaeProcess;

	bool busy = false;

	struct Request
	{
		SongItem song;
		uint64_t uid;
		QString aceStepPath;
		QString textEncoderModelPath;
		QString ditModelPath;
		QString vaeModelPath;
		QString requestFilePath;
		QString requestLlmFilePath;
	};

	Request request;

	const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

signals:
	void songGenerated(SongItem song);
	void generationCanceled(SongItem song);
	void generationError(QString error);
	void progressUpdate(int progress);

public slots:
	bool requestGeneration(SongItem song, QString requestTemplate, QString aceStepPath,
						   QString qwen3ModelPath, QString textEncoderModelPath, QString ditModelPath,
						   QString vaeModelPath);

public:
	AceStep(QObject* parent = nullptr);
	bool isGenerating(SongItem* song = nullptr);
	void cancelGeneration();

private slots:
	void qwenProcFinished(int code, QProcess::ExitStatus status);
	void ditProcFinished(int code, QProcess::ExitStatus status);
};

#endif // ACESTEPWORKER_H
