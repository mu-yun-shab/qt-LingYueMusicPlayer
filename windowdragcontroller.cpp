#include "windowdragcontroller.h"
#include <QMouseEvent>
#include <QWidget>

WindowDragController::WindowDragController(QWidget *dragTarget, QObject *parent)
    : QObject(parent), m_dragTarget(dragTarget)
{
    if (m_dragTarget) {
        m_dragTarget->installEventFilter(this);
    }
}

void WindowDragController::addExcludedWidget(QWidget *widget)
{
    if (widget) {
        m_excludedWidgets.insert(widget);
    }
}

bool WindowDragController::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_dragTarget || watched != m_dragTarget) {
        return QObject::eventFilter(watched, event);
    }

    QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);
    if (!mouseEvent) return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        // 检查点击位置是否在排除控件内
        for (QWidget *widget : m_excludedWidgets) {
            if (widget && widget->isVisible() &&
                widget->rect().contains(widget->mapFromGlobal(mouseEvent->globalPos()))) {
                return false;
            }
        }

        if (mouseEvent->button() == Qt::LeftButton) {
            m_dragStartPosition = mouseEvent->globalPos();
            m_windowStartPosition = m_dragTarget->pos();
            return true;
        }
        break;

    case QEvent::MouseMove:
        if (mouseEvent->buttons() & Qt::LeftButton && !m_dragStartPosition.isNull()) {
            QPoint delta = mouseEvent->globalPos() - m_dragStartPosition;
            m_dragTarget->move(m_windowStartPosition + delta);
            return true;
        }
        break;

    case QEvent::MouseButtonRelease:
        m_dragStartPosition = QPoint();
        break;

    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}
