// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBOX_H
#define QBOX_H

#include <QtCore/qmargins.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
struct CGRect;
#endif
#if defined(Q_OS_WASM) || defined(Q_QDOC)
namespace emscripten {
class val;
}
#endif

QT_BEGIN_NAMESPACE

class QBoxF;

class Q_CORE_EXPORT QBox
{
public:
    constexpr QBox() noexcept : xp(0), yp(0), w(0), h(0) { }
    constexpr QBox(const QPoint &topleft, const QPoint &bottomright) noexcept;
    constexpr QBox(const QPoint &topleft, const QSize &size) noexcept;
    constexpr QBox(int left, int top, int width, int height) noexcept;

    constexpr inline bool isNull() const noexcept;
    constexpr inline bool isEmpty() const noexcept;
    constexpr inline bool isValid() const noexcept;

    constexpr inline int left() const noexcept;
    constexpr inline int top() const noexcept;
    constexpr inline int right() const noexcept;
    constexpr inline int bottom() const noexcept;
    [[nodiscard]] QBox normalized() const noexcept;

    constexpr inline int x() const noexcept;
    constexpr inline int y() const noexcept;
    constexpr inline void setLeft(int pos) noexcept;
    constexpr inline void setTop(int pos) noexcept;
    constexpr inline void setRight(int pos) noexcept;
    constexpr inline void setBottom(int pos) noexcept;
    constexpr inline void setX(int x) noexcept;
    constexpr inline void setY(int y) noexcept;

    constexpr inline void setTopLeft(const QPoint &p) noexcept;
    constexpr inline void setBottomRight(const QPoint &p) noexcept;
    constexpr inline void setTopRight(const QPoint &p) noexcept;
    constexpr inline void setBottomLeft(const QPoint &p) noexcept;

    constexpr inline QPoint topLeft() const noexcept;
    constexpr inline QPoint bottomRight() const noexcept;
    constexpr inline QPoint topRight() const noexcept;
    constexpr inline QPoint bottomLeft() const noexcept;
    constexpr inline QPoint center() const noexcept;

    constexpr inline void moveLeft(int pos) noexcept;
    constexpr inline void moveTop(int pos) noexcept;
    constexpr inline void moveRight(int pos) noexcept;
    constexpr inline void moveBottom(int pos) noexcept;
    constexpr inline void moveTopLeft(const QPoint &p) noexcept;
    constexpr inline void moveBottomRight(const QPoint &p) noexcept;
    constexpr inline void moveTopRight(const QPoint &p) noexcept;
    constexpr inline void moveBottomLeft(const QPoint &p) noexcept;
    constexpr inline void moveCenter(const QPoint &p) noexcept;

    constexpr inline void translate(int dx, int dy) noexcept;
    constexpr inline void translate(const QPoint &p) noexcept;
    [[nodiscard]] constexpr inline QBox translated(int dx, int dy) const noexcept;
    [[nodiscard]] constexpr inline QBox translated(const QPoint &p) const noexcept;
    [[nodiscard]] constexpr inline QBox transposed() const noexcept;

    constexpr inline void moveTo(int x, int t) noexcept;
    constexpr inline void moveTo(const QPoint &p) noexcept;

    constexpr inline void setRect(int x, int y, int w, int h) noexcept;
    constexpr inline void getRect(int *x, int *y, int *w, int *h) const;

    constexpr inline void setCoords(int x1, int y1, int x2, int y2) noexcept;
    constexpr inline void getCoords(int *x1, int *y1, int *x2, int *y2) const;

    constexpr inline void adjust(int x1, int y1, int x2, int y2) noexcept;
    [[nodiscard]] constexpr inline QBox adjusted(int x1, int y1, int x2, int y2) const noexcept;

    constexpr inline QSize size() const noexcept;
    constexpr inline int width() const noexcept;
    constexpr inline int height() const noexcept;
    constexpr inline void setWidth(int w) noexcept;
    constexpr inline void setHeight(int h) noexcept;
    constexpr inline void setSize(const QSize &s) noexcept;

