#include "music.h"
#include "ui_music.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMenu>
#include <QInputDialog>
#include <QDateTime>
#include <QTimer>
#include <QEventLoop>
#include <QFontMetrics>

Music::Music(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Music)
    ,m_currentLyricIndex(-1)
    ,m_scrollPos(0)
    ,m_floatLyrics(nullptr)
    ,m_floatLyricsVisible(false)
    , m_floatLyricsLocked(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setWindowIcon(QIcon(":/icon/app1.ico"));

    // 确保 mp3 文件夹存在
    QString mp3DirPath = QCoreApplication::applicationDirPath() + "/mp3";
    QDir mp3Dir(mp3DirPath);
    if (!mp3Dir.exists()) {
        if (!mp3Dir.mkpath(".")) {
            qWarning() << "无法创建 mp3 文件夹:" << mp3DirPath;
            QMessageBox::warning(this, "警告", "无法创建 mp3 文件夹，本地音乐功能可能无法使用");
        } else {
            qDebug() << "已创建 mp3 文件夹:" << mp3DirPath;
        }
    }

    //禁用歌词下拉条
    ui->tab_2_Lyrics->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tab_2_Lyrics->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //禁用歌词文字选中
    ui->tab_2_Lyrics->setTextInteractionFlags(Qt::NoTextInteraction);

    // 初始化管理器
    m_playlistManager = new PlaylistManager(this);
    m_localMusicManager = new LocalMusicManager(mp3DirPath, this);
    m_player = new MusicPlayer(this);

    // 初始化本地歌曲列表
    m_localSongs = m_localMusicManager->songs(); // 保存完整列表
    ui->tab_3_LocalLists->addItems(m_localSongs); // 添加到UI

    // 创建播放列表动画管理器
    m_playlistAnimator = new PlaylistAnimator(
        ui->Playlist,
        ui->PlaylistOpenButton,
        ui->PlaylistCloseButton,
        ui->PlaylistImport,
        ui->PlaylistExport,
        this
    );

    // 创建窗口拖动控制器
    m_dragController = new WindowDragController(this, this);

    // 添加需要排除拖动的控件
    m_dragController->addExcludedWidget(ui->CloseButton);
    m_dragController->addExcludedWidget(ui->PlaylistOpenButton);
    m_dragController->addExcludedWidget(ui->PlaylistCloseButton);
    m_dragController->addExcludedWidget(ui->MusicPlaybackProgress);

    // 设置连接
    setupConnections();

    // 初始化UI
    updatePlayButtonIcon();
    ui->MusicPlaybackProgress->setValue(0);

    // 设置列表的上下文菜单策略
    ui->Playlist->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tab_3_LocalLists->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Playlist, &QListWidget::customContextMenuRequested, this, &Music::showPlaylistContextMenu);
    connect(ui->tab_3_LocalLists, &QListWidget::customContextMenuRequested, this, &Music::showLocalListContextMenu);

    // 初始化滚动标题计时器
    m_titleScrollTimer = new QTimer(this);
    connect(m_titleScrollTimer, &QTimer::timeout, this, &Music::updateScrollingTitle);
    m_titleScrollTimer->start(200); // 每200毫秒滚动一次
}


// 实现歌词解析函数
void Music::parseLyrics(const QString& lyrics)
{
    m_lyricLines.clear();
    m_currentLyricIndex = -1;

    QStringList lines = lyrics.split('\n');
    // 修复正则表达式：支持分钟、秒、毫秒
    QRegularExpression regex("\\[(\\d+):(\\d+)\\.?(\\d*)\\](.*)");

    for (const QString& line : lines) {
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            int minutes = match.captured(1).toInt();
            int seconds = match.captured(2).toInt();
            int milliseconds = 0;

            // 处理毫秒部分（可能为0-3位）
            QString msStr = match.captured(3);
            if (!msStr.isEmpty()) {
                if (msStr.length() == 2) {
                    milliseconds = msStr.toInt() * 10;
                } else if (msStr.length() == 3) {
                    milliseconds = msStr.toInt();
                } else if (msStr.length() == 1) {
                    milliseconds = msStr.toInt() * 100;
                }
            }

            QString text = match.captured(4).trimmed();
            if (!text.isEmpty()) {
                qint64 time = (minutes * 60000) + (seconds * 1000) + milliseconds;
                m_lyricLines.append(qMakePair(time, text));
            }
        }
    }

    // 按时间排序
    std::sort(m_lyricLines.begin(), m_lyricLines.end(),
              [](const QPair<qint64, QString> &a, const QPair<qint64, QString> &b) {
                  return a.first < b.first;
              });
}

