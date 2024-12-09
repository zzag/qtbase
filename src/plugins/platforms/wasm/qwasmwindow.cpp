// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
#include <QtCore/qfile.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <private/qpixmapcache_p.h>
#include <QtGui/qopenglfunctions.h>
#include <QBuffer>

#include "qwasmbase64iconstore.h"
#include "qwasmdom.h"
#include "qwasmclipboard.h"
#include "qwasmintegration.h"
#include "qwasmkeytranslator.h"
#include "qwasmwindow.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"
#include "qwasmevent.h"
#include "qwasmeventdispatcher.h"
#include "qwasmaccessibility.h"
#include "qwasmclipboard.h"
#include "qwasmdrag.h"

#include <iostream>
#include <sstream>

#include <emscripten/val.h>

#include <QtCore/private/qstdweb_p.h>
#include <QKeySequence>

QT_BEGIN_NAMESPACE

namespace {
QWasmWindowStack::PositionPreference positionPreferenceFromWindowFlags(Qt::WindowFlags flags)
{
    if (flags.testFlag(Qt::WindowStaysOnTopHint))
        return QWasmWindowStack::PositionPreference::StayOnTop;
    if (flags.testFlag(Qt::WindowStaysOnBottomHint))
        return QWasmWindowStack::PositionPreference::StayOnBottom;
    return QWasmWindowStack::PositionPreference::Regular;
}
} // namespace

Q_GUI_EXPORT int qt_defaultDpiX();

QWasmWindow::QWasmWindow(QWindow *w, QWasmDeadKeySupport *deadKeySupport,
                         QWasmCompositor *compositor, QWasmBackingStore *backingStore)
    : QPlatformWindow(w),
      m_compositor(compositor),
      m_backingStore(backingStore),
      m_deadKeySupport(deadKeySupport),
      m_document(dom::document()),
      m_decoratedWindow(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_window(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_a11yContainer(m_document.call<emscripten::val>("createElement", emscripten::val("div"))),
      m_canvas(m_document.call<emscripten::val>("createElement", emscripten::val("canvas")))
{
    m_decoratedWindow.set("className", "qt-decorated-window");
    m_decoratedWindow["style"].set("display", std::string("none"));

    m_nonClientArea = std::make_unique<NonClientArea>(this, m_decoratedWindow);
    m_nonClientArea->titleBar()->setTitle(window()->title());

    m_window.set("className", "qt-window");
    m_decoratedWindow.call<void>("appendChild", m_window);

    m_canvas["classList"].call<void>("add", emscripten::val("qt-window-canvas"));

    // Set contentEditable so that the window gets clipboard events,
    // then hide the resulting focus frame.
    m_window.set("contentEditable", std::string("true"));
    m_window["style"].set("outline", std::string("none"));

    QWasmClipboard::installEventHandlers(m_window);

    // Set inputMode to none to stop the mobile keyboard from opening
    // when the user clicks on the window.
    m_window.set("inputMode", std::string("none"));

    // Hide the canvas from screen readers.
    m_canvas.call<void>("setAttribute", std::string("aria-hidden"), std::string("true"));
    m_window.call<void>("appendChild", m_canvas);

    m_a11yContainer["classList"].call<void>("add", emscripten::val("qt-window-a11y-container"));
    m_window.call<void>("appendChild", m_a11yContainer);

    const bool rendersTo2dContext = w->surfaceType() != QSurface::OpenGLSurface;
    if (rendersTo2dContext)
        m_context2d = m_canvas.call<emscripten::val>("getContext", emscripten::val("2d"));
    static int serialNo = 0;
    m_winId = ++serialNo;
    m_decoratedWindow.set("id", "qt-window-" + std::to_string(m_winId));
    emscripten::val::module_property("specialHTMLTargets").set(canvasSelector(), m_canvas);

    m_flags = window()->flags();

    registerEventHandlers();

    setParent(parent());
}

void QWasmWindow::registerEventHandlers()
{
    m_pointerDownCallback = std::make_unique<qstdweb::EventCallback>(m_window, "pointerdown",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerDown, event)); }
    );
    m_pointerMoveCallback = std::make_unique<qstdweb::EventCallback>(m_window, "pointermove",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerMove, event)); }
    );
    m_pointerUpCallback = std::make_unique<qstdweb::EventCallback>(m_window, "pointerup",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerUp, event)); }
    );
    m_pointerCancelCallback = std::make_unique<qstdweb::EventCallback>(m_window, "pointercancel",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerCancel, event)); }
    );
    m_pointerEnterCallback = std::make_unique<qstdweb::EventCallback>(m_window, "pointerenter",
        [this](emscripten::val event) { this->handlePointerEnterLeaveEvent(PointerEvent(EventType::PointerEnter, event)); }
    );
    m_pointerLeaveCallback = std::make_unique<qstdweb::EventCallback>(m_window, "pointerleave",
        [this](emscripten::val event) { this->handlePointerEnterLeaveEvent(PointerEvent(EventType::PointerLeave, event)); }
    );

    m_window.call<void>("setAttribute", emscripten::val("draggable"), emscripten::val("true"));
    m_dragStartCallback = std::make_unique<qstdweb::EventCallback>(m_window, "dragstart",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragStart, event, window());
            QWasmDrag::instance()->onNativeDragStarted(&dragEvent);
        }
    );
    m_dragOverCallback = std::make_unique<qstdweb::EventCallback>(m_window, "dragover",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragOver, event, window());
            QWasmDrag::instance()->onNativeDragOver(&dragEvent);
        }
    );
    m_dropCallback = std::make_unique<qstdweb::EventCallback>(m_window, "drop",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::Drop, event, window());
            QWasmDrag::instance()->onNativeDrop(&dragEvent);
        }
    );
    m_dragEndCallback = std::make_unique<qstdweb::EventCallback>(m_window, "dragend",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragEnd, event, window());
            QWasmDrag::instance()->onNativeDragFinished(&dragEvent);
        }
    );
    m_dragLeaveCallback = std::make_unique<qstdweb::EventCallback>(m_window, "dragleave",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragLeave, event, window());
            QWasmDrag::instance()->onNativeDragLeave(&dragEvent);
        }
    );

    m_wheelEventCallback = std::make_unique<qstdweb::EventCallback>( m_window, "wheel",
        [this](emscripten::val event) { this->handleWheelEvent(event); });

    QWasmInputContext *wasmInput = QWasmIntegration::get()->wasmInputContext();
    if (wasmInput) {
        m_keyDownCallbackForInputContext =
            std::make_unique<qstdweb::EventCallback>(wasmInput->m_inputElement, "keydown",
            [this](emscripten::val event) { this->handleKeyForInputContextEvent(EventType::KeyDown, event); });
        m_keyUpCallbackForInputContext =
            std::make_unique<qstdweb::EventCallback>(wasmInput->m_inputElement, "keyup",
            [this](emscripten::val event) { this->handleKeyForInputContextEvent(EventType::KeyUp, event); });
    }

    m_keyDownCallback = std::make_unique<qstdweb::EventCallback>(m_window, "keydown",
        [this](emscripten::val event) { this->handleKeyEvent(KeyEvent(EventType::KeyDown, event, m_deadKeySupport)); });
    m_keyUpCallback =std::make_unique<qstdweb::EventCallback>(m_window, "keyup",
        [this](emscripten::val event) {this->handleKeyEvent(KeyEvent(EventType::KeyUp, event, m_deadKeySupport)); });
}