    QBox operator|(const QBox &r) const noexcept;
    QBox operator&(const QBox &r) const noexcept;
    inline QBox &operator|=(const QBox &r) noexcept;
    inline QBox &operator&=(const QBox &r) noexcept;

    bool contains(const QBox &r) const noexcept;
    bool contains(const QPoint &p) const noexcept;
    inline bool contains(int x, int y) const noexcept;
    [[nodiscard]] inline QBox united(const QBox &other) const noexcept;
    [[nodiscard]] inline QBox intersected(const QBox &other) const noexcept;
    bool intersects(const QBox &r) const noexcept;

    constexpr inline QBox marginsAdded(const QMargins &margins) const noexcept;
    constexpr inline QBox marginsRemoved(const QMargins &margins) const noexcept;
    constexpr inline QBox &operator+=(const QMargins &margins) noexcept;
    constexpr inline QBox &operator-=(const QMargins &margins) noexcept;

    [[nodiscard]] static constexpr inline QBox span(const QPoint &p1, const QPoint &p2) noexcept;

    friend constexpr inline bool operator==(const QBox &r1, const QBox &r2) noexcept
    {
        return r1.xp == r2.xp && r1.w == r2.w && r1.yp == r2.yp && r1.h == r2.h;
    }
    friend constexpr inline bool operator!=(const QBox &r1, const QBox &r2) noexcept
    {
        return r1.xp != r2.xp || r1.w != r2.w || r1.yp != r2.yp || r1.h != r2.h;
    }

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    [[nodiscard]] CGRect toCGRect() const noexcept;
#endif
    [[nodiscard]] constexpr inline QBoxF toBoxF() const noexcept;

private:
    int xp;
    int yp;
    int w;
    int h;
};
Q_DECLARE_TYPEINFO(QBox, Q_RELOCATABLE_TYPE);

/*****************************************************************************
  QBox stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QBox &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QBox &);
#endif

/*****************************************************************************
  QBox inline member functions
 *****************************************************************************/

constexpr inline QBox::QBox(int aleft, int atop, int awidth, int aheight) noexcept
    : xp(aleft), yp(atop), w(awidth), h(aheight)
{
}

constexpr inline QBox::QBox(const QPoint &atopLeft, const QPoint &abottomRight) noexcept
    : xp(atopLeft.x()),
      yp(atopLeft.y()),
      w(abottomRight.x() - atopLeft.x()),
      h(abottomRight.y() - atopLeft.y())
{
}

constexpr inline QBox::QBox(const QPoint &atopLeft, const QSize &asize) noexcept
    : xp(atopLeft.x()), yp(atopLeft.y()), w(asize.width()), h(asize.height())
{
}

constexpr inline bool QBox::isNull() const noexcept
{
    return w == 0 && h == 0;
}

constexpr inline bool QBox::isEmpty() const noexcept
{
    return w <= 0 || h <= 0;
}

constexpr inline bool QBox::isValid() const noexcept
{
    return w > 0 && h > 0;
}

constexpr inline int QBox::left() const noexcept
{
    return xp;
}

constexpr inline int QBox::top() const noexcept
{
    return yp;
}

constexpr inline int QBox::right() const noexcept
{
    return xp + w;
}

constexpr inline int QBox::bottom() const noexcept
{
    return yp + w;
}

constexpr inline int QBox::x() const noexcept
{
    return xp;
}

constexpr inline int QBox::y() const noexcept
{
    return yp;
}

constexpr inline void QBox::setLeft(int pos) noexcept
{
    int diff = pos - xp;
    xp += diff;
    w -= diff;
}

constexpr inline void QBox::setRight(int pos) noexcept
{
    w = pos - xp;
}

constexpr inline void QBox::setTop(int pos) noexcept
{
    int diff = pos - yp;
    yp += diff;
    h -= diff;
}

constexpr inline void QBox::setBottom(int pos) noexcept
{
    h = pos - yp;
}

constexpr inline void QBox::setTopLeft(const QPoint &p) noexcept
{
    setTop(p.y());
    setLeft(p.x());
}

constexpr inline void QBox::setBottomRight(const QPoint &p) noexcept
{
    setBottom(p.y());
    setRight(p.x());
}