// 实现歌词更新函数
void Music::updateLyricsDisplay(qint64 position)
{
    if (m_lyricLines.isEmpty()) {
        // 显示暂无歌词
        QString html = QString("<p style='color: #FFFFFF; font-size: 16pt; text-align: center;'>暂无歌词</p>");
        ui->tab_2_Lyrics->setHtml(html);

        // 更新浮动歌词窗口
        if (m_floatLyrics) {
            m_floatLyrics->updateLyricsDisplay(position);
        }
        return;
    }

    // 查找当前应该显示的歌词行
    int newIndex = -1;
    for (int i = 0; i < m_lyricLines.size(); ++i) {
        if (m_lyricLines[i].first <= position) {
            newIndex = i;
        } else {
            break;
        }
    }

    // 如果当前行没有变化，则不需要更新
    if (newIndex == m_currentLyricIndex) {
        return;
    }

    m_currentLyricIndex = newIndex;

    // 构建歌词HTML（只显示当前行和前/后两行）
    QString lyricsHtml;
    int startIdx = qMax(0, m_currentLyricIndex - 2);
    int endIdx = qMin(m_lyricLines.size() - 1, m_currentLyricIndex + 2);

    for (int i = startIdx; i <= endIdx; ++i) {
        if (i == m_currentLyricIndex) {
            lyricsHtml += "<p style='color: #1DB954; font-size: 18pt; text-align: center; margin: 10px 0;'>" +
                          m_lyricLines[i].second + "</p>";
        } else {
            lyricsHtml += "<p style='color: #FFFFFF; font-size: 14pt; text-align: center; margin: 5px 0;'>" +
                          m_lyricLines[i].second + "</p>";
        }
    }

    ui->tab_2_Lyrics->setHtml(lyricsHtml);

    // 更新浮动歌词窗口
    if (m_floatLyrics) {
        m_floatLyrics->updateLyricsDisplay(position);
    }
}

/**
 * @brief 设置所有信号槽连接
 */
void Music::setupConnections()
{
    connect(ui->player, &QPushButton::clicked, this, &Music::togglePlayPause);
    connect(ui->NextTrack, &QPushButton::clicked, this, &Music::playNext);
    connect(ui->PreviousTrack, &QPushButton::clicked, this, &Music::playPrevious);
    connect(ui->PlaylistOpenButton, &QPushButton::clicked, this, &Music::togglePlaylist);
    connect(ui->PlaylistCloseButton, &QPushButton::clicked, this, &Music::togglePlaylist);
    connect(ui->CloseButton, &QPushButton::clicked, this, &Music::onCloseButtonClicked);
    connect(ui->tab_3_SearchLineEdit, &QLineEdit::textChanged,this, &Music::onLocalSearchTextChanged);
    connect(ui->tab_2_FloatingLyricsButton, &QPushButton::clicked, this, &Music::toggleFloatingLyrics);
    connect(ui->tab_5_LyricsHoveringWindowControlTransparencySlider, &QSlider::valueChanged,this, &Music::setFloatingLyricsOpacity);
    connect(ui->tab_2_Lock, &QPushButton::clicked, this, &Music::onLockButtonClicked);
    connect(m_player, &MusicPlayer::positionChanged, this, &Music::updatePlaybackPosition);
    connect(m_player, &MusicPlayer::durationChanged, this, &Music::updateDuration);
    connect(m_player, &MusicPlayer::stateChanged, this, &Music::updatePlayButtonIcon);
    connect(m_player, &MusicPlayer::currentMediaChanged, this, &Music::updateCurrentSongInfo);
    connect(m_player, &MusicPlayer::mediaFinished, this, &Music::playNext);
    connect(m_player, &MusicPlayer::positionChanged, this, [this](qint64 position) {updatePlaybackPosition(position); updateLyricsDisplay(position);});
    connect(ui->PlaylistImport, &QPushButton::clicked, this, &Music::importPlaylist);
    connect(ui->PlaylistExport, &QPushButton::clicked, this, &Music::exportPlaylist);

    // 播放列表信号
    connect(m_playlistManager, &PlaylistManager::playlistChanged, [this]() {
        ui->Playlist->clear();
        for (int i = 0; i < m_playlistManager->count(); ++i) {
            auto item = m_playlistManager->itemAt(i);
            QFileInfo fileInfo(item.filePath);
            ui->Playlist->addItem(fileInfo.fileName());
        }
        // 同步播放列表到播放器
        m_player->setPlaylist(m_playlistManager->items());
    });
    
    connect(m_localMusicManager, &LocalMusicManager::libraryUpdated, this, &Music::updateLibrary);
    connect(ui->Playlist, &QListWidget::itemDoubleClicked, this, &Music::handlePlaylistItemDoubleClicked);
    connect(ui->MusicPlaybackProgress, &QSlider::sliderMoved, m_player, &MusicPlayer::setPosition);
    connect(m_player, &MusicPlayer::mediaError, this, [this](const QString& error) {QMessageBox::critical(this, "播放错误", error);});
}

