#ifndef LOCALMUSICMANAGER_H
#define LOCALMUSICMANAGER_H

#include <QObject>
#include <QDir>
#include <QFileSystemWatcher>
#include <QTimer>

/**
 * @brief 本地音乐管理器，监控指定目录下的音乐文件
 */
class LocalMusicManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param directory 监控的音乐目录
     * @param parent 父对象
     */
    explicit LocalMusicManager(const QString& directory, QObject *parent = nullptr);

    /// 获取歌曲列表
    QStringList songs() const;
    
    /// 获取监控目录
    QString directory() const;

signals:
    /// 音乐库更新信号
    void libraryUpdated();

private slots:
    /// 扫描目录获取音乐文件
    void scanDirectory();
    
    /// 处理目录变化事件
    void directoryChanged();

private:
    QString m_directory;              ///< 监控的音乐目录
    QFileSystemWatcher m_watcher;     ///< 文件系统监控器
    QTimer m_rescanTimer;             ///< 重扫描定时器
    QStringList m_songs;              ///< 歌曲列表
};

#endif