QWasmWindow::~QWasmWindow()
{
    shutdown();

    emscripten::val::module_property("specialHTMLTargets").delete_(canvasSelector());
    m_window.call<void>("removeChild", m_canvas);
    m_context2d = emscripten::val::undefined();
    commitParent(nullptr);
    if (m_requestAnimationFrameId > -1)
        emscripten_cancel_animation_frame(m_requestAnimationFrameId);
#if QT_CONFIG(accessibility)
    QWasmAccessibility::removeAccessibilityEnableButton(window());
#endif
}

QSurfaceFormat QWasmWindow::format() const
{
    return window()->requestedFormat();
}

QWasmWindow *QWasmWindow::fromWindow(QWindow *window)
{
    return static_cast<QWasmWindow *>(window->handle());
}

void QWasmWindow::onRestoreClicked()
{
    window()->setWindowState(Qt::WindowNoState);
}

void QWasmWindow::onMaximizeClicked()
{
    window()->setWindowState(Qt::WindowMaximized);
}

void QWasmWindow::onToggleMaximized()
{
    window()->setWindowState(m_state.testFlag(Qt::WindowMaximized) ? Qt::WindowNoState
                                                                   : Qt::WindowMaximized);
}

void QWasmWindow::onCloseClicked()
{
    window()->close();
}

void QWasmWindow::onNonClientAreaInteraction()
{
    requestActivateWindow();
    QGuiApplicationPrivate::instance()->closeAllPopups();
}