/**
 * @brief 析构函数，清理资源
 */
Music::~Music()
{
    delete m_dragController;
    delete m_playlistAnimator;
    delete m_player;
    delete m_playlistManager;
    delete m_localMusicManager;
    delete m_floatLyrics;
    delete ui;
}

/**
 * @brief 切换播放列表可见性
 */
void Music::togglePlaylist()
{
    m_playlistAnimator->togglePlaylist();
}

/**
 * @brief 关闭按钮点击处理(保证悬浮歌词不会遮挡关闭弹窗)
 */
void Music::onCloseButtonClicked()
{
    // 如果浮动歌词窗口存在且可见，暂时移除其置顶属性
    if (m_floatLyrics && m_floatLyricsVisible) {
        Qt::WindowFlags flags = m_floatLyrics->windowFlags();
        m_floatLyrics->setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
        m_floatLyrics->show(); // 需要重新显示窗口使标志生效
    }

    QMessageBox::StandardButton reply;
    QMessageBox msgBox(this);
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint); // 确保弹窗置顶
    msgBox.setWindowTitle("不要走嘛~");
    msgBox.setText("确定要退出程序吗？");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    reply = static_cast<QMessageBox::StandardButton>(msgBox.exec());

    // 恢复浮动歌词窗口的置顶属性
    if (m_floatLyrics && m_floatLyricsVisible) {
        Qt::WindowFlags flags = m_floatLyrics->windowFlags();
        m_floatLyrics->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
        m_floatLyrics->show(); // 重新显示窗口使标志生效
    }

    if (reply == QMessageBox::Yes) {
        close();
    }
}

/**
 * @brief 格式化时间（毫秒转换为分:秒）
 * @param milliseconds 毫秒数
 * @return 格式化后的时间字符串
 */
