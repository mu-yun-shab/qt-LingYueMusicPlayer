#include "playlistanimator.h"
#include <QApplication>
#include <QDebug>

PlaylistAnimator::PlaylistAnimator(QWidget *playlist,
                                 QWidget *openButton,
                                 QWidget *closeButton,
                                 QWidget *importButton,  
                                 QWidget *exportButton,  
                                 QObject *parent)
    : QObject(parent),
      m_playlist(playlist),
      m_openButton(openButton),
      m_closeButton(closeButton),
      m_importButton(importButton),  
      m_exportButton(exportButton),  
      m_visible(false)
{
    // 初始化动画组
    m_animationGroup = new QParallelAnimationGroup(this);

    // 播放列表动画
    m_playlistAnimation = new QPropertyAnimation(m_playlist, "geometry", this);
    m_playlistAnimation->setDuration(300);
    m_animationGroup->addAnimation(m_playlistAnimation);

    // 关闭按钮动画
    m_closeButtonAnimation = new QPropertyAnimation(m_closeButton, "geometry", this);
    m_closeButtonAnimation->setDuration(300);
    m_animationGroup->addAnimation(m_closeButtonAnimation);

    // 导入按钮动画  // 新增
    m_importButtonAnimation = new QPropertyAnimation(m_importButton, "geometry", this);
    m_importButtonAnimation->setDuration(300);
    m_animationGroup->addAnimation(m_importButtonAnimation);

    // 导出按钮动画  // 新增
    m_exportButtonAnimation = new QPropertyAnimation(m_exportButton, "geometry", this);
    m_exportButtonAnimation->setDuration(300);
    m_animationGroup->addAnimation(m_exportButtonAnimation);

    // 保存播放列表的位置信息
    m_playlistVisibleRect = m_playlist->geometry();
    m_playlistHiddenRect = m_playlistVisibleRect;
    m_playlistHiddenRect.moveTo(800, m_playlistHiddenRect.y());

    // 保存关闭按钮的位置信息
    m_closeButtonVisibleRect = m_closeButton->geometry();
    m_closeButtonHiddenRect = m_closeButtonVisibleRect;
    m_closeButtonHiddenRect.moveTo(
        m_closeButtonVisibleRect.x() + (800 - m_playlistVisibleRect.x()),
        m_closeButtonVisibleRect.y()
    );

    // 保存导入按钮的位置信息  
    m_importButtonVisibleRect = m_importButton->geometry();
    m_importButtonHiddenRect = m_importButtonVisibleRect;
    m_importButtonHiddenRect.moveTo(
        m_importButtonVisibleRect.x() + (800 - m_playlistVisibleRect.x()),
        m_importButtonVisibleRect.y()
    );

    // 保存导出按钮的位置信息  
    m_exportButtonVisibleRect = m_exportButton->geometry();
    m_exportButtonHiddenRect = m_exportButtonVisibleRect;
    m_exportButtonHiddenRect.moveTo(
        m_exportButtonVisibleRect.x() + (800 - m_playlistVisibleRect.x()),
        m_exportButtonVisibleRect.y()
    );

    // 初始状态：播放列表和按钮隐藏
    m_playlist->setGeometry(m_playlistHiddenRect);
    m_closeButton->setGeometry(m_closeButtonHiddenRect);
    m_importButton->setGeometry(m_importButtonHiddenRect); 
    m_exportButton->setGeometry(m_exportButtonHiddenRect);  
    m_closeButton->hide();
    m_importButton->hide(); 
    m_exportButton->hide();  
}

void PlaylistAnimator::adjustZOrder()
{
    QWidget *topLevel = m_playlist->window();
    m_playlist->raise();
    m_openButton->raise();
    m_closeButton->raise();
    m_importButton->raise();  
    m_exportButton->raise(); 
    if (QWidget *closeBtn = topLevel->findChild<QWidget*>("CloseButton")) {
        closeBtn->raise();
    }
}

void PlaylistAnimator::togglePlaylist()
{
    adjustZOrder();  // 确保正确的图层顺序

    m_visible = !m_visible;

    if (m_visible) {
        // 显示播放列表和按钮
        m_playlistAnimation->setStartValue(m_playlist->geometry());
        m_playlistAnimation->setEndValue(m_playlistVisibleRect);

        m_closeButtonAnimation->setStartValue(m_closeButton->geometry());
        m_closeButtonAnimation->setEndValue(m_closeButtonVisibleRect);

        m_importButtonAnimation->setStartValue(m_importButton->geometry());  
        m_importButtonAnimation->setEndValue(m_importButtonVisibleRect);    

        m_exportButtonAnimation->setStartValue(m_exportButton->geometry());  
        m_exportButtonAnimation->setEndValue(m_exportButtonVisibleRect);    

        m_animationGroup->start();

        // 切换按钮状态
        m_openButton->hide();
        m_closeButton->show();
        m_importButton->show();  
        m_exportButton->show();  
    } else {
        // 隐藏播放列表和按钮
        m_playlistAnimation->setStartValue(m_playlist->geometry());
        m_playlistAnimation->setEndValue(m_playlistHiddenRect);

        m_closeButtonAnimation->setStartValue(m_closeButton->geometry());
        m_closeButtonAnimation->setEndValue(m_closeButtonHiddenRect);

        m_importButtonAnimation->setStartValue(m_importButton->geometry()); 
        m_importButtonAnimation->setEndValue(m_importButtonHiddenRect);      

        m_exportButtonAnimation->setStartValue(m_exportButton->geometry());  
        m_exportButtonAnimation->setEndValue(m_exportButtonHiddenRect);      

        m_animationGroup->start();

        // 切换按钮状态
        m_closeButton->hide();
        m_importButton->hide();  
        m_exportButton->hide();  
        m_openButton->show();
    }
}

bool PlaylistAnimator::isPlaylistVisible() const
{
    return m_visible;
}