#ifndef ACESTEPWORKER_H
#define ACESTEPWORKER_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QString>
#include <QJsonObject>
#include <QMutex>

#include "SongItem.h"

class AceStepWorker : public QObject
{
    Q_OBJECT
public:
    explicit AceStepWorker(QObject *parent = nullptr);
    ~AceStepWorker();
    
    void generateSong(const SongItem& song, const QString &jsonTemplate,
                     const QString &aceStepPath, const QString &qwen3ModelPath,
                     const QString &textEncoderModelPath, const QString &ditModelPath,
                     const QString &vaeModelPath);
    void cancelGeneration();
	bool songGenerateing(SongItem* song);
    
signals:
    void songGenerated(const SongItem& song);
    void generationFinished();
    void generationError(const QString &error);
	void progressUpdate(int percent);
    
private:
    class Worker : public QRunnable {
    public:
        Worker(AceStepWorker *parent, const SongItem& song, const QString &jsonTemplate,
               const QString &aceStepPath, const QString &qwen3ModelPath,
               const QString &textEncoderModelPath, const QString &ditModelPath,
               const QString &vaeModelPath)
            : parent(parent), song(song), jsonTemplate(jsonTemplate),
              aceStepPath(aceStepPath), qwen3ModelPath(qwen3ModelPath),
              textEncoderModelPath(textEncoderModelPath), ditModelPath(ditModelPath),
              vaeModelPath(vaeModelPath) {}
        
        void run() override;

		const SongItem& getSong();
        
    private:
        AceStepWorker *parent;
        SongItem song;
        QString jsonTemplate;
        QString aceStepPath;
        QString qwen3ModelPath;
        QString textEncoderModelPath;
        QString ditModelPath;
        QString vaeModelPath;
    };

	QMutex workerMutex;
    Worker *currentWorker;
};

#endif // ACESTEPWORKER_H
