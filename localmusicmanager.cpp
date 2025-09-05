#include "localmusicmanager.h"
#include <QDebug>

LocalMusicManager::LocalMusicManager(const QString& directory, QObject *parent)
    : QObject(parent), m_directory(directory), m_rescanTimer(this)
{
    m_rescanTimer.setSingleShot(true);
    m_rescanTimer.setInterval(500);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &LocalMusicManager::directoryChanged);
    connect(&m_rescanTimer, &QTimer::timeout,
            this, &LocalMusicManager::scanDirectory);

    // 确保目录存在
    QDir dir(m_directory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_watcher.addPath(m_directory);
    scanDirectory();
}

QStringList LocalMusicManager::songs() const
{
    return m_songs;
}

QString LocalMusicManager::directory() const
{
    return m_directory;
}

void LocalMusicManager::scanDirectory()
{
    QDir dir(m_directory);

    // 支持的音乐格式：MP3, WAV, FLAC, OGG, WMA, AAC
    static const QStringList nameFilters = {
        "*.mp3", "*.wav", "*.flac",
        "*.ogg", "*.wma", "*.aac"
    };

    QStringList newSongs = dir.entryList(nameFilters, QDir::Files);

    if (newSongs != m_songs) {
        m_songs = newSongs;
        emit libraryUpdated();
    }
}

void LocalMusicManager::directoryChanged()
{
    m_rescanTimer.start();
}