QString Music::formatTime(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;

    return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

/**
 * @brief 更新播放按钮图标（播放/暂停状态）
 */
void Music::updatePlayButtonIcon()
{
    if (m_player->isPlaying()) {
        ui->player->setStyleSheet("border-image: url(:/icon/pause.png);");
    } else {
        ui->player->setStyleSheet("border-image: url(:/icon/play.png);");
    }
}

/**
 * @brief 切换播放/暂停状态
 */
void Music::togglePlayPause()
{
    if (m_player->isPlaying()) {
        m_player->pause();
    } else {
        // 如果没有正在播放的歌曲，从播放列表第一首开始
        if (m_currentPlayIndex < 0 && m_playlistManager->count() > 0) {
            handlePlaylistItemDoubleClicked(ui->Playlist->item(0));
        } else if (m_playlistManager->count() > 0) {
            m_player->play();
        }
    }
}

/**
 * @brief 播放下一首
 */
void Music::playNext()
{
    if (m_playlistManager->count() == 0) return;

    int nextIndex = (m_currentPlayIndex + 1) % m_playlistManager->count();
    handlePlaylistItemDoubleClicked(ui->Playlist->item(nextIndex));
}

/**
 * @brief 播放上一首
 */
void Music::playPrevious()
{
    if (m_playlistManager->count() == 0) return;

    int prevIndex = (m_currentPlayIndex - 1 + m_playlistManager->count()) % m_playlistManager->count();
    handlePlaylistItemDoubleClicked(ui->Playlist->item(prevIndex));
}

/**
 * @brief 更新播放位置
 * @param position 当前播放位置（毫秒）
 */
void Music::updatePlaybackPosition(qint64 position)
{
    if (!ui->MusicPlaybackProgress->isSliderDown()) {
        ui->MusicPlaybackProgress->setValue(position);
    }
    updateTimeDisplay();
}

/**
 * @brief 更新歌曲总时长
 * @param duration 歌曲总时长（毫秒）
 */
void Music::updateDuration(qint64 duration)
{
    ui->MusicPlaybackProgress->setRange(0, duration);
    updateTimeDisplay();
}

/**
 * @brief 更新时间显示
 */
void Music::updateTimeDisplay()
{
    qint64 position = m_player->position();
    qint64 duration = m_player->duration();

    ui->timeLabel->setText(
        formatTime(position) + " / " + formatTime(duration)
    );
}

/**
 * @brief 处理播放列表项双击事件
 * @param item 被双击的列表项
 */
void Music::handlePlaylistItemDoubleClicked(QListWidgetItem *item)
{
    int index = ui->Playlist->row(item);
    if (index >= 0 && index < m_playlistManager->count()) {
        auto playlistItem = m_playlistManager->itemAt(index);
        QFileInfo fileInfo(playlistItem.filePath);
        QString songTitle = fileInfo.completeBaseName();

        // 更新曲名显示 - 使用新的控件名称
        ui->tab_2_SongTitle->setText(songTitle);
        ui->SongTitle->setText(songTitle);
        
        // 检查文件是否存在
        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "错误", "文件不存在: " + playlistItem.filePath);
            return;
        }
        
        // 检查格式支持
        if (!MusicPlayer::isFormatSupported(playlistItem.filePath)) {
            QMessageBox::warning(this, "错误", 
                "不支持的音频格式: " + fileInfo.suffix() + "\n文件: " + fileInfo.fileName());
            return;
        }

        m_currentPlayIndex = index;
        m_player->setCurrentIndex(index);
        m_player->setVolume(playlistItem.volume);
        m_player->play();
    }
}
/**
 * @brief 更新本地音乐库显示
 */
void Music::updateLibrary()
{
    m_localSongs = m_localMusicManager->songs(); // 保存完整列表
    ui->tab_3_LocalLists->clear();
    ui->tab_3_LocalLists->addItems(m_localSongs);

    // 如果有搜索文本，重新应用搜索过滤
    if (!ui->tab_3_SearchLineEdit->text().isEmpty()) {
        onLocalSearchTextChanged(ui->tab_3_SearchLineEdit->text());
    }
}

/**
 * @brief 更新当前歌曲信息（包括歌词）
 */
