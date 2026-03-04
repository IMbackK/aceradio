#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFileInfo>
#include <QString>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QTimer>

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();
    
    void play(const QString &filePath);
    void play();
    void stop();
    void pause();
    void setPosition(int position);
    bool isPlaying() const;
    int duration() const;
    int position() const;
    
signals:
    void playbackStarted();
    void playbackFinished();
    void playbackError(const QString &error);
    void positionChanged(int position);
    void durationChanged(int duration);
    
private slots:
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    
private:
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    QTimer *positionTimer;
};

#endif // AUDIOPLAYER_H