constexpr inline void QBox::setTopRight(const QPoint &p) noexcept
{
    setTop(p.y());
    setRight(p.x());
}

constexpr inline void QBox::setBottomLeft(const QPoint &p) noexcept
{
    setBottom(p.y());
    setLeft(p.x());
}

constexpr inline void QBox::setX(int ax) noexcept
{
    xp = ax;
}

constexpr inline void QBox::setY(int ay) noexcept
{
    yp = ay;
}

constexpr inline QPoint QBox::topLeft() const noexcept
{
    return QPoint(xp, yp);
}

constexpr inline QPoint QBox::bottomRight() const noexcept
{
    return QPoint(xp + w, yp + h);
}

constexpr inline QPoint QBox::topRight() const noexcept
{
    return QPoint(xp + w, yp);
}

constexpr inline QPoint QBox::bottomLeft() const noexcept
{
    return QPoint(xp, yp + h);
}

constexpr inline QPoint QBox::center() const noexcept
{
    return QPoint(xp + w / 2, yp + h / 2);
}

constexpr inline int QBox::width() const noexcept
{
    return w;
}

constexpr inline int QBox::height() const noexcept
{
    return h;
}

constexpr inline QSize QBox::size() const noexcept
{
    return QSize(width(), height());
}

constexpr inline void QBox::translate(int dx, int dy) noexcept
{
    xp += dx;
    yp += dy;
}

constexpr inline void QBox::translate(const QPoint &p) noexcept
{
    xp += p.x();
    yp += p.y();
}

constexpr inline QBox QBox::translated(int dx, int dy) const noexcept
{
    return QBox(xp + dx, yp + dy, w, h);
}

constexpr inline QBox QBox::translated(const QPoint &p) const noexcept
{
    return QBox(xp + p.x(), yp + p.y(), w, h);
}

constexpr inline QBox QBox::transposed() const noexcept
{
    return QBox(topLeft(), size().transposed());
}

constexpr inline void QBox::moveTo(int ax, int ay) noexcept
{
    xp = ax;
    yp = ay;
}

constexpr inline void QBox::moveTo(const QPoint &p) noexcept
{
    xp = p.x();
    yp = p.y();
}

constexpr inline void QBox::moveLeft(int pos) noexcept
{
    xp = pos;
}

constexpr inline void QBox::moveTop(int pos) noexcept
{
    yp = pos;
}

constexpr inline void QBox::moveRight(int pos) noexcept
{
    xp = pos - w;
}

constexpr inline void QBox::moveBottom(int pos) noexcept
{
    yp = pos - h;
}

constexpr inline void QBox::moveTopLeft(const QPoint &p) noexcept
{
    moveLeft(p.x());
    moveTop(p.y());
}

constexpr inline void QBox::moveBottomRight(const QPoint &p) noexcept
{
    moveRight(p.x());
    moveBottom(p.y());
}

constexpr inline void QBox::moveTopRight(const QPoint &p) noexcept
{
    moveRight(p.x());
    moveTop(p.y());
}

constexpr inline void QBox::moveBottomLeft(const QPoint &p) noexcept
{
    moveLeft(p.x());
    moveBottom(p.y());
}

constexpr inline void QBox::moveCenter(const QPoint &p) noexcept
{
    xp = p.x() - w / 2;
    yp = p.y() - h / 2;
}

constexpr inline void QBox::getRect(int *ax, int *ay, int *aw, int *ah) const
{
    *ax = xp;
    *ay = yp;
    *aw = w;
    *ah = h;
}

constexpr inline void QBox::setRect(int ax, int ay, int aw, int ah) noexcept
{
    xp = ax;
    yp = ay;
    w = aw;
    h = ah;
}

constexpr inline void QBox::getCoords(int *xp1, int *yp1, int *xp2, int *yp2) const
{
    *xp1 = xp;
    *yp1 = yp;
    *xp2 = xp + w;
    *yp2 = yp + h;
}

constexpr inline void QBox::setCoords(int xp1, int yp1, int xp2, int yp2) noexcept
{
    xp = xp1;
    yp = yp1;
    w = xp2 - xp1;
    h = yp2 - yp1;
}