bool QWasmWindow::onNonClientEvent(const PointerEvent &event)
{
    QPointF pointInScreen = platformScreen()->mapFromLocal(
        dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));
    return QWindowSystemInterface::handleMouseEvent(
            window(), QWasmIntegration::getTimestamp(), window()->mapFromGlobal(pointInScreen),
            pointInScreen, event.mouseButtons, event.mouseButton,
            MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::NonClient),
            event.modifiers);
}

void QWasmWindow::initialize()
{
    auto initialGeometry = QPlatformWindow::initialGeometry(window(),
            windowGeometry(), defaultWindowSize, defaultWindowSize);
    m_normalGeometry = initialGeometry;

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());
    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

    if (window()->isTopLevel())
        setWindowIcon(window()->icon());
    QPlatformWindow::setGeometry(m_normalGeometry);

#if QT_CONFIG(accessibility)
    // Add accessibility-enable button. The user can activate this
    // button to opt-in to accessibility.
     if (window()->isTopLevel())
        QWasmAccessibility::addAccessibilityEnableButton(window());
#endif
}

QWasmScreen *QWasmWindow::platformScreen() const
{
    return static_cast<QWasmScreen *>(window()->screen()->handle());
}

void QWasmWindow::paint()
{
    if (!m_backingStore || !isVisible() || m_context2d.isUndefined())
        return;

    auto image = m_backingStore->getUpdatedWebImage(this);
    if (image.isUndefined())
        return;
    m_context2d.call<void>("putImageData", image, emscripten::val(0), emscripten::val(0));
}

void QWasmWindow::setZOrder(int z)
{
    m_decoratedWindow["style"].set("zIndex", std::to_string(z));
}

void QWasmWindow::setWindowCursor(QByteArray cssCursorName)
{
    m_window["style"].set("cursor", emscripten::val(cssCursorName.constData()));
}

void QWasmWindow::setGeometry(const QRect &rect)
{
    const auto margins = frameMargins();

    const QRect clientAreaRect = ([this, &rect, &margins]() {
        if (m_state.testFlag(Qt::WindowFullScreen))
            return platformScreen()->geometry();
        if (m_state.testFlag(Qt::WindowMaximized))
            return platformScreen()->availableGeometry().marginsRemoved(frameMargins());

        auto offset = rect.topLeft() - (!parent() ? screen()->geometry().topLeft() : QPoint());

        // In viewport
        auto containerGeometryInViewport =
                QRectF::fromDOMRect(parentNode()->containerElement().call<emscripten::val>(
                                            "getBoundingClientRect"))
                        .toRect();

        auto rectInViewport = QRect(containerGeometryInViewport.topLeft() + offset, rect.size());

        QRect cappedGeometry(rectInViewport);
        if (!parent()) {
            // Clamp top level windows top position to the screen bounds
            cappedGeometry.moveTop(
                std::max(std::min(rectInViewport.y(), containerGeometryInViewport.bottom()),
                         containerGeometryInViewport.y() + margins.top()));
        }
        cappedGeometry.setSize(
                cappedGeometry.size().expandedTo(windowMinimumSize()).boundedTo(windowMaximumSize()));
        return QRect(QPoint(rect.x(), rect.y() + cappedGeometry.y() - rectInViewport.y()),
                     rect.size());
    })();
    m_nonClientArea->onClientAreaWidthChange(clientAreaRect.width());

    const auto frameRect =
            clientAreaRect
                    .adjusted(-margins.left(), -margins.top(), margins.right(), margins.bottom())
                    .translated(!parent() ? -screen()->geometry().topLeft() : QPoint());

    m_decoratedWindow["style"].set("left", std::to_string(frameRect.left()) + "px");
    m_decoratedWindow["style"].set("top", std::to_string(frameRect.top()) + "px");
    m_canvas["style"].set("width", std::to_string(clientAreaRect.width()) + "px");
    m_canvas["style"].set("height", std::to_string(clientAreaRect.height()) + "px");
    m_a11yContainer["style"].set("width", std::to_string(clientAreaRect.width()) + "px");
    m_a11yContainer["style"].set("height", std::to_string(clientAreaRect.height()) + "px");

    // Important for the title flexbox to shrink correctly
    m_window["style"].set("width", std::to_string(clientAreaRect.width()) + "px");

    QSizeF canvasSize = clientAreaRect.size() * devicePixelRatio();

    m_canvas.set("width", canvasSize.width());
    m_canvas.set("height", canvasSize.height());

    bool shouldInvalidate = true;
    if (!m_state.testFlag(Qt::WindowFullScreen) && !m_state.testFlag(Qt::WindowMaximized)) {
        shouldInvalidate = m_normalGeometry.size() != clientAreaRect.size();
        m_normalGeometry = clientAreaRect;
    }
    QWindowSystemInterface::handleGeometryChange(window(), clientAreaRect);
    if (shouldInvalidate)
        invalidate();
    else
        m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()));
}

