#ifndef FLOATINGLYRICS_H
#define FLOATINGLYRICS_H

#include <QDialog>
#include <QMouseEvent>
#include <QList>
#include <QPair>

namespace Ui {
class FloatingLyrics;
}

/**
 * @brief 浮动歌词窗口类
 * 实现无边框、可拖拽、可调整透明度的歌词显示窗口
 */
class FloatingLyrics : public QDialog
{
    Q_OBJECT

public:
    explicit FloatingLyrics(QWidget *parent = nullptr);
    ~FloatingLyrics();

    // 设置透明度
    void setOpacity(int opacity);

    // 设置鼠标穿透
    void setClickThrough(bool enable);

    // 歌词相关功能
    void parseLyrics(const QString& lyrics);
    void updateLyricsDisplay(qint64 position);
    void clearLyrics();

protected:
    // 鼠标事件处理函数
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // 事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::FloatingLyrics *ui;
    bool m_dragging;           // 是否正在拖动
    QPoint m_dragStartPosition; // 拖动起始位置
    int m_opacity;             // 透明度值 (0-100)
    bool m_clickThrough;       // 是否启用鼠标穿透

    // 歌词相关成员变量
    QList<QPair<qint64, QString>> m_lyricLines; ///< 存储歌词行（时间戳和文本）
    int m_currentLyricIndex;                    ///< 当前歌词行索引
};

#endif // FLOATINGLYRICS_H