constexpr inline QBox QBox::adjusted(int xp1, int yp1, int xp2, int yp2) const noexcept
{
    return QBox(xp + xp1, yp + yp1, w + xp2 - xp1, h + yp2 - yp1);
}

constexpr inline void QBox::adjust(int dx1, int dy1, int dx2, int dy2) noexcept
{
    xp += dx1;
    yp += dy1;
    w += dx2 - dx1;
    h += dy2 - dy1;
}

constexpr inline void QBox::setWidth(int aw) noexcept
{
    w = aw;
}

constexpr inline void QBox::setHeight(int ah) noexcept
{
    h = ah;
}

constexpr inline void QBox::setSize(const QSize &s) noexcept
{
    w = s.width();
    h = s.height();
}

inline bool QBox::contains(int ax, int ay) const noexcept
{
    return contains(QPoint(ax, ay));
}

inline QBox &QBox::operator|=(const QBox &r) noexcept
{
    *this = *this | r;
    return *this;
}

inline QBox &QBox::operator&=(const QBox &r) noexcept
{
    *this = *this & r;
    return *this;
}

inline QBox QBox::intersected(const QBox &other) const noexcept
{
    return *this & other;
}

inline QBox QBox::united(const QBox &r) const noexcept
{
    return *this | r;
}

constexpr inline QBox operator+(const QBox &rectangle, const QMargins &margins) noexcept
{
    return QBox(QPoint(rectangle.left() - margins.left(), rectangle.top() - margins.top()),
                QPoint(rectangle.right() + margins.right(), rectangle.bottom() + margins.bottom()));
}

constexpr inline QBox operator+(const QMargins &margins, const QBox &rectangle) noexcept
{
    return QBox(QPoint(rectangle.left() - margins.left(), rectangle.top() - margins.top()),
                QPoint(rectangle.right() + margins.right(), rectangle.bottom() + margins.bottom()));
}

constexpr inline QBox operator-(const QBox &lhs, const QMargins &rhs) noexcept
{
    return QBox(QPoint(lhs.left() + rhs.left(), lhs.top() + rhs.top()),
                QPoint(lhs.right() - rhs.right(), lhs.bottom() - rhs.bottom()));
}

constexpr inline QBox QBox::marginsAdded(const QMargins &margins) const noexcept
{
    return QBox(QPoint(xp - margins.left(), yp - margins.top()),
                QSize(w + margins.left() + margins.right(), h + margins.top() + margins.bottom()));
}

constexpr inline QBox QBox::marginsRemoved(const QMargins &margins) const noexcept
{
    return QBox(QPoint(xp + margins.left(), yp + margins.top()),
                QSize(w - margins.left() - margins.right(), h - margins.top() - margins.bottom()));
}

constexpr inline QBox &QBox::operator+=(const QMargins &margins) noexcept
{
    *this = marginsAdded(margins);
    return *this;
}

constexpr inline QBox &QBox::operator-=(const QMargins &margins) noexcept
{
    *this = marginsRemoved(margins);
    return *this;
}

constexpr QBox QBox::span(const QPoint &p1, const QPoint &p2) noexcept
{
    return QBox(QPoint(qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y())),
                QPoint(qMax(p1.x(), p2.x()), qMax(p1.y(), p2.y())));
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QBox &);
#endif

class Q_CORE_EXPORT QBoxF
{
public:
    constexpr QBoxF() noexcept : xp(0.), yp(0.), w(0.), h(0.) { }
    constexpr QBoxF(const QPointF &topleft, const QSizeF &size) noexcept;
    constexpr QBoxF(const QPointF &topleft, const QPointF &bottomRight) noexcept;
    constexpr QBoxF(qreal left, qreal top, qreal width, qreal height) noexcept;
    constexpr QBoxF(const QBox &rect) noexcept;

    constexpr inline bool isNull() const noexcept;
    constexpr inline bool isEmpty() const noexcept;
    constexpr inline bool isValid() const noexcept;
    [[nodiscard]] QBoxF normalized() const noexcept;