void QWasmWindow::setVisible(bool visible)
{
    // TODO(mikolajboc): isVisible()?
    const bool nowVisible = m_decoratedWindow["style"]["display"].as<std::string>() == "block";
    if (visible == nowVisible)
        return;

    m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()), QWasmCompositor::ExposeEventDelivery);
    m_decoratedWindow["style"].set("display", visible ? "block" : "none");
    if (window()->isActive())
        m_canvas.call<void>("focus");
    if (visible)
        applyWindowState();
}

bool QWasmWindow::isVisible() const
{
    return window()->isVisible();
}

QMargins QWasmWindow::frameMargins() const
{
    const auto frameRect =
            QRectF::fromDOMRect(m_decoratedWindow.call<emscripten::val>("getBoundingClientRect"));
    const auto canvasRect =
            QRectF::fromDOMRect(m_window.call<emscripten::val>("getBoundingClientRect"));
    return QMarginsF(canvasRect.left() - frameRect.left(), canvasRect.top() - frameRect.top(),
                     frameRect.right() - canvasRect.right(),
                     frameRect.bottom() - canvasRect.bottom())
            .toMargins();
}

void QWasmWindow::raise()
{
    bringToTop();
    invalidate();
}

void QWasmWindow::lower()
{
    sendToBottom();
    invalidate();
}

WId QWasmWindow::winId() const
{
    return m_winId;
}

void QWasmWindow::propagateSizeHints()
{
    // setGeometry() will take care of minimum and maximum size constraints
    setGeometry(windowGeometry());
    m_nonClientArea->propagateSizeHints();
}

void QWasmWindow::setOpacity(qreal level)
{
    m_decoratedWindow["style"].set("opacity", qBound(0.0, level, 1.0));
}

void QWasmWindow::invalidate()
{
    m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()));
}

void QWasmWindow::onActivationChanged(bool active)
{
    dom::syncCSSClassWith(m_decoratedWindow, "inactive", !active);
}

// Fix top level window flags in case only the type flags are passed.
static inline Qt::WindowFlags fixTopLevelWindowFlags(Qt::WindowFlags flags)
{
    if (!(flags.testFlag(Qt::CustomizeWindowHint))) {
        if (flags.testFlag(Qt::Window)) {
            flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint
                  |Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint;
        }
        if (flags.testFlag(Qt::Dialog) || flags.testFlag(Qt::Tool))
            flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;

        if ((flags & Qt::WindowType_Mask) == Qt::SplashScreen)
            flags |= Qt::FramelessWindowHint;
    }
    return flags;
}

void QWasmWindow::setWindowFlags(Qt::WindowFlags flags)
{
    flags = fixTopLevelWindowFlags(flags);

    if (flags.testFlag(Qt::WindowStaysOnTopHint) != m_flags.testFlag(Qt::WindowStaysOnTopHint)
        || flags.testFlag(Qt::WindowStaysOnBottomHint)
                != m_flags.testFlag(Qt::WindowStaysOnBottomHint)) {
        onPositionPreferenceChanged(positionPreferenceFromWindowFlags(flags));
    }
    m_flags = flags;
    dom::syncCSSClassWith(m_decoratedWindow, "frameless", !hasFrame() || !window()->isTopLevel());
    dom::syncCSSClassWith(m_decoratedWindow, "has-border", hasBorder());
    dom::syncCSSClassWith(m_decoratedWindow, "has-shadow", hasShadow());
    dom::syncCSSClassWith(m_decoratedWindow, "has-title", hasTitleBar());
    dom::syncCSSClassWith(m_decoratedWindow, "transparent-for-input",
                          flags.testFlag(Qt::WindowTransparentForInput));

    m_nonClientArea->titleBar()->setMaximizeVisible(hasMaximizeButton());
    m_nonClientArea->titleBar()->setCloseVisible(m_flags.testFlag(Qt::WindowCloseButtonHint));
}