void Music::updateCurrentSongInfo()
{
    if (m_currentPlayIndex >= 0 && m_currentPlayIndex < m_playlistManager->count()) {
        auto item = m_playlistManager->itemAt(m_currentPlayIndex);
        QFileInfo fileInfo(item.filePath);
        QString songTitle = fileInfo.completeBaseName();

        // 设置完整标题用于滚动
        m_currentTitle = songTitle;
        m_scrollPos = 0;  // 重置滚动位置
        m_scrollPos2 = 0; // 重置第二个滚动位置

        // 初始显示完整标题
        ui->SongTitle->setText(songTitle);
        ui->tab_2_SongTitle->setText(songTitle);

        QString lyricsPath = fileInfo.path() + "/" + songTitle + ".lrc";

        QFile file(lyricsPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
             stream.setCodec("UTF-8");
#else
             stream.setEncoding(QStringConverter::Utf8);
#endif
             QString lyrics = stream.readAll();
             parseLyrics(lyrics);

             // 将歌词传递给浮动歌词窗口
             if (m_floatLyrics) {
                 m_floatLyrics->parseLyrics(lyrics);
             }

             file.close();
         }
         else {
            m_lyricLines.clear();
            QString html = QString("<p style='color: #FFFFFF; font-size: 16pt; text-align: center;'>暂无歌词</p>");
            ui->tab_2_Lyrics->setHtml(html);

            // 清空浮动歌词窗口的歌词
            if (m_floatLyrics) {
                m_floatLyrics->clearLyrics();
            }
        }
    }
    // 更新封面
    if (m_currentPlayIndex >= 0 && m_currentPlayIndex < m_playlistManager->count()) {
        auto item = m_playlistManager->itemAt(m_currentPlayIndex);
        updateCoverImage(item.filePath);
    }

    // 强制更新一次歌词显示
    updateLyricsDisplay(m_player->position());
}
/**
 * @brief 显示播放列表的右键菜单
 * @param pos 鼠标位置
 */
void Music::showPlaylistContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->Playlist->itemAt(pos);
    if (!item) return;

    int index = ui->Playlist->row(item);
    if (index < 0 || index >= m_playlistManager->count()) return;

    QMenu menu;
    
    // 添加菜单项
    QAction *moveUpAction = menu.addAction("上移");
    QAction *moveDownAction = menu.addAction("下移");
    QAction *adjustVolumeAction = menu.addAction("调节音量");
    QAction *removeAction = menu.addAction("删除");
    
    // 显示菜单并获取选择
    QAction *selectedAction = menu.exec(ui->Playlist->viewport()->mapToGlobal(pos));
    
    // 处理菜单选择
    if (selectedAction == moveUpAction) {
        if (index > 0) {
            m_playlistManager->moveItem(index, index - 1);
        }
    } else if (selectedAction == moveDownAction) {
        if (index < m_playlistManager->count() - 1) {
            m_playlistManager->moveItem(index, index + 1);
        }
    } else if (selectedAction == adjustVolumeAction) {
        bool ok;
        int currentVolume = m_playlistManager->itemAt(index).volume;
        int newVolume = QInputDialog::getInt(this, "调节音量", "音量 (0-100):", 
                                           currentVolume, 0, 100, 1, &ok);
        if (ok) {
            m_playlistManager->setVolume(index, newVolume);
            // 如果是当前播放的歌曲，立即更新音量
            if (index == m_currentPlayIndex) {
                m_player->setVolume(newVolume);
            }
        }
    } else if (selectedAction == removeAction) {
        m_playlistManager->removeItem(index);
        // 如果是当前播放的歌曲，停止播放
        if (index == m_currentPlayIndex) {
            m_player->stop();
            m_currentPlayIndex = -1;
        }
    }
}

/**
 * @brief 显示本地列表的右键菜单
 * @param pos 鼠标位置
 */
void Music::showLocalListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->tab_3_LocalLists->itemAt(pos);
    if (!item) return;

    QString fileName = item->text();
    QString filePath = m_localMusicManager->directory() + "/" + fileName;
    
    QMenu menu;
    QAction *addToPlaylistAction = menu.addAction("添加到播放列表");
    
    // 检查格式支持
    if (!MusicPlayer::isFormatSupported(filePath)) {
        addToPlaylistAction->setEnabled(false);
        addToPlaylistAction->setToolTip("不支持的音频格式: " + QFileInfo(filePath).suffix());
    }
    
    QAction *selectedAction = menu.exec(ui->tab_3_LocalLists->viewport()->mapToGlobal(pos));
    
    if (selectedAction == addToPlaylistAction) {
        m_playlistManager->addItem(filePath);
    }
}

/**
 * @brief 导出播放列表
 */
