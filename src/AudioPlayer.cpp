#include "AudioPlayer.h"
#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent),
      mediaPlayer(new QMediaPlayer(this)),
      audioOutput(new QAudioOutput(this)),
      positionTimer(new QTimer(this))
{
    // Set up audio output with default device
    mediaPlayer->setAudioOutput(audioOutput);
    
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &AudioPlayer::handlePlaybackStateChanged);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &AudioPlayer::handleMediaStatusChanged);
    
    // Set up position timer for updating playback position
    positionTimer->setInterval(500); // Update every 500ms
    connect(positionTimer, &QTimer::timeout, [this]() {
        if (isPlaying()) {
            emit positionChanged(mediaPlayer->position());
        }
    });
}

AudioPlayer::~AudioPlayer()
{
    stop();
}

void AudioPlayer::play(const QString &filePath)
{
    if (isPlaying()) {
        stop();
    }
    
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
    mediaPlayer->play();
    
    // Start position timer
    positionTimer->start();
}

void AudioPlayer::play()
{
    if (!isPlaying()) {
        mediaPlayer->play();
        positionTimer->start();
    }
}

void AudioPlayer::pause()
{
    if (isPlaying()) {
        mediaPlayer->pause();
        positionTimer->stop();
    }
}

void AudioPlayer::setPosition(int position)
{
    mediaPlayer->setPosition(position);
}

void AudioPlayer::stop()
{
    mediaPlayer->stop();
    positionTimer->stop();
}

bool AudioPlayer::isPlaying() const
{
    return mediaPlayer->playbackState() == QMediaPlayer::PlayingState;
}

int AudioPlayer::duration() const
{
    return mediaPlayer->duration();
}

int AudioPlayer::position() const
{
    return mediaPlayer->position();
}

void AudioPlayer::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        emit playbackStarted();
    } else if (state == QMediaPlayer::StoppedState || 
               state == QMediaPlayer::PausedState) {
        // Check if we reached the end
        if (mediaPlayer->position() >= mediaPlayer->duration() - 100) {
            emit playbackFinished();
        }
    }
}

void AudioPlayer::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        emit playbackFinished();
    } else if (status == QMediaPlayer::LoadedMedia || 
               status == QMediaPlayer::BufferedMedia) {
        // Media loaded successfully, emit duration
        int duration = mediaPlayer->duration();
        if (duration > 0) {
            emit durationChanged(duration);
        }
    } else if (status == QMediaPlayer::InvalidMedia) {
        emit playbackError(mediaPlayer->errorString());
    }
}