void QWasmWindow::setWindowState(Qt::WindowStates newState)
{
    // Child windows can not have window states other than Qt::WindowActive
    if (parent())
        newState &= Qt::WindowActive;

    const Qt::WindowStates oldState = m_state;

    if (newState.testFlag(Qt::WindowMinimized)) {
        newState.setFlag(Qt::WindowMinimized, false);
        qWarning("Qt::WindowMinimized is not implemented in wasm");
        window()->setWindowStates(newState);
        return;
    }

    if (newState == oldState)
        return;

    m_state = newState;
    m_previousWindowState = oldState;

    applyWindowState();
}

void QWasmWindow::setWindowTitle(const QString &title)
{
    m_nonClientArea->titleBar()->setTitle(title);
}

void QWasmWindow::setWindowIcon(const QIcon &icon)
{
    const auto dpi = screen()->devicePixelRatio();
    auto pixmap = icon.pixmap(10 * dpi, 10 * dpi);
    if (pixmap.isNull()) {
        m_nonClientArea->titleBar()->setIcon(
                Base64IconStore::get()->getIcon(Base64IconStore::IconType::QtLogo), "svg+xml");
        return;
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    pixmap.save(&buffer, "png");
    m_nonClientArea->titleBar()->setIcon(bytes.toBase64().toStdString(), "png");
}

void QWasmWindow::applyWindowState()
{
    QRect newGeom;

    const bool isFullscreen = m_state.testFlag(Qt::WindowFullScreen);
    const bool isMaximized = m_state.testFlag(Qt::WindowMaximized);
    if (isFullscreen)
        newGeom = platformScreen()->geometry();
    else if (isMaximized)
        newGeom = platformScreen()->availableGeometry().marginsRemoved(frameMargins());
    else
        newGeom = normalGeometry();

    dom::syncCSSClassWith(m_decoratedWindow, "has-border", hasBorder());
    dom::syncCSSClassWith(m_decoratedWindow, "maximized", isMaximized);

    m_nonClientArea->titleBar()->setRestoreVisible(isMaximized);
    m_nonClientArea->titleBar()->setMaximizeVisible(hasMaximizeButton());

    if (isVisible())
        QWindowSystemInterface::handleWindowStateChanged(window(), m_state, m_previousWindowState);
    setGeometry(newGeom);
}

void QWasmWindow::commitParent(QWasmWindowTreeNode *parent)
{
    onParentChanged(m_commitedParent, parent, positionPreferenceFromWindowFlags(window()->flags()));
    m_commitedParent = parent;
}

void QWasmWindow::handleKeyEvent(const KeyEvent &event)
{
    qCDebug(qLcQpaWasmInputContext) << "processKey as KeyEvent";
    if (processKey(event))
        event.webEvent.call<void>("preventDefault");
    event.webEvent.call<void>("stopPropagation");
}

bool QWasmWindow::processKey(const KeyEvent &event)
{
    constexpr bool ProceedToNativeEvent = false;
    Q_ASSERT(event.type == EventType::KeyDown || event.type == EventType::KeyUp);

    const auto clipboardResult =
            QWasmIntegration::get()->getWasmClipboard()->processKeyboard(event);

    using ProcessKeyboardResult = QWasmClipboard::ProcessKeyboardResult;
    if (clipboardResult == ProcessKeyboardResult::NativeClipboardEventNeeded)
        return ProceedToNativeEvent;

    const auto result = QWindowSystemInterface::handleKeyEvent(
            0, event.type == EventType::KeyDown ? QEvent::KeyPress : QEvent::KeyRelease, event.key,
            event.modifiers, event.text, event.autoRepeat);
    return clipboardResult == ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            ? ProceedToNativeEvent
            : result;
}

void QWasmWindow::handleKeyForInputContextEvent(EventType eventType, const emscripten::val &event)
{
    //
    // Things to consider:
    //
    // (Alt + '̃~') + a      -> compose('~', 'a')
    // (Compose) + '\'' + e -> compose('\'', 'e')
    // complex (i.e Chinese et al) input handling
    // Multiline text edit backspace at start of line
    //
    const QWasmInputContext *wasmInput = QWasmIntegration::get()->wasmInputContext();
    if (wasmInput) {
        const auto keyString = QString::fromStdString(event["key"].as<std::string>());
        qCDebug(qLcQpaWasmInputContext) << "Key callback" << keyString << keyString.size();
        if (keyString == "Unidentified") {
            // Android makes a bunch of KeyEvents as "Unidentified"
            // They will be processed just in InputContext.
            return;
        } else if (event["isComposing"].as<bool>()) {
            // Handled by the input context
            return;
        } else if (event["ctrlKey"].as<bool>()
                || event["altKey"].as<bool>()
                || event["metaKey"].as<bool>()) {
            // Not all platforms use 'isComposing' for '~' + 'a', in this
            // case send the key with state ('ctrl', 'alt', or 'meta') to
            // processKeyForInputContext

            ; // fallthrough
        } else if (keyString.size() != 1) {
            // This is like; 'Shift','ArrowRight','AltGraph', ...
            // send all of these to processKeyForInputContext

            ; // fallthrough
        } else if (wasmInput->inputMethodAccepted()) {
            // processed in inputContext with skipping processKey
            return;
        }
    }

    qCDebug(qLcQpaWasmInputContext) << "processKey as KeyEvent";
    if (processKeyForInputContext(KeyEvent(eventType, event, m_deadKeySupport)))
        event.call<void>("preventDefault");
    event.call<void>("stopImmediatePropagation");
}

bool QWasmWindow::processKeyForInputContext(const KeyEvent &event)
{
    qCDebug(qLcQpaWasmInputContext) << Q_FUNC_INFO;
    Q_ASSERT(event.type == EventType::KeyDown || event.type == EventType::KeyUp);

    QKeySequence keySeq(event.modifiers | event.key);

    if (keySeq == QKeySequence::Paste) {
        // Process it in pasteCallback and inputCallback
        return false;
    }

    const auto result = QWindowSystemInterface::handleKeyEvent(
            0, event.type == EventType::KeyDown ? QEvent::KeyPress : QEvent::KeyRelease, event.key,
            event.modifiers, event.text);

    // Copy/Cut callback required to copy qtClipboard to system clipboard
    if (keySeq == QKeySequence::Copy || keySeq == QKeySequence::Cut)
        return false;

    return result;
}

void QWasmWindow::handlePointerEnterLeaveEvent(const PointerEvent &event)
{
    if (processPointerEnterLeave(event))
        event.webEvent.call<void>("preventDefault");
}

bool QWasmWindow::processPointerEnterLeave(const PointerEvent &event)
{
    if (event.pointerType != PointerType::Mouse && event.pointerType != PointerType::Pen)
        return false;

    switch (event.type) {
    case EventType::PointerEnter: {
        const auto pointInScreen = platformScreen()->mapFromLocal(
            dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));
        QWindowSystemInterface::handleEnterEvent(
                window(), mapFromGlobal(pointInScreen.toPoint()), pointInScreen);
        break;
    }
    case EventType::PointerLeave:
        QWindowSystemInterface::handleLeaveEvent(window());
        break;
    default:
        break;
    }

    return false;
}