    constexpr inline qreal left() const noexcept { return xp; }
    constexpr inline qreal top() const noexcept { return yp; }
    constexpr inline qreal right() const noexcept { return xp + w; }
    constexpr inline qreal bottom() const noexcept { return yp + h; }

    constexpr inline qreal x() const noexcept;
    constexpr inline qreal y() const noexcept;
    constexpr inline void setLeft(qreal pos) noexcept;
    constexpr inline void setTop(qreal pos) noexcept;
    constexpr inline void setRight(qreal pos) noexcept;
    constexpr inline void setBottom(qreal pos) noexcept;
    constexpr inline void setX(qreal pos) noexcept { setLeft(pos); }
    constexpr inline void setY(qreal pos) noexcept { setTop(pos); }

    constexpr inline QPointF topLeft() const noexcept { return QPointF(xp, yp); }
    constexpr inline QPointF bottomRight() const noexcept { return QPointF(xp + w, yp + h); }
    constexpr inline QPointF topRight() const noexcept { return QPointF(xp + w, yp); }
    constexpr inline QPointF bottomLeft() const noexcept { return QPointF(xp, yp + h); }
    constexpr inline QPointF center() const noexcept;

    constexpr inline void setTopLeft(const QPointF &p) noexcept;
    constexpr inline void setBottomRight(const QPointF &p) noexcept;
    constexpr inline void setTopRight(const QPointF &p) noexcept;
    constexpr inline void setBottomLeft(const QPointF &p) noexcept;

    constexpr inline void moveLeft(qreal pos) noexcept;
    constexpr inline void moveTop(qreal pos) noexcept;
    constexpr inline void moveRight(qreal pos) noexcept;
    constexpr inline void moveBottom(qreal pos) noexcept;
    constexpr inline void moveTopLeft(const QPointF &p) noexcept;
    constexpr inline void moveBottomRight(const QPointF &p) noexcept;
    constexpr inline void moveTopRight(const QPointF &p) noexcept;
    constexpr inline void moveBottomLeft(const QPointF &p) noexcept;
    constexpr inline void moveCenter(const QPointF &p) noexcept;

    constexpr inline void translate(qreal dx, qreal dy) noexcept;
    constexpr inline void translate(const QPointF &p) noexcept;

    [[nodiscard]] constexpr inline QBoxF translated(qreal dx, qreal dy) const noexcept;
    [[nodiscard]] constexpr inline QBoxF translated(const QPointF &p) const noexcept;

    [[nodiscard]] constexpr inline QBoxF transposed() const noexcept;

    constexpr inline void moveTo(qreal x, qreal y) noexcept;
    constexpr inline void moveTo(const QPointF &p) noexcept;

    constexpr inline void setRect(qreal x, qreal y, qreal w, qreal h) noexcept;
    constexpr inline void getRect(qreal *x, qreal *y, qreal *w, qreal *h) const;

    constexpr inline void setCoords(qreal x1, qreal y1, qreal x2, qreal y2) noexcept;
    constexpr inline void getCoords(qreal *x1, qreal *y1, qreal *x2, qreal *y2) const;

    constexpr inline void adjust(qreal x1, qreal y1, qreal x2, qreal y2) noexcept;
    [[nodiscard]] constexpr inline QBoxF adjusted(qreal x1, qreal y1, qreal x2,
                                                  qreal y2) const noexcept;

    constexpr inline QSizeF size() const noexcept;
    constexpr inline qreal width() const noexcept;
    constexpr inline qreal height() const noexcept;
    constexpr inline void setWidth(qreal w) noexcept;
    constexpr inline void setHeight(qreal h) noexcept;
    constexpr inline void setSize(const QSizeF &s) noexcept;

    QBoxF operator|(const QBoxF &r) const noexcept;
    QBoxF operator&(const QBoxF &r) const noexcept;
    inline QBoxF &operator|=(const QBoxF &r) noexcept;
    inline QBoxF &operator&=(const QBoxF &r) noexcept;

