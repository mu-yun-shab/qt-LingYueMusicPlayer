#ifndef PLAYLISTANIMATOR_H
#define PLAYLISTANIMATOR_H

#include <QObject>
#include <QPropertyAnimation>
#include <QWidget>
#include <QParallelAnimationGroup>

/**
 * @brief 播放列表动画管理器
 */
class PlaylistAnimator : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistAnimator(QWidget *playlist,
                             QWidget *openButton,
                             QWidget *closeButton,
                             QWidget *importButton,  // 新增导入按钮
                             QWidget *exportButton,  // 新增导出按钮
                             QObject *parent = nullptr);

    /// 切换播放列表可见性
    void togglePlaylist();
    
    /// 检查播放列表是否可见
    bool isPlaylistVisible() const;

private:
    /// 调整图层顺序
    void adjustZOrder();
    
    QWidget *m_playlist;             ///< 播放列表控件
    QWidget *m_openButton;            ///< 打开按钮
    QWidget *m_closeButton;           ///< 关闭按钮
    QWidget *m_importButton;          ///< 导入按钮  
    QWidget *m_exportButton;          ///< 导出按钮  
    QParallelAnimationGroup *m_animationGroup;  ///< 动画组
    QPropertyAnimation *m_playlistAnimation;    ///< 播放列表动画
    QPropertyAnimation *m_closeButtonAnimation; ///< 关闭按钮动画
    QPropertyAnimation *m_importButtonAnimation; ///< 导入按钮动画  
    QPropertyAnimation *m_exportButtonAnimation; ///< 导出按钮动画  
    bool m_visible;                   ///< 播放列表是否可见
    
    QRect m_playlistVisibleRect;      ///< 播放列表可见位置
    QRect m_playlistHiddenRect;       ///< 播放列表隐藏位置
    QRect m_closeButtonVisibleRect;   ///< 关闭按钮可见位置
    QRect m_closeButtonHiddenRect;    ///< 关闭按钮隐藏位置
    QRect m_importButtonVisibleRect;  ///< 导入按钮可见位置  
    QRect m_importButtonHiddenRect;   ///< 导入按钮隐藏位置  
    QRect m_exportButtonVisibleRect;  ///< 导出按钮可见位置  
    QRect m_exportButtonHiddenRect;   ///< 导出按钮隐藏位置  
};

#endif