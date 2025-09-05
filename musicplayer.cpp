#include "musicplayer.h"
#include <QFileInfo>
#include <QDebug>
#include <QMediaMetaData>
#include <QMediaResource>
#include <QSet>

// 初始化静态成员
QMap<QString, QString> MusicPlayer::m_supportedFormats;

// 初始化支持的格式
void MusicPlayer::initSupportedFormats()
{
    if (m_supportedFormats.isEmpty()) {
        // 核心支持的格式
        m_supportedFormats.insert("mp3", "MP3 Audio");
        m_supportedFormats.insert("wav", "WAV Audio");
        m_supportedFormats.insert("ogg", "Ogg Vorbis");
        m_supportedFormats.insert("flac", "FLAC Audio");
        m_supportedFormats.insert("aac", "AAC Audio");
        m_supportedFormats.insert("m4a", "MPEG-4 Audio");
        m_supportedFormats.insert("wma", "Windows Media Audio");
        
        // 扩展支持的格式（平台相关）
#ifdef Q_OS_WIN
        // Windows平台额外支持
        m_supportedFormats.insert("ac3", "Dolby Digital");
        m_supportedFormats.insert("dts", "DTS Audio");
#elif defined(Q_OS_MAC)
        // macOS平台额外支持
        m_supportedFormats.insert("aiff", "AIFF Audio");
        m_supportedFormats.insert("caf", "Core Audio Format");
#elif defined(Q_OS_LINUX)
        // Linux平台额外支持
        m_supportedFormats.insert("opus", "Opus Audio");
#endif
    }
}

// 检查格式是否支持
bool MusicPlayer::isFormatSupported(const QString& filePath)
{
    if (m_supportedFormats.isEmpty()) {
        initSupportedFormats();
    }
    
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    return m_supportedFormats.contains(extension);
}

/**
 * @brief 构造函数，初始化播放器
 */
MusicPlayer::MusicPlayer(QObject *parent) 
    : QObject(parent), 
      m_player(new QMediaPlayer(this)),
      m_playlist(new QMediaPlaylist(this)),
      m_positionTimer(new QTimer(this))
{
    initSupportedFormats(); // 初始化格式支持
    
    // 配置硬件加速
#ifdef Q_OS_WIN
    m_player->setProperty("videoAcceleration", "dxva2");
#elif defined(Q_OS_MAC)
    m_player->setProperty("videoAcceleration", "videotoolbox");
#elif defined(Q_OS_LINUX)
    m_player->setProperty("videoAcceleration", "vaapi");
#endif

    m_player->setPlaylist(m_playlist);
    m_playlist->setPlaybackMode(QMediaPlaylist::Loop);

    connect(m_player, SIGNAL(error(QMediaPlayer::Error)), 
            this, SLOT(onMediaError(QMediaPlayer::Error)));

    connect(m_positionTimer, &QTimer::timeout, this, &MusicPlayer::updatePosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MusicPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::handleMediaStatusChanged);
    connect(m_player, &QMediaPlayer::stateChanged, this, &MusicPlayer::stateChanged);
    
    m_positionTimer->start(500);
}

/**
 * @brief 析构函数，清理资源
 */
MusicPlayer::~MusicPlayer()
{
    stop();
    delete m_player;
}

/**
 * @brief 开始播放
 */
void MusicPlayer::play()
{
    if (m_player->state() != QMediaPlayer::PlayingState) {
        m_player->play();
    }
}

/**
 * @brief 暂停播放
 */
void MusicPlayer::pause()
{
    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
    }
}

/**
 * @brief 停止播放
 */
void MusicPlayer::stop()
{
    m_player->stop();
}

/**
 * @brief 播放下一首
 */
void MusicPlayer::next()
{
    m_playlist->next();
}

/**
 * @brief 播放上一首
 */
void MusicPlayer::previous()
{
    m_playlist->previous();
}

/**
 * @brief 设置音量
 * @param volume 音量值 (0-100)
 */
void MusicPlayer::setVolume(int volume)
{
    m_player->setVolume(volume);
}

/**
 * @brief 设置播放位置
 * @param position 播放位置（毫秒）
 */
void MusicPlayer::setPosition(qint64 position)
{
    m_player->setPosition(position);
}

/**
 * @brief 设置当前媒体文件
 * @param filePath 媒体文件路径
 */
void MusicPlayer::setCurrentMedia(const QString& filePath)
{
    if (m_currentFilePath != filePath) {
        m_currentFilePath = filePath;
        m_player->setMedia(QUrl::fromLocalFile(filePath));
        
        QFileInfo fileInfo(filePath);
        emit currentMediaChanged(fileInfo.baseName());
    }
}

/**
 * @brief 检查是否正在播放
 * @return 正在播放返回true，否则false
 */
bool MusicPlayer::isPlaying() const
{
    return m_player->state() == QMediaPlayer::PlayingState;
}

/**
 * @brief 获取当前媒体时长
 * @return 媒体时长（毫秒）
 */
qint64 MusicPlayer::duration() const
{
    return m_player->duration();
}

/**
 * @brief 获取当前播放位置
 * @return 播放位置（毫秒）
 */
qint64 MusicPlayer::position() const
{
    return m_player->position();
}

/**
 * @brief 更新播放位置（定时器触发）
 */
void MusicPlayer::updatePosition()
{
    if (m_player->state() == QMediaPlayer::PlayingState) {
        emit positionChanged(m_player->position());
    }
}

/**
 * @brief 处理媒体状态变化
 * @param status 媒体状态
 */
void MusicPlayer::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia) {
        emit durationChanged(m_player->duration());
    } else if (status == QMediaPlayer::EndOfMedia) {
        // 媒体播放完成时发出信号
        emit mediaFinished();
    }
}

void MusicPlayer::setPlaylist(const QList<PlaylistManager::PlaylistItem>& playlist)
{
    m_playlist->clear();
    for (const auto& item : playlist) {
        // 检查格式支持
        if (isFormatSupported(item.filePath)) {
            m_playlist->addMedia(QUrl::fromLocalFile(item.filePath));
        } else {
            qWarning() << "Unsupported format:" << item.filePath;
            emit mediaError("不支持的音频格式: " + QFileInfo(item.filePath).suffix());
        }
    }
}

void MusicPlayer::onMediaError(QMediaPlayer::Error error)
{
    QString errorString;
    switch (error) {
    case QMediaPlayer::ResourceError:
        errorString = "媒体资源无法解析";
        break;
    case QMediaPlayer::FormatError:
        errorString = "媒体格式不支持";
        break;
    case QMediaPlayer::NetworkError:
        errorString = "网络错误";
        break;
    case QMediaPlayer::AccessDeniedError:
        errorString = "访问被拒绝";
        break;
    default:
        errorString = "未知媒体错误";
    }
    emit mediaError(errorString);
}

void MusicPlayer::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_playlist->mediaCount()) {
        m_playlist->setCurrentIndex(index);
        QMediaContent media = m_playlist->currentMedia();
        if (!media.isNull()) {
            QString filePath = media.canonicalUrl().toLocalFile();
            
            // 流式处理大文件
            if (QFileInfo(filePath).size() > 100 * 1024 * 1024) { // 100MB以上
                m_player->setMedia(media, nullptr);
            } else {
                m_player->setMedia(media);
            }
            
            QFileInfo fileInfo(filePath);
            emit currentMediaChanged(fileInfo.baseName());
        }
    }
}