void QWasmWindow::processPointer(const PointerEvent &event)
{
    switch (event.type) {
    case EventType::PointerDown:
        m_window.call<void>("setPointerCapture", event.pointerId);
        if ((window()->flags() & Qt::WindowDoesNotAcceptFocus)
                    != Qt::WindowDoesNotAcceptFocus
            && window()->isTopLevel())
                window()->requestActivate();
        break;
    case EventType::PointerUp:
        m_window.call<void>("releasePointerCapture", event.pointerId);
        break;
    default:
        break;
    };

    const bool eventAccepted = deliverPointerEvent(event);
    if (!eventAccepted && event.type == EventType::PointerDown)
        QGuiApplicationPrivate::instance()->closeAllPopups();
    event.webEvent.call<void>("preventDefault");
    event.webEvent.call<void>("stopPropagation");
}

bool QWasmWindow::deliverPointerEvent(const PointerEvent &event)
{
    const auto pointInScreen = platformScreen()->mapFromLocal(
        dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));

    const auto geometryF = platformScreen()->geometry().toRectF();
    const QPointF targetPointClippedToScreen(
            qBound(geometryF.left(), pointInScreen.x(), geometryF.right()),
            qBound(geometryF.top(), pointInScreen.y(), geometryF.bottom()));

    if (event.pointerType == PointerType::Mouse) {
        const QEvent::Type eventType =
                MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::Client);

        return eventType != QEvent::None
                && QWindowSystemInterface::handleMouseEvent(
                        window(), QWasmIntegration::getTimestamp(),
                        window()->mapFromGlobal(targetPointClippedToScreen),
                        targetPointClippedToScreen, event.mouseButtons, event.mouseButton,
                        eventType, event.modifiers);
    }

    if (event.pointerType == PointerType::Pen) {
        qreal pressure;
        switch (event.type) {
            case EventType::PointerDown :
            case EventType::PointerMove :
                pressure = event.pressure;
                break;
            case EventType::PointerUp :
                pressure = 0.0;
                break;
            default:
                return false;
        }
        // Tilt in the browser is in the range +-90, but QTabletEvent only goes to +-60.
        qreal xTilt = qBound(-60.0, event.tiltX, 60.0);
        qreal yTilt = qBound(-60.0, event.tiltY, 60.0);
        // Barrel rotation is reported as 0 to 359, but QTabletEvent wants a signed value.
        qreal rotation = event.twist > 180.0 ? 360.0 - event.twist : event.twist;
        return QWindowSystemInterface::handleTabletEvent(
            window(), QWasmIntegration::getTimestamp(), platformScreen()->tabletDevice(),
            window()->mapFromGlobal(targetPointClippedToScreen),
            targetPointClippedToScreen, event.mouseButtons, pressure, xTilt, yTilt,
            event.tangentialPressure, rotation, event.modifiers);
    }

    QWindowSystemInterface::TouchPoint *touchPoint;

    QPointF pointInTargetWindowCoords =
            QPointF(window()->mapFromGlobal(targetPointClippedToScreen));
    QPointF normalPosition(pointInTargetWindowCoords.x() / window()->width(),
                           pointInTargetWindowCoords.y() / window()->height());

    const auto tp = m_pointerIdToTouchPoints.find(event.pointerId);
    if (event.pointerType != PointerType::Pen && tp != m_pointerIdToTouchPoints.end()) {
        touchPoint = &tp.value();
    } else {
        touchPoint = &m_pointerIdToTouchPoints
                              .insert(event.pointerId, QWindowSystemInterface::TouchPoint())
                              .value();

        // Assign touch point id. TouchPoint::id is int, but QGuiApplicationPrivate::processTouchEvent()
        // will not synthesize mouse events for touch points with negative id; use the absolute value for
        // the touch point id.
        touchPoint->id = qAbs(event.pointerId);

        touchPoint->state = QEventPoint::State::Pressed;
    }

    const bool stationaryTouchPoint = (normalPosition == touchPoint->normalPosition);
    touchPoint->normalPosition = normalPosition;
    touchPoint->area = QRectF(targetPointClippedToScreen, QSizeF(event.width, event.height))
                                   .translated(-event.width / 2, -event.height / 2);
    touchPoint->pressure = event.pressure;

    switch (event.type) {
    case EventType::PointerUp:
        touchPoint->state = QEventPoint::State::Released;
        break;
    case EventType::PointerMove:
        touchPoint->state = (stationaryTouchPoint ? QEventPoint::State::Stationary
                                                  : QEventPoint::State::Updated);
        break;
    default:
        break;
    }

    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(m_pointerIdToTouchPoints.size());
    std::transform(m_pointerIdToTouchPoints.begin(), m_pointerIdToTouchPoints.end(),
                   std::back_inserter(touchPointList),
                   [](const QWindowSystemInterface::TouchPoint &val) { return val; });

    if (event.type == EventType::PointerUp)
        m_pointerIdToTouchPoints.remove(event.pointerId);

    return event.type == EventType::PointerCancel
            ? QWindowSystemInterface::handleTouchCancelEvent(
                    window(), QWasmIntegration::getTimestamp(), platformScreen()->touchDevice(),
                    event.modifiers)
            : QWindowSystemInterface::handleTouchEvent(
                    window(), QWasmIntegration::getTimestamp(), platformScreen()->touchDevice(),
                    touchPointList, event.modifiers);
}