void Music::exportPlaylist()
{
    // 获取保存文件路径
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出播放列表",
        QDir::homePath() + "/playlist.txt",
        "文本文件 (*.txt)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建文件");
        return;
    }

    QTextStream stream(&file);
    // 写入播放列表数据
    for (int i = 0; i < m_playlistManager->count(); ++i) {
        auto item = m_playlistManager->itemAt(i);
        QFileInfo fileInfo(item.filePath);
        stream << fileInfo.fileName() << "|" << item.volume << "\n";
    }
    file.close();
    QMessageBox::information(this, "成功", "播放列表已导出");

    // 询问是否分享
    QMessageBox::StandardButton shareReply;
    shareReply = QMessageBox::question(this, "分享播放列表", "是否将播放列表、歌曲和封面打包分享?",QMessageBox::Yes | QMessageBox::No);

    if (shareReply == QMessageBox::Yes) {
        sharePlaylist(filePath); // 调用分享功能
    }
}

/**
 * @brief 导入播放列表
 */
void Music::importPlaylist()
{
    // 检查当前播放列表是否有歌曲
    if (m_playlistManager->count() > 0) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("播放列表有歌曲呢");
        msgBox.setText("当前播放列表中有歌曲，是否保存?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.setButtonText(QMessageBox::Save, "保存并导入");
        msgBox.setButtonText(QMessageBox::Discard, "不保存直接导入");
        msgBox.setButtonText(QMessageBox::Cancel, "取消导入");

        int choice = msgBox.exec();

        if (choice == QMessageBox::Cancel) {
            return; // 用户取消操作
        }
        else if (choice == QMessageBox::Save) {
            // 用户选择保存
            exportPlaylist();
        }

        // 清空当前播放列表（无论用户选择保存还是直接导入）
        m_playlistManager->clear();
    }

    // 获取打开文件路径
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "导入播放列表",
        QDir::homePath(),
        "文本文件 (*.txt)"
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList parts = line.split("|");

        if (parts.size() < 2) continue; // 跳过无效行

        QString fileName = parts[0];
        bool ok;
        int volume = parts[1].toInt(&ok);

        // 验证音量值
        if (!ok || volume < 0 || volume > 100) {
            volume = 50; // 使用默认音量
        }

        // 构建完整文件路径
        QString fullPath = m_localMusicManager->directory() + "/" + fileName;

        // 添加到播放列表
        if (QFile::exists(fullPath)) {
            m_playlistManager->addItem(fullPath, volume);
        }
    }
    file.close();
    QMessageBox::information(this, "成功", "播放列表已导入");
}

/**
 * @brief 打包分享
 */
