#include "playlistmanager.h"
#include <QFile>
#include <QTextStream>

/**
 * @brief 构造函数
 */
PlaylistManager::PlaylistManager(QObject *parent)
    : QObject(parent)
{
}

struct PlaylistItem {
    QString filePath;
    int volume;
};

/**
 * @brief 添加歌曲到播放列表
 * @param filePath 歌曲文件路径
 * @param volume 初始音量（0-100）
 */
void PlaylistManager::addItem(const QString& filePath, int volume)
{
    m_items.append({filePath, volume});
    emit playlistChanged();
}

/**
 * @brief 从播放列表移除歌曲
 * @param index 歌曲索引
 */
void PlaylistManager::removeItem(int index)
{
    if (index >= 0 && index < m_items.size()) {
        m_items.removeAt(index);
        emit playlistChanged();
    }
}

/**
 * @brief 清空播放列表
 */
void PlaylistManager::clear()
{
    m_items.clear();
    emit playlistChanged();
}

/**
 * @brief 移动播放列表中歌曲的位置
 * @param from 原位置
 * @param to 目标位置
 */
void PlaylistManager::moveItem(int from, int to)
{
    if (from >= 0 && from < m_items.size() && to >= 0 && to < m_items.size()) {
        m_items.move(from, to);
        emit playlistChanged();
    }
}

/**
 * @brief 设置歌曲音量
 * @param index 歌曲索引
 * @param volume 音量值（0-100）
 */
void PlaylistManager::setVolume(int index, int volume)
{
    if (index >= 0 && index < m_items.size()) {
        m_items[index].volume = volume;
        emit itemVolumeChanged(index);
    }
}

/**
 * @brief 获取播放列表中歌曲数量
 * @return 歌曲数量
 */
int PlaylistManager::count() const
{
    return m_items.size();
}

/**
 * @brief 获取指定索引的歌曲信息
 * @param index 歌曲索引
 * @return 歌曲信息结构体
 */
PlaylistManager::PlaylistItem PlaylistManager::itemAt(int index) const
{
    return m_items.value(index);
}

/**
 * @brief 获取整个播放列表
 * @return 播放列表
 */
QList<PlaylistManager::PlaylistItem> PlaylistManager::items() const
{
    return m_items;
}