void QWasmWindow::handleWheelEvent(const emscripten::val &event)
{
    if (processWheel(WheelEvent(EventType::Wheel, event)))
        event.call<void>("preventDefault");
}

bool QWasmWindow::processWheel(const WheelEvent &event)
{
    // Web scroll deltas are inverted from Qt deltas - negate.
    const int scrollFactor = -([&event]() {
        switch (event.deltaMode) {
        case DeltaMode::Pixel:
            return 1;
        case DeltaMode::Line:
            return 12;
        case DeltaMode::Page:
            return 20;
        };
    })();

    const auto pointInScreen = platformScreen()->mapFromLocal(
        dom::mapPoint(event.target(), platformScreen()->element(), event.localPoint));

    return QWindowSystemInterface::handleWheelEvent(
            window(), QWasmIntegration::getTimestamp(), window()->mapFromGlobal(pointInScreen),
            pointInScreen, (event.delta * scrollFactor).toPoint(),
            (event.delta * scrollFactor).toPoint(), event.modifiers, Qt::NoScrollPhase,
            Qt::MouseEventNotSynthesized, event.webkitDirectionInvertedFromDevice);
}

QRect QWasmWindow::normalGeometry() const
{
    return m_normalGeometry;
}

qreal QWasmWindow::devicePixelRatio() const
{
    return screen()->devicePixelRatio();
}