    bool contains(const QBoxF &r) const noexcept;
    bool contains(const QPointF &p) const noexcept;
    inline bool contains(qreal x, qreal y) const noexcept;
    [[nodiscard]] inline QBoxF united(const QBoxF &other) const noexcept;
    [[nodiscard]] inline QBoxF intersected(const QBoxF &other) const noexcept;
    bool intersects(const QBoxF &r) const noexcept;

    constexpr inline QBoxF marginsAdded(const QMarginsF &margins) const noexcept;
    constexpr inline QBoxF marginsRemoved(const QMarginsF &margins) const noexcept;
    constexpr inline QBoxF &operator+=(const QMarginsF &margins) noexcept;
    constexpr inline QBoxF &operator-=(const QMarginsF &margins) noexcept;

    friend constexpr inline bool operator==(const QBoxF &r1, const QBoxF &r2) noexcept
    {
        return r1.topLeft() == r2.topLeft() && r1.size() == r2.size();
    }
    friend constexpr inline bool operator!=(const QBoxF &r1, const QBoxF &r2) noexcept
    {
        return r1.topLeft() != r2.topLeft() || r1.size() != r2.size();
    }

    [[nodiscard]] constexpr inline QBox toBox() const noexcept;
    [[nodiscard]] QBox toAlignedRect() const noexcept;

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    [[nodiscard]] static QBoxF fromCGRect(CGRect rect) noexcept;
    [[nodiscard]] CGRect toCGRect() const noexcept;
#endif

#if defined(Q_OS_WASM) || defined(Q_QDOC)
    [[nodiscard]] static QBoxF fromDOMRect(emscripten::val domRect);
    [[nodiscard]] emscripten::val toDOMRect() const;
#endif

private:
    qreal xp;
    qreal yp;
    qreal w;
    qreal h;
};
Q_DECLARE_TYPEINFO(QBoxF, Q_RELOCATABLE_TYPE);

/*****************************************************************************
  QBoxF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QBoxF &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QBoxF &);
#endif

/*****************************************************************************
  QBoxF inline member functions
 *****************************************************************************/

constexpr inline QBoxF::QBoxF(qreal aleft, qreal atop, qreal awidth, qreal aheight) noexcept
    : xp(aleft), yp(atop), w(awidth), h(aheight)
{
}

constexpr inline QBoxF::QBoxF(const QPointF &atopLeft, const QSizeF &asize) noexcept
    : xp(atopLeft.x()), yp(atopLeft.y()), w(asize.width()), h(asize.height())
{
}

constexpr inline QBoxF::QBoxF(const QPointF &atopLeft, const QPointF &abottomRight) noexcept
    : xp(atopLeft.x()),
      yp(atopLeft.y()),
      w(abottomRight.x() - atopLeft.x()),
      h(abottomRight.y() - atopLeft.y())
{
}