void Music::sharePlaylist(const QString& playlistFilePath)
{
    // 获取保存ZIP文件路径
    QString zipPath = QFileDialog::getSaveFileName(
        this,
        "保存分享包",
        QDir::homePath() + "/playlist_share.zip",
        "ZIP文件 (*.zip)"
    );

    if (zipPath.isEmpty()) return;

    // 创建临时目录 - 使用普通目录代替 QTemporaryDir
    QString tempPath = QDir::tempPath() + "/" + "music_player_temp_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir tempDir;
    if (!tempDir.mkpath(tempPath)) {
        QMessageBox::warning(this, "错误", "无法创建临时目录");
        return;
    }

    // 复制播放列表文件到临时目录
    QString playlistFileName = QFileInfo(playlistFilePath).fileName();
    if (!QFile::copy(playlistFilePath, tempPath + "/" + playlistFileName)) {
        QMessageBox::warning(this, "错误", "无法复制播放列表文件");
        return;
    }

    // 创建进度对话框
    QProgressDialog progress("打包分享文件中...", "取消", 0, m_playlistManager->count(), this);
    progress.setWindowModality(Qt::WindowModal);

    // 遍历播放列表添加歌曲和资源
    for (int i = 0; i < m_playlistManager->count(); ++i) {
        progress.setValue(i);
        if (progress.wasCanceled()) {
            // 删除临时目录
            QDir(tempPath).removeRecursively();
            return;
        }

        auto item = m_playlistManager->itemAt(i);
        QFileInfo fileInfo(item.filePath);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();

        // 复制歌曲文件
        if (!QFile::copy(item.filePath, tempPath + "/" + fileInfo.fileName())) {
            qWarning() << "Failed to copy song file:" << item.filePath;
        }

        // 复制歌词文件（如果存在）
        QString lyricPath = dirPath + "/" + baseName + ".lrc";
        if (QFile::exists(lyricPath)) {
            if (!QFile::copy(lyricPath, tempPath + "/" + QFileInfo(lyricPath).fileName())) {
                qWarning() << "Failed to copy lyric file:" << lyricPath;
            }
        }

        // 复制封面文件（如果存在）
        QString coverPath = dirPath + "/" + baseName + ".png";
        if (!QFile::exists(coverPath)) {
            coverPath = dirPath + "/" + baseName + ".jpg"; // 尝试jpg格式
        }
        if (QFile::exists(coverPath)) {
            if (!QFile::copy(coverPath, tempPath + "/" + QFileInfo(coverPath).fileName())) {
                qWarning() << "Failed to copy cover file:" << coverPath;
            }
        }
    }
    progress.setValue(m_playlistManager->count());

    // 使用系统命令创建ZIP
    QProcess zipProcess;
    QString program;
    QStringList arguments;

#ifdef Q_OS_WIN
    // Windows 使用 PowerShell
    program = "powershell";
    arguments << "-Command"
              << QString("Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force")
                 .arg(tempPath.replace("/", "\\"))
                 .arg(zipPath.replace("/", "\\"));
#else
    // Linux/macOS 使用 zip
    program = "zip";
    arguments << "-r" << zipPath << tempPath;
#endif

    zipProcess.start(program, arguments);
    if (!zipProcess.waitForStarted()) {
        QMessageBox::warning(this, "错误", "无法启动压缩进程");
        // 删除临时目录
        QDir(tempPath).removeRecursively();
        return;
    }

    // 等待压缩完成，使用简单的循环代替 QThread::msleep
    QProgressDialog zipProgress("正在创建压缩包...", "取消", 0, 0, this);
    zipProgress.setWindowModality(Qt::WindowModal);

    while (zipProcess.state() == QProcess::Running) {
        QCoreApplication::processEvents(); // 处理事件循环

        if (zipProgress.wasCanceled()) {
            zipProcess.kill();
            QFile::remove(zipPath); // 删除未完成的ZIP文件
            // 删除临时目录
            QDir(tempPath).removeRecursively();
            return;
        }

        // 使用 QTimer 延迟代替 msleep
        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(100); // 100ms 延迟
        loop.exec();
    }

    if (zipProcess.exitCode() != 0) {
        QMessageBox::warning(this, "错误",
                            QString("压缩失败: %1").arg(QString::fromUtf8(zipProcess.readAllStandardError())));
        // 删除临时目录
        QDir(tempPath).removeRecursively();
        return;
    }

    // 删除临时目录
    if (!QDir(tempPath).removeRecursively()) {
        qWarning() << "Failed to remove temporary directory:" << tempPath;
    }

    QMessageBox::information(this, "成功", "分享包已创建: " + zipPath);
}

/**
 * @brief 本地音乐搜索
 */
void Music::onLocalSearchTextChanged(const QString &text)
{
    // 如果搜索框为空，显示全部歌曲
    if (text.isEmpty()) {
        ui->tab_3_LocalLists->clear();
        ui->tab_3_LocalLists->addItems(m_localSongs); // 使用保存的完整列表
        return;
    }

    // 执行模糊搜索
    QStringList filteredSongs; // 修复变量名（移除多余的换行符）
    for (const QString &song : m_localSongs) {
        if (song.contains(text, Qt::CaseInsensitive)) {
            filteredSongs.append(song);
        }
    }

    // 更新列表显示
    ui->tab_3_LocalLists->clear();
    ui->tab_3_LocalLists->addItems(filteredSongs);
}