void QWasmWindow::requestUpdate()
{
    m_compositor->requestUpdateWindow(this, QRect(QPoint(0, 0), geometry().size()), QWasmCompositor::UpdateRequestDelivery);
}

bool QWasmWindow::hasFrame() const
{
    return !m_flags.testFlag(Qt::FramelessWindowHint);
}

bool QWasmWindow::hasBorder() const
{
    return hasFrame() && !m_state.testFlag(Qt::WindowFullScreen) && !m_flags.testFlag(Qt::SubWindow)
            && !windowIsPopupType(m_flags) && !parent();
}

bool QWasmWindow::hasTitleBar() const
{
    return hasBorder() && m_flags.testFlag(Qt::WindowTitleHint);
}

bool QWasmWindow::hasShadow() const
{
    return hasBorder() && !m_flags.testFlag(Qt::NoDropShadowWindowHint);
}

bool QWasmWindow::hasMaximizeButton() const
{
    return !m_state.testFlag(Qt::WindowMaximized) && m_flags.testFlag(Qt::WindowMaximizeButtonHint);
}

bool QWasmWindow::windowIsPopupType(Qt::WindowFlags flags) const
{
    if (flags.testFlag(Qt::Tool))
        return false; // Qt::Tool has the Popup bit set but isn't an actual Popup window

    return (flags.testFlag(Qt::Popup));
}

void QWasmWindow::requestActivateWindow()
{
    QWindow *modalWindow;
    if (QGuiApplicationPrivate::instance()->isWindowBlocked(window(), &modalWindow)) {
        static_cast<QWasmWindow *>(modalWindow->handle())->requestActivateWindow();
        return;
    }

    raise();
    setAsActiveNode();

    if (!QWasmIntegration::get()->inputContext())
        m_canvas.call<void>("focus");

    QPlatformWindow::requestActivateWindow();
}

bool QWasmWindow::setMouseGrabEnabled(bool grab)
{
    Q_UNUSED(grab);
    return false;
}

bool QWasmWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowBlocked:
        m_decoratedWindow["classList"].call<void>("add", emscripten::val("blocked"));
        return false; // Propagate further
    case QEvent::WindowUnblocked:;
        m_decoratedWindow["classList"].call<void>("remove", emscripten::val("blocked"));
        return false; // Propagate further
    default:
        return QPlatformWindow::windowEvent(event);
    }
}

void QWasmWindow::setMask(const QRegion &region)
{
    if (region.isEmpty()) {
        m_decoratedWindow["style"].set("clipPath", emscripten::val(""));
        return;
    }

    std::ostringstream cssClipPath;
    cssClipPath << "path('";
    for (const auto &rect : region) {
        const auto cssRect = rect.adjusted(0, 0, 1, 1);
        cssClipPath << "M " << cssRect.left() << " " << cssRect.top() << " ";
        cssClipPath << "L " << cssRect.right() << " " << cssRect.top() << " ";
        cssClipPath << "L " << cssRect.right() << " " << cssRect.bottom() << " ";
        cssClipPath << "L " << cssRect.left() << " " << cssRect.bottom() << " z ";
    }
    cssClipPath << "')";
    m_decoratedWindow["style"].set("clipPath", emscripten::val(cssClipPath.str()));
}

void QWasmWindow::setParent(const QPlatformWindow *)
{
    // The window flags depend on whether we are a
    // child window or not, so update them here.
    setWindowFlags(window()->flags());

    commitParent(parentNode());
}

std::string QWasmWindow::canvasSelector() const
{
    return "!qtwindow" + std::to_string(m_winId);
}

emscripten::val QWasmWindow::containerElement()
{
    return m_window;
}

QWasmWindowTreeNode *QWasmWindow::parentNode()
{
    if (parent())
        return static_cast<QWasmWindow *>(parent());
    return platformScreen();
}

QWasmWindow *QWasmWindow::asWasmWindow()
{
    return this;
}

void QWasmWindow::onParentChanged(QWasmWindowTreeNode *previous, QWasmWindowTreeNode *current,
                                  QWasmWindowStack::PositionPreference positionPreference)
{
    if (previous)
        previous->containerElement().call<void>("removeChild", m_decoratedWindow);
    if (current)
        current->containerElement().call<void>("appendChild", m_decoratedWindow);
    QWasmWindowTreeNode::onParentChanged(previous, current, positionPreference);
}

QT_END_NAMESPACE