constexpr inline QBoxF::QBoxF(const QBox &r) noexcept
    : xp(r.x()), yp(r.y()), w(r.width()), h(r.height())
{
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE

constexpr inline bool QBoxF::isNull() const noexcept
{
    return w == 0. && h == 0.;
}

constexpr inline bool QBoxF::isEmpty() const noexcept
{
    return w <= 0. || h <= 0.;
}

QT_WARNING_POP

constexpr inline bool QBoxF::isValid() const noexcept
{
    return w > 0. && h > 0.;
}

constexpr inline qreal QBoxF::x() const noexcept
{
    return xp;
}

constexpr inline qreal QBoxF::y() const noexcept
{
    return yp;
}

constexpr inline void QBoxF::setLeft(qreal pos) noexcept
{
    qreal diff = pos - xp;
    xp += diff;
    w -= diff;
}

constexpr inline void QBoxF::setRight(qreal pos) noexcept
{
    w = pos - xp;
}

constexpr inline void QBoxF::setTop(qreal pos) noexcept
{
    qreal diff = pos - yp;
    yp += diff;
    h -= diff;
}

constexpr inline void QBoxF::setBottom(qreal pos) noexcept
{
    h = pos - yp;
}

constexpr inline void QBoxF::setTopLeft(const QPointF &p) noexcept
{
    setLeft(p.x());
    setTop(p.y());
}

constexpr inline void QBoxF::setTopRight(const QPointF &p) noexcept
{
    setRight(p.x());
    setTop(p.y());
}

constexpr inline void QBoxF::setBottomLeft(const QPointF &p) noexcept
{
    setLeft(p.x());
    setBottom(p.y());
}

constexpr inline void QBoxF::setBottomRight(const QPointF &p) noexcept
{
    setRight(p.x());
    setBottom(p.y());
}

constexpr inline QPointF QBoxF::center() const noexcept
{
    return QPointF(xp + w / 2, yp + h / 2);
}

constexpr inline void QBoxF::moveLeft(qreal pos) noexcept
{
    xp = pos;
}

constexpr inline void QBoxF::moveTop(qreal pos) noexcept
{
    yp = pos;
}

constexpr inline void QBoxF::moveRight(qreal pos) noexcept
{
    xp = pos - w;
}

constexpr inline void QBoxF::moveBottom(qreal pos) noexcept
{
    yp = pos - h;
}

constexpr inline void QBoxF::moveTopLeft(const QPointF &p) noexcept
{
    moveLeft(p.x());
    moveTop(p.y());
}

constexpr inline void QBoxF::moveTopRight(const QPointF &p) noexcept
{
    moveRight(p.x());
    moveTop(p.y());
}

constexpr inline void QBoxF::moveBottomLeft(const QPointF &p) noexcept
{
    moveLeft(p.x());
    moveBottom(p.y());
}

constexpr inline void QBoxF::moveBottomRight(const QPointF &p) noexcept
{
    moveRight(p.x());
    moveBottom(p.y());
}

constexpr inline void QBoxF::moveCenter(const QPointF &p) noexcept
{
    xp = p.x() - w / 2;
    yp = p.y() - h / 2;
}

constexpr inline qreal QBoxF::width() const noexcept
{
    return w;
}

constexpr inline qreal QBoxF::height() const noexcept
{
    return h;
}

constexpr inline QSizeF QBoxF::size() const noexcept
{
    return QSizeF(w, h);
}

constexpr inline void QBoxF::translate(qreal dx, qreal dy) noexcept
{
    xp += dx;
    yp += dy;
}

constexpr inline void QBoxF::translate(const QPointF &p) noexcept
{
    xp += p.x();
    yp += p.y();
}

constexpr inline void QBoxF::moveTo(qreal ax, qreal ay) noexcept
{
    xp = ax;
    yp = ay;
}

constexpr inline void QBoxF::moveTo(const QPointF &p) noexcept
{
    xp = p.x();
    yp = p.y();
}

constexpr inline QBoxF QBoxF::translated(qreal dx, qreal dy) const noexcept
{
    return QBoxF(xp + dx, yp + dy, w, h);
}

constexpr inline QBoxF QBoxF::translated(const QPointF &p) const noexcept
{
    return QBoxF(xp + p.x(), yp + p.y(), w, h);
}

constexpr inline QBoxF QBoxF::transposed() const noexcept
{
    return QBoxF(topLeft(), size().transposed());
}

constexpr inline void QBoxF::getRect(qreal *ax, qreal *ay, qreal *aaw, qreal *aah) const
{
    *ax = this->xp;
    *ay = this->yp;
    *aaw = this->w;
    *aah = this->h;
}

constexpr inline void QBoxF::setRect(qreal ax, qreal ay, qreal aaw, qreal aah) noexcept
{
    this->xp = ax;
    this->yp = ay;
    this->w = aaw;
    this->h = aah;
}

constexpr inline void QBoxF::getCoords(qreal *xp1, qreal *yp1, qreal *xp2, qreal *yp2) const
{
    *xp1 = xp;
    *yp1 = yp;
    *xp2 = xp + w;
    *yp2 = yp + h;
}

constexpr inline void QBoxF::setCoords(qreal xp1, qreal yp1, qreal xp2, qreal yp2) noexcept
{
    xp = xp1;
    yp = yp1;
    w = xp2 - xp1;
    h = yp2 - yp1;
}

constexpr inline void QBoxF::adjust(qreal xp1, qreal yp1, qreal xp2, qreal yp2) noexcept
{
    xp += xp1;
    yp += yp1;
    w += xp2 - xp1;
    h += yp2 - yp1;
}

constexpr inline QBoxF QBoxF::adjusted(qreal xp1, qreal yp1, qreal xp2, qreal yp2) const noexcept
{
    return QBoxF(xp + xp1, yp + yp1, w + xp2 - xp1, h + yp2 - yp1);
}

constexpr inline void QBoxF::setWidth(qreal aw) noexcept
{
    this->w = aw;
}

constexpr inline void QBoxF::setHeight(qreal ah) noexcept
{
    this->h = ah;
}

constexpr inline void QBoxF::setSize(const QSizeF &s) noexcept
{
    w = s.width();
    h = s.height();
}

inline bool QBoxF::contains(qreal ax, qreal ay) const noexcept
{
    return contains(QPointF(ax, ay));
}

inline QBoxF &QBoxF::operator|=(const QBoxF &r) noexcept
{
    *this = *this | r;
    return *this;
}

inline QBoxF &QBoxF::operator&=(const QBoxF &r) noexcept
{
    *this = *this & r;
    return *this;
}

inline QBoxF QBoxF::intersected(const QBoxF &r) const noexcept
{
    return *this & r;
}

inline QBoxF QBoxF::united(const QBoxF &r) const noexcept
{
    return *this | r;
}

constexpr QBoxF QBox::toBoxF() const noexcept
{
    return *this;
}

constexpr inline QBox QBoxF::toBox() const noexcept
{
    // This rounding is designed to minimize the maximum possible difference
    // in topLeft(), bottomRight(), and size() after rounding.
    // All dimensions are at most off by 0.75, and topLeft by at most 0.5.
    const int nxp = qRound(xp);
    const int nyp = qRound(yp);
    const int nw = qRound(w + (xp - nxp) / 2);
    const int nh = qRound(h + (yp - nyp) / 2);
    return QBox(nxp, nyp, nw, nh);
}

constexpr inline QBoxF operator+(const QBoxF &lhs, const QMarginsF &rhs) noexcept
{
    return QBoxF(QPointF(lhs.left() - rhs.left(), lhs.top() - rhs.top()),
                 QSizeF(lhs.width() + rhs.left() + rhs.right(),
                        lhs.height() + rhs.top() + rhs.bottom()));
}

constexpr inline QBoxF operator+(const QMarginsF &lhs, const QBoxF &rhs) noexcept
{
    return QBoxF(QPointF(rhs.left() - lhs.left(), rhs.top() - lhs.top()),
                 QSizeF(rhs.width() + lhs.left() + lhs.right(),
                        rhs.height() + lhs.top() + lhs.bottom()));
}

constexpr inline QBoxF operator-(const QBoxF &lhs, const QMarginsF &rhs) noexcept
{
    return QBoxF(QPointF(lhs.left() + rhs.left(), lhs.top() + rhs.top()),
                 QSizeF(lhs.width() - rhs.left() - rhs.right(),
                        lhs.height() - rhs.top() - rhs.bottom()));
}

constexpr inline QBoxF QBoxF::marginsAdded(const QMarginsF &margins) const noexcept
{
    return QBoxF(
            QPointF(xp - margins.left(), yp - margins.top()),
            QSizeF(w + margins.left() + margins.right(), h + margins.top() + margins.bottom()));
}

constexpr inline QBoxF QBoxF::marginsRemoved(const QMarginsF &margins) const noexcept
{
    return QBoxF(
            QPointF(xp + margins.left(), yp + margins.top()),
            QSizeF(w - margins.left() - margins.right(), h - margins.top() - margins.bottom()));
}

constexpr inline QBoxF &QBoxF::operator+=(const QMarginsF &margins) noexcept
{
    *this = marginsAdded(margins);
    return *this;
}

constexpr inline QBoxF &QBoxF::operator-=(const QMarginsF &margins) noexcept
{
    *this = marginsRemoved(margins);
    return *this;
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QBoxF &);
#endif

QT_END_NAMESPACE

#endif // QBOX_H