void Music::updateCoverImage(const QString& songFilePath)
{
    QFileInfo fileInfo(songFilePath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();

    // 尝试加载PNG封面
    QString coverPath = dirPath + "/" + baseName + ".png";
    if (!QFile::exists(coverPath)) {
        // 如果PNG不存在，尝试加载JPG封面
        coverPath = dirPath + "/" + baseName + ".jpg";
    }

    QPixmap coverPixmap;
    if (QFile::exists(coverPath) && coverPixmap.load(coverPath)) {
        // 缩放封面以适应标签大小并保持比例
        coverPixmap = coverPixmap.scaled(ui->MusicCover->width(),
                                        ui->MusicCover->height(),
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation);
        ui->MusicCover->setPixmap(coverPixmap);
    } else {
        // 没有封面时显示默认图标
        ui->MusicCover->setPixmap(QPixmap(":/icon/default_cover.png"));
    }
}

/**
 * @brief 更新滚动歌曲标题
 */
void Music::updateScrollingTitle()
{
    if (m_currentTitle.isEmpty()) {
        ui->SongTitle->setText("");
        ui->tab_2_SongTitle->setText("");
        return;
    }

    // 创建滚动文本：曲名 + 10个空格 + 曲名 + 10个空格 + 曲名
    const QString unit = m_currentTitle + QString(10, ' ');
    const int repeatCount = 3; // 重复3次
    QString displayText;
    for (int i = 0; i < repeatCount; i++) {
        displayText += unit;
    }
    int totalLen = displayText.length();

    // 更新底部曲名标签
    updateLabelScroll(ui->SongTitle, displayText, totalLen, m_scrollPos);

    // 更新大标题曲名标签
    updateLabelScroll(ui->tab_2_SongTitle, displayText, totalLen, m_scrollPos2);

    // 更新滚动位置
    m_scrollPos = (m_scrollPos + 1) % totalLen;
    m_scrollPos2 = (m_scrollPos2 + 1) % totalLen;
}

void Music::updateLabelScroll(QLabel *label, const QString &displayText, int totalLen, int &pos)
{
    if (displayText.isEmpty() || totalLen == 0) {
        label->setText("");
        return;
    }

    // 从当前位置开始截取足够长的文本
    QString visibleText;
    int start = pos;
    for (int i = 0; i < totalLen; i++) {
        int index = (start + i) % totalLen;
        visibleText += displayText[index];
    }

    // 设置标签文本
    label->setText(visibleText);
}

void Music::toggleFloatingLyrics()
{
    if (!m_floatLyrics) {
        // 第一次使用时创建浮动歌词窗口
        m_floatLyrics = new FloatingLyrics(this);
        m_floatLyrics->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

        // 同步锁定状态
        if (m_floatLyricsLocked) {
            m_floatLyrics->setClickThrough(true);
            ui->tab_2_Lock->setStyleSheet("border-image: url(:/icon/lock.png);");
        } else {
            m_floatLyrics->setClickThrough(false);
            ui->tab_2_Lock->setStyleSheet("border-image: url(:/icon/unlock.png);");
        }
    }

    if (m_floatLyricsVisible) {
        // 隐藏窗口
        m_floatLyrics->hide();
        m_floatLyricsVisible = false;
    } else {
        // 显示窗口并定位到合适位置
        QPoint pos = this->pos();
        pos.setX(pos.x() + this->width() + 10); // 在主窗口右侧显示
        pos.setY(pos.y());
        m_floatLyrics->move(pos);
        m_floatLyrics->show();
        m_floatLyricsVisible = true;

        // 强制更新一次歌词显示
        m_floatLyrics->updateLyricsDisplay(m_player->position());
    }
}
void Music::setFloatingLyricsOpacity(int opacity)
{
    if (m_floatLyrics) {
        m_floatLyrics->setOpacity(opacity);
    }
}

void Music::onLockButtonClicked()
{
    m_floatLyricsLocked = !m_floatLyricsLocked;

    if (m_floatLyrics) {
        if (m_floatLyricsLocked) {
            ui->tab_2_Lock->setStyleSheet("border-image: url(:/icon/lock.png);");
            // 启用鼠标穿透
            m_floatLyrics->setClickThrough(true);
        } else {
            ui->tab_2_Lock->setStyleSheet("border-image: url(:/icon/unlock.png);");
            // 禁用鼠标穿透
            m_floatLyrics->setClickThrough(false);
        }
    }
}
