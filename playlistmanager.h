#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QFileInfo>

/**
 * @brief 播放列表管理器
 */
class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    /// 播放列表项结构
    struct PlaylistItem {
        QString filePath;  ///< 文件路径
        int volume;        ///< 音量值 (0-100)
    };

    explicit PlaylistManager(QObject *parent = nullptr);

    /// 添加歌曲到播放列表
    void addItem(const QString& filePath, int volume = 50);
    
    /// 从播放列表移除歌曲
    void removeItem(int index);
    
    /// 清空播放列表
    void clear();
    
    /// 移动播放列表中歌曲的位置
    void moveItem(int from, int to);
    
    /// 设置歌曲音量
    void setVolume(int index, int volume);
    
    /// 获取播放列表中歌曲数量
    int count() const;
    
    /// 获取指定索引的歌曲信息
    PlaylistItem itemAt(int index) const;
    
    /// 获取整个播放列表
    QList<PlaylistItem> items() const;

signals:
    /// 播放列表改变信号
    void playlistChanged();
    
    /// 歌曲音量改变信号
    void itemVolumeChanged(int index);

private:
    QList<PlaylistItem> m_items;  ///< 播放列表项集合
};

#endif
