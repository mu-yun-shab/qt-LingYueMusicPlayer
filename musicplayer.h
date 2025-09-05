#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QTimer>
#include "playlistmanager.h"

/**
 * @brief 音乐播放器核心类
 */
class MusicPlayer : public QObject
{
    Q_OBJECT
public:
    explicit MusicPlayer(QObject *parent = nullptr);
    ~MusicPlayer();

    // 播放控制
    void play();
    void pause();
    void stop();
    void next();
    void previous();

    // 参数设置
    void setVolume(int volume);
    void setCurrentIndex(int index);
    void setPosition(qint64 position);
    void setCurrentMedia(const QString& filePath);

    // 状态查询
    bool isPlaying() const;
    qint64 duration() const;
    qint64 position() const;
    
    /// 检查格式是否支持
    static bool isFormatSupported(const QString& filePath);

signals:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void stateChanged(QMediaPlayer::State state);
    void currentMediaChanged(const QString& title);
    void mediaError(const QString& errorMessage);
    void mediaFinished();

private slots:
    /// 更新播放位置（定时器触发）
    void updatePosition();
    
    /// 处理媒体状态变化
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    
    /// 处理媒体错误
    void onMediaError(QMediaPlayer::Error error);

private:
    QMediaPlayer *m_player;          ///< 媒体播放器
    QMediaPlaylist *m_playlist;      ///< 播放列表
    QTimer *m_positionTimer;         ///< 位置更新定时器
    QString m_currentFilePath;       ///< 当前文件路径

    /// 解码器支持映射
    static QMap<QString, QString> m_supportedFormats;
    
    /// 初始化支持格式
    static void initSupportedFormats();

public slots:
    /// 设置播放列表
    void setPlaylist(const QList<PlaylistManager::PlaylistItem>& playlist);
};

#endif
