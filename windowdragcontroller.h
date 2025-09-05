#ifndef WINDOWDRAGCONTROLLER_H
#define WINDOWDRAGCONTROLLER_H

#include <QObject>
#include <QPoint>
#include <QSet>

class QWidget;
class QMouseEvent;

/**
 * @brief 窗口拖动控制器
 */
class WindowDragController : public QObject
{
    Q_OBJECT
public:
    explicit WindowDragController(QWidget *dragTarget, QObject *parent = nullptr);

    /// 添加需要排除拖动的控件
    void addExcludedWidget(QWidget *widget);

protected:
    /// 事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *m_dragTarget;         ///< 要拖动的窗口
    QPoint m_dragStartPosition;    ///< 拖动开始时的鼠标位置
    QPoint m_windowStartPosition;  ///< 拖动开始时的窗口位置
    QSet<QWidget*> m_excludedWidgets; ///< 排除拖动的控件集合
};

#endif
