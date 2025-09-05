#include <QIcon>
#include "floatinglyrics.h"
#include "ui_floatinglyrics.h"
#include <QMouseEvent>
#include <QGraphicsEffect>
#include <QRegularExpression>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief FloatingLyrics 构造函数
 * @param parent 父窗口指针
 */
FloatingLyrics::FloatingLyrics(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FloatingLyrics),
    m_dragging(false),
    m_opacity(80),
    m_clickThrough(false),
    m_currentLyricIndex(-1)
{
    ui->setupUi(this);
    // 设置窗口属性：无边框、工具窗口、置顶
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowIcon(QIcon(":/icon/app1.ico"));
    // 启用透明背景
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    // 禁止文本交互（防止文本被选中）
    ui->Lyrics->setTextInteractionFlags(Qt::NoTextInteraction);

    // 安装事件过滤器
    installEventFilter(this);

    // 设置初始透明度
    setOpacity(m_opacity);

    // 设置浮动歌词窗口样式
    ui->Lyrics->setStyleSheet(
        "QTextBrowser {"
        "   background-color: rgba(30, 30, 30, 200);"
        "   color: #FFFFFF;"
        "   border: 2px solid #383838;"
        "   border-radius: 10px;"
        "}"
    );
}

/**
 * @brief FloatingLyrics 析构函数
 */
FloatingLyrics::~FloatingLyrics()
{
    delete ui;
}

/**
 * @brief 设置窗口透明度
 * @param opacity 透明度值 (0-100)
 */
void FloatingLyrics::setOpacity(int opacity)
{
    m_opacity = opacity;
    float opacityValue = static_cast<float>(opacity) / 100.0f;

    // 设置窗口透明度
    setWindowOpacity(opacityValue);

    // 设置歌词文本的透明度
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(opacityValue);
    ui->Lyrics->setGraphicsEffect(effect);
}

/**
 * @brief 设置鼠标穿透效果
 * @param enable 是否启用鼠标穿透
 */
void FloatingLyrics::setClickThrough(bool enable)
{
    m_clickThrough = enable;

#ifdef Q_OS_WIN
    // Windows平台使用API实现真正的鼠标穿透
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (enable) {
        // 设置窗口为分层窗口并启用鼠标穿透
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
    } else {
        // 移除鼠标穿透属性
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    }
#endif
}

/**
 * @brief 解析歌词内容
 * @param lyrics 歌词字符串
 */
void FloatingLyrics::parseLyrics(const QString& lyrics)
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

/**
 * @brief 更新歌词显示
 * @param position 当前播放位置（毫秒）
 */
void FloatingLyrics::updateLyricsDisplay(qint64 position)
{
    if (m_lyricLines.isEmpty()) {
        // 显示暂无歌词
        QString html = QString("<p style='color: #FFFFFF; font-size: 16pt; text-align: center;'>暂无歌词</p>");
        ui->Lyrics->setHtml(html);
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
            lyricsHtml += "<p style='color: #1DB954; font-size: 18pt; text-align: center; margin: 10px 0;'>" + m_lyricLines[i].second + "</p>";
        } else {
            lyricsHtml += "<p style='color: #FFFFFF; font-size: 14pt; text-align: center; margin: 5px 0;'>" + m_lyricLines[i].second + "</p>";
        }
    }

    ui->Lyrics->setHtml(lyricsHtml);
}

/**
 * @brief 清空歌词内容
 */
void FloatingLyrics::clearLyrics()
{
    m_lyricLines.clear();
    m_currentLyricIndex = -1;
    ui->Lyrics->clear();
}

/**
 * @brief 事件过滤器处理函数
 * @param watched 被监视的对象
 * @param event 事件对象
 * @return 是否处理该事件
 */
bool FloatingLyrics::eventFilter(QObject *watched, QEvent *event)
{
    // 如果启用了鼠标穿透，忽略所有鼠标事件
    if (m_clickThrough) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
            return true;
        default:
            break;
        }
    }

    return QDialog::eventFilter(watched, event);
}

/**
 * @brief 鼠标按下事件处理
 * @param event 鼠标事件
 */
void FloatingLyrics::mousePressEvent(QMouseEvent *event)
{
    if (m_clickThrough) {
        // 如果启用了鼠标穿透，忽略事件
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

/**
 * @brief 鼠标移动事件处理
 * @param event 鼠标事件
 */
void FloatingLyrics::mouseMoveEvent(QMouseEvent *event)
{
    if (m_clickThrough) {
        // 如果启用了鼠标穿透，忽略事件
        event->ignore();
        return;
    }

    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

/**
 * @brief 鼠标释放事件处理
 * @param event 鼠标事件
 */
void FloatingLyrics::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clickThrough) {
        // 如果启用了鼠标穿透，忽略事件
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}
