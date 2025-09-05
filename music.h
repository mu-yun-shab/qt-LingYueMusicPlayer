#ifndef MUSIC_H
#define MUSIC_H

#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QTimer>
#include "floatinglyrics.h"
#include "playlistanimator.h"
#include "windowdragcontroller.h"
#include "musicplayer.h"
#include "playlistmanager.h"
#include "localmusicmanager.h"

class QListWidgetItem;

QT_BEGIN_NAMESPACE
namespace Ui { class Music; }
QT_END_NAMESPACE

/**
 * @brief 音乐播放器主窗口类
 */
class Music : public QMainWindow
{
    Q_OBJECT

public:
    Music(QWidget *parent = nullptr);
    ~Music();

public slots:
    /// 锁定悬浮歌词
    void onLockButtonClicked();

    /// 设置浮动歌词窗口透明度
    void setFloatingLyricsOpacity(int opacity);

    /// 切换浮动歌词窗口显示状态
    void toggleFloatingLyrics();

    ///更新标签的滚动
    void updateLabelScroll(QLabel *label, const QString &displayText, int totalLen, int &pos);

    ///更新滚动歌曲标题
    void updateScrollingTitle();

    /// 更新封面图片
    void updateCoverImage(const QString& songFilePath);

    ///模糊搜索
    void onLocalSearchTextChanged(const QString &text);

    ///导入播放列表
    void importPlaylist();

    /// 导出播放列表
    void exportPlaylist();

    /// 切换播放列表可见性
    void togglePlaylist();
    
    /// 关闭按钮点击处理
    void onCloseButtonClicked();
    
    /// 更新播放按钮图标
    void updatePlayButtonIcon();
    
    /// 切换播放/暂停状态
    void togglePlayPause();
    
    /// 播放下一首
    void playNext();
    
    /// 播放上一首
    void playPrevious();
    
    /// 更新播放位置
    void updatePlaybackPosition(qint64 position);
    
    /// 更新歌曲总时长
    void updateDuration(qint64 duration);
    
    /// 更新时间显示
    void updateTimeDisplay();
    
    /// 处理播放列表项双击事件
    void handlePlaylistItemDoubleClicked(QListWidgetItem *item);
    
    /// 更新本地音乐库显示
    void updateLibrary();
    
    /// 更新当前歌曲信息
    void updateCurrentSongInfo();
    
    /// 显示播放列表的右键菜单
    void showPlaylistContextMenu(const QPoint &pos);
    
    /// 显示本地列表的右键菜单
    void showLocalListContextMenu(const QPoint &pos);

private:
    /// 分享播放列表（打包ZIP）
    void sharePlaylist(const QString& playlistFilePath);

    /// 解析歌词文件
    void parseLyrics(const QString& lyrics);
    
    /// 更新歌词显示
    void updateLyricsDisplay(qint64 position);
    
    /// 设置所有信号槽连接
    void setupConnections();
    
    /// 格式化时间（毫秒转换为分:秒）
    QString formatTime(qint64 milliseconds) const;

    Ui::Music *ui;
    PlaylistAnimator *m_playlistAnimator;
    WindowDragController *m_dragController;
    MusicPlayer *m_player;
    PlaylistManager *m_playlistManager;
    LocalMusicManager *m_localMusicManager;
    
    QList<QPair<qint64, QString>> m_lyricLines; ///< 存储歌词行（时间戳和文本）
    int m_currentLyricIndex = -1;               ///< 当前歌词行索引
    int m_currentPlayIndex = -1;                ///< 当前播放的歌曲索引
    QStringList m_localSongs;                   ///< 存储完整的本地歌曲列表
    QTimer *m_titleScrollTimer;                 ///< 标题滚动计时器
    int m_scrollPos = 0;                        ///< 滚动位置
    int m_scrollPos2 = 0;                       ///< 滚动位置
    QString m_currentTitle;                     ///< 当前显示的完整标题
    FloatingLyrics *m_floatLyrics;              ///< 浮动歌词窗口
    bool m_floatLyricsVisible;                  ///< 浮动歌词窗口是否可见
    bool m_floatLyricsLocked;                   ///< 是否锁定浮动歌词
};

#endif
