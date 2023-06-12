// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbox.h"
#include "qdatastream.h"
#include "qmath.h"

#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBox
    \inmodule QtCore
    \ingroup painting
    \reentrant

    \brief The QBox class defines a rectangle in the plane using
    integer precision.

    A rectangle is normally expressed as a top-left corner and a
    size. The size (width and height) of a QBox is always equivalent
    to the mathematical rectangle that forms the basis for its
    rendering.

    A QBox can be constructed with a set of left, top, width and
    height coordinates, or from a QPoint and a QSize. The following
    code creates two identical rectangles.

    \snippet code/src_corelib_tools_qrect.cpp 0

    The QBox class provides a collection of functions that return the
    various rectangle coordinates, and enable manipulation of
    these. QBox also provides functions to move the rectangle relative
    to the various coordinates. In addition there is a moveTo()
    function that moves the rectangle, leaving its top left corner at
    the given coordinates. Alternatively, the translate() function
    moves the rectangle the given offset relative to the current
    position, and the translated() function returns a translated copy
    of this rectangle.

    The size() function returns the rectangle's dimensions as a
    QSize. The dimensions can also be retrieved separately using the
    width() and height() functions. To manipulate the dimensions use
    the setSize(), setWidth() or setHeight() functions. Alternatively,
    the size can be changed by applying either of the functions
    setting the rectangle coordinates, for example, setBottom() or
    setRight().

    The contains() function tells whether a given point is inside the
    rectangle or not, and the intersects() function returns \c true if
    this rectangle intersects with a given rectangle. The QBox class
    also provides the intersected() function which returns the
    intersection rectangle, and the united() function which returns the
    rectangle that encloses the given rectangle and this:

    \table
    \row
    \li \inlineimage qrect-intersect.png
    \li \inlineimage qrect-unite.png
    \row
    \li intersected()
    \li united()
    \endtable

    The isEmpty() function returns \c true if the rectangle's width or
    height is less than, or equal to, 0. Note that an empty rectangle
    is not valid: The isValid() function returns \c true if both width
    and height is larger than 0. A null rectangle (isNull() == true)
    on the other hand, has both width and height set to 0.

    Note that due to the way QBox and QBoxF are defined, an
    empty QBox is defined in essentially the same way as QBoxF.

    Finally, QBox objects can be streamed as well as compared.

    \tableofcontents

    \section1 Rendering

    When using an \l {QPainter::Antialiasing}{anti-aliased} painter,
    the boundary line of a QBox will be rendered symmetrically on
    both sides of the mathematical rectangle's boundary line. But when
    using an aliased painter (the default) other rules apply.

    Then, when rendering with a one pixel wide pen the QBox's boundary
    line will be rendered to the right and below the mathematical
    rectangle's boundary line.

    When rendering with a two pixels wide pen the boundary line will
    be split in the middle by the mathematical rectangle. This will be
    the case whenever the pen is set to an even number of pixels,
    while rendering with a pen with an odd number of pixels, the spare
    pixel will be rendered to the right and below the mathematical
    rectangle as in the one pixel case.

    \table
    \row
        \li \inlineimage qrect-diagram-zero.png
        \li \inlineimage qrect-diagram-one.png
    \row
        \li Logical representation
        \li One pixel wide pen
    \row
        \li \inlineimage qrect-diagram-two.png
        \li \inlineimage qrect-diagram-three.png
    \row
        \li Two pixel wide pen
        \li Three pixel wide pen
    \endtable

    \section1 Coordinates

    The QBox class provides a collection of functions that return
    the various rectangle coordinates, and enable manipulation of
    these. QBox also provides functions to move the rectangle
    relative to the various coordinates.

    For example: the bottom(), setBottom() and moveBottom() functions:
    bottom() returns the y-coordinate of the rectangle's bottom edge,
    setBottom() sets the bottom edge of the rectangle to the given y
    coordinate (it may change the height, but will never change the
    rectangle's top edge) and moveBottom() moves the entire rectangle
    vertically, leaving the rectangle's bottom edge at the given y
    coordinate and its size unchanged.

    \image qrectf-coordinates.png

    It is also possible to add offsets to this rectangle's coordinates
    using the adjust() function, as well as retrieve a new rectangle
    based on adjustments of the original one using the adjusted()
    function. If either of the width and height is negative, use the
    normalized() function to retrieve a rectangle where the corners
    are swapped.

    In addition, QBox provides the getCoords() function which extracts
    the position of the rectangle's top-left and bottom-right corner,
    and the getRect() function which extracts the rectangle's top-left
    corner, width and height. Use the setCoords() and setRect()
    function to manipulate the rectangle's coordinates and dimensions
    in one go.

    \section1 Constraints

    QBox is limited to the minimum and maximum values for the \c int type.
    Operations on a QBox that could potentially result in values outside this
    range will result in undefined behavior.

    \sa QBoxF, QRegion
*/

/*****************************************************************************
  QBox member functions
 *****************************************************************************/

/*!
    \fn QBox::QBox()

    Constructs a null rectangle.

    \sa isNull()
*/

/*!
    \fn QBox::QBox(const QPoint &topLeft, const QPoint &bottomRight)

    Constructs a rectangle with the given \a topLeft and \a bottomRight corners, both included.

    If \a bottomRight is to higher and to the left of \a topLeft, the rectangle defined
    is instead non-inclusive of the corners.

    \note To ensure both points are included regardless of relative order, use span().

    \sa setTopLeft(), setBottomRight(), span()
*/

/*!
    \fn QBox::QBox(const QPoint &topLeft, const QSize &size)

    Constructs a rectangle with the given \a topLeft corner and the
    given \a size.

    \sa setTopLeft(), setSize()
*/

/*!
    \fn QBox::QBox(int x, int y, int width, int height)

    Constructs a rectangle with (\a x, \a y) as its top-left corner
    and the given \a width and \a height.

    \sa setRect()
*/

/*!
    \fn bool QBox::isNull() const

    Returns \c true if the rectangle is a null rectangle, otherwise
    returns \c false.

    A null rectangle has both the width and the height set to 0. A null rectangle
    is also empty, and hence is not valid.

    \sa isEmpty(), isValid()
*/

/*!
    \fn bool QBox::isEmpty() const

    Returns \c true if the rectangle is empty, otherwise returns \c false.

    An empty rectangle has width() <= 0 or height() <= 0.  An empty
    rectangle is not valid (i.e., isEmpty() == !isValid()).

    Use the normalized() function to retrieve a rectangle where the
    corners are swapped.

    \sa isNull(), isValid(), normalized()
*/

/*!
    \fn bool QBox::isValid() const

    Returns \c true if the rectangle is valid, otherwise returns \c false.

    A valid rectangle has a width() > 0 and height() > 0. Note that
    non-trivial operations like intersections are not defined for
    invalid rectangles. A valid rectangle is not empty (i.e., isValid()
    == !isEmpty()).

    \sa isNull(), isEmpty(), normalized()
*/

/*!
    Returns a normalized rectangle; i.e., a rectangle that has a
    non-negative width and height.

    If width() < 0 the function swaps the left and right corners, and
    it swaps the top and bottom corners if height() < 0. The corners
    are at the same time changed from being non-inclusive to inclusive.

    \sa isValid(), isEmpty()
*/

QBox QBox::normalized() const noexcept
{
    QBox r = *this;
    if (r.w < 0) {
        r.xp += r.w;
        r.w = -r.w;
    }
    if (r.h < 0) {
        r.yp += r.h;
        r.h = -r.h;
    }
    return r;
}

/*!
    \fn int QBox::left() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent
    to x().

    \sa setLeft(), topLeft(), bottomLeft()
*/

/*!
    \fn int QBox::top() const

    Returns the y-coordinate of the rectangle's top edge.
    Equivalent to y().

    \sa setTop(), topLeft(), topRight()
*/

/*!
    \fn int QBox::right() const

    Returns the x-coordinate of the rectangle's right edge. Equivalent
    to x() + width().

    \sa setRight(), topRight(), bottomRight()
*/

/*!
    \fn int QBox::bottom() const

    Returns the y-coordinate of the rectangle's bottom edge. Equivalent
    to y() + height().

    \sa setBottom(), bottomLeft(), bottomRight()
*/

/*!
    \fn int QBox::x() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent to left().

    \sa setX(), y(), topLeft()
*/

/*!
    \fn int QBox::y() const

    Returns the y-coordinate of the rectangle's top edge. Equivalent to top().

    \sa setY(), x(), topLeft()
*/

/*!
    \fn void QBox::setLeft(int x)

    Sets the left edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setX().

    \sa left(), moveLeft()
*/

/*!
    \fn void QBox::setTop(int y)

    Sets the top edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the
    bottom edge of the rectangle.

    Equivalent to setY().

    \sa top(), moveTop()
*/

/*!
    \fn void QBox::setRight(int x)

    Sets the right edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the left
    edge of the rectangle.

    \sa right(), moveRight()
*/

/*!
    \fn void QBox::setBottom(int y)

    Sets the bottom edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the top
    edge of the rectangle.

    \sa bottom(), moveBottom(),
*/

/*!
    \fn void QBox::setX(int x)

    Sets the left edge of the rectangle to the given \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setLeft().

    \sa x(), setY(), setTopLeft()
*/

/*!
    \fn void QBox::setY(int y)

    Sets the top edge of the rectangle to the given \a y
    coordinate. May change the height, but will never change the
    bottom edge of the rectangle.

    Equivalent to setTop().

    \sa y(), setX(), setTopLeft()
*/

/*!
    \fn void QBox::setTopLeft(const QPoint &position)

    Set the top-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-right corner of the rectangle.

    \sa topLeft(), moveTopLeft()
*/

/*!
    \fn void QBox::setBottomRight(const QPoint &position)

    Set the bottom-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-left corner of the rectangle.

    \sa bottomRight(), moveBottomRight()
*/

/*!
    \fn void QBox::setTopRight(const QPoint &position)

    Set the top-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-left corner of the rectangle.

    \sa topRight(), moveTopRight()
*/

/*!
    \fn void QBox::setBottomLeft(const QPoint &position)

    Set the bottom-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-right corner of the rectangle.

    \sa bottomLeft(), moveBottomLeft()
*/

/*!
    \fn QPoint QBox::topLeft() const

    Returns the position of the rectangle's top-left corner.

    \sa setTopLeft(), top(), left()
*/

/*!
    \fn QPoint QBox::bottomRight() const

    Returns the position of the rectangle's bottom-right corner.

    \sa setBottomRight(), bottom(), right()
*/

/*!
    \fn QPoint QBox::topRight() const

    Returns the position of the rectangle's top-right corner.

    \sa setTopRight(), top(), right()
*/

/*!
    \fn QPoint QBox::bottomLeft() const

    Returns the position of the rectangle's bottom-left corner.

    \sa setBottomLeft(), bottom(), left()
*/

/*!
    \fn QPoint QBox::center() const

    Returns the center point of the rectangle.

    \sa moveCenter()
*/

/*!
    \fn void QBox::getRect(int *x, int *y, int *width, int *height) const

    Extracts the position of the rectangle's top-left corner to *\a x
    and *\a y, and its dimensions to *\a width and *\a height.

    \sa setRect(), getCoords()
*/

/*!
    \fn void QBox::getCoords(int *x1, int *y1, int *x2, int *y2) const

    Extracts the position of the rectangle's top-left corner to *\a x1
    and *\a y1, and the position of the bottom-right corner to *\a x2
    and *\a y2.

    \sa setCoords(), getRect()
*/

/*!
    \fn void QBox::moveLeft(int x)

    Moves the rectangle horizontally, leaving the rectangle's left
    edge at the given \a x coordinate. The rectangle's size is
    unchanged.

    \sa left(), setLeft(), moveRight()
*/

/*!
    \fn void QBox::moveTop(int y)

    Moves the rectangle vertically, leaving the rectangle's top edge
    at the given \a y coordinate. The rectangle's size is unchanged.

    \sa top(), setTop(), moveBottom()
*/

/*!
    \fn void QBox::moveRight(int x)

    Moves the rectangle horizontally, leaving the rectangle's right
    edge at the given \a x coordinate. The rectangle's size is
    unchanged.

    \sa right(), setRight(), moveLeft()
*/

/*!
    \fn void QBox::moveBottom(int y)

    Moves the rectangle vertically, leaving the rectangle's bottom
    edge at the given \a y coordinate. The rectangle's size is
    unchanged.

    \sa bottom(), setBottom(), moveTop()
*/

/*!
    \fn void QBox::moveTopLeft(const QPoint &position)

    Moves the rectangle, leaving the top-left corner at the given \a
    position. The rectangle's size is unchanged.

    \sa setTopLeft(), moveTop(), moveLeft()
*/

/*!
    \fn void QBox::moveBottomRight(const QPoint &position)

    Moves the rectangle, leaving the bottom-right corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomRight(), moveRight(), moveBottom()
*/

/*!
    \fn void QBox::moveTopRight(const QPoint &position)

    Moves the rectangle, leaving the top-right corner at the given \a
    position. The rectangle's size is unchanged.

    \sa setTopRight(), moveTop(), moveRight()
*/

/*!
    \fn void QBox::moveBottomLeft(const QPoint &position)

    Moves the rectangle, leaving the bottom-left corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomLeft(), moveBottom(), moveLeft()
*/

/*!
    \fn void QBox::moveCenter(const QPoint &position)

    Moves the rectangle, leaving the center point at the given \a
    position. The rectangle's size is unchanged.

    \sa center()
*/

/*!
    \fn void QBox::moveTo(int x, int y)

    Moves the rectangle, leaving the top-left corner at the given
    position (\a x, \a y).  The rectangle's size is unchanged.

    \sa translate(), moveTopLeft()
*/

/*!
    \fn void QBox::moveTo(const QPoint &position)

    Moves the rectangle, leaving the top-left corner at the given \a
    position.
*/

/*!
    \fn void QBox::translate(int dx, int dy)

    Moves the rectangle \a dx along the x axis and \a dy along the y
    axis, relative to the current position. Positive values move the
    rectangle to the right and down.

    \sa moveTopLeft(), moveTo(), translated()
*/

/*!
    \fn void QBox::translate(const QPoint &offset)
    \overload

    Moves the rectangle \a{offset}.\l{QPoint::x()}{x()} along the x
    axis and \a{offset}.\l{QPoint::y()}{y()} along the y axis,
    relative to the current position.
*/

/*!
    \fn QBox QBox::translated(int dx, int dy) const

    Returns a copy of the rectangle that is translated \a dx along the
    x axis and \a dy along the y axis, relative to the current
    position. Positive values move the rectangle to the right and
    down.

    \sa translate()

*/

/*!
    \fn QBox QBox::translated(const QPoint &offset) const

    \overload

    Returns a copy of the rectangle that is translated
    \a{offset}.\l{QPoint::x()}{x()} along the x axis and
    \a{offset}.\l{QPoint::y()}{y()} along the y axis, relative to the
    current position.
*/

/*!
    \fn QBox QBox::transposed() const

    Returns a copy of the rectangle that has its width and height
    exchanged:

    \snippet code/src_corelib_tools_qrect.cpp 2

    \sa QSize::transposed()
*/

/*!
    \fn void QBox::setRect(int x, int y, int width, int height)

    Sets the coordinates of the rectangle's top-left corner to (\a{x},
    \a{y}), and its size to the given \a width and \a height.

    \sa getRect(), setCoords()
*/

/*!
    \fn void QBox::setCoords(int x1, int y1, int x2, int y2)

    Sets the coordinates of the rectangle's top-left corner to (\a x1,
    \a y1), and the coordinates of its bottom-right corner to (\a x2,
    \a y2).

    \sa getCoords(), setRect()
*/

/*! \fn QBox QBox::adjusted(int dx1, int dy1, int dx2, int dy2) const

    Returns a new rectangle with \a dx1, \a dy1, \a dx2 and \a dy2
    added respectively to the existing coordinates of this rectangle.

    \sa adjust()
*/

/*! \fn void QBox::adjust(int dx1, int dy1, int dx2, int dy2)

    Adds \a dx1, \a dy1, \a dx2 and \a dy2 respectively to the
    existing coordinates of the rectangle.

    \sa adjusted(), setRect()
*/

/*!
    \fn QSize QBox::size() const

    Returns the size of the rectangle.

    \sa setSize(), width(), height()
*/

/*!
    \fn int QBox::width() const

    Returns the width of the rectangle.

    \sa setWidth(), height(), size()
*/

/*!
    \fn int QBox::height() const

    Returns the height of the rectangle.

    \sa setHeight(), width(), size()
*/

/*!
    \fn void QBox::setWidth(int width)

    Sets the width of the rectangle to the given \a width. The right
    edge is changed, but not the left one.

    \sa width(), setSize()
*/

/*!
    \fn void QBox::setHeight(int height)

    Sets the height of the rectangle to the given \a height. The bottom
    edge is changed, but not the top one.

    \sa height(), setSize()
*/

/*!
    \fn void QBox::setSize(const QSize &size)

    Sets the size of the rectangle to the given \a size. The top-left
    corner is not moved.

    \sa size(), setWidth(), setHeight()
*/

/*!
    \fn bool QBox::contains(const QPoint &point) const

    Returns \c true if the given \a point is inside the rectangle,
    otherwise returns \c false. A point is considered to be inside
    the rectangle if its coordinates lie inside the rectangle or on the
    left or the top edge. An empty rectangle does not contain any point.

    \sa intersects()
*/

bool QBox::contains(const QPoint &p) const noexcept
{
    if (isNull())
        return false;

    int l = xp;
    int r = xp;
    if (w < 0)
        l += w;
    else
        r += w;

    if (p.x() < l || p.x() >= r)
        return false;

    int t = yp;
    int b = yp;
    if (h < 0)
        t += h;
    else
        b += h;

    if (p.y() < t || p.y() >= b)
        return false;

    return true;
}

/*!
    \fn bool QBox::contains(int x, int y) const
    \overload

    Returns \c true if the point (\a x, \a y) is inside this rectangle,
    otherwise returns \c false.
*/

/*!
    \fn bool QBox::contains(const QBox &rectangle) const
    \overload

    Returns \c true if the given \a rectangle is inside this rectangle.
    otherwise returns \c false.
*/

bool QBox::contains(const QBox &r) const noexcept
{
    if (isNull() || r.isNull())
        return false;

    int l1 = xp;
    int r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;

    int l2 = r.xp;
    int r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;

    if (l2 < l1 || r2 > r1)
        return false;

    int t1 = yp;
    int b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;

    int t2 = r.yp;
    int b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;

    if (t2 < t1 || b2 > b1)
        return false;

    return true;
}

/*!
    \fn QBox& QBox::operator|=(const QBox &rectangle)

    Unites this rectangle with the given \a rectangle.

    \sa united(), operator|()
*/

/*!
    \fn QBox& QBox::operator&=(const QBox &rectangle)

    Intersects this rectangle with the given \a rectangle.

    \sa intersected(), operator&()
*/

/*!
    \fn QBox QBox::operator|(const QBox &rectangle) const

    Returns the bounding rectangle of this rectangle and the given \a
    rectangle.

    \sa operator|=(), united()
*/

QBox QBox::operator|(const QBox &r) const noexcept
{
    if (isNull())
        return r;
    if (r.isNull())
        return *this;

    int left = xp;
    int right = xp;
    if (w < 0)
        left += w;
    else
        right += w;

    if (r.w < 0) {
        left = qMin(left, r.xp + r.w);
        right = qMax(right, r.xp);
    } else {
        left = qMin(left, r.xp);
        right = qMax(right, r.xp + r.w);
    }

    int top = yp;
    int bottom = yp;
    if (h < 0)
        top += h;
    else
        bottom += h;

    if (r.h < 0) {
        top = qMin(top, r.yp + r.h);
        bottom = qMax(bottom, r.yp);
    } else {
        top = qMin(top, r.yp);
        bottom = qMax(bottom, r.yp + r.h);
    }

    return QBox(left, top, right - left, bottom - top);
}

/*!
    \fn QBox QBox::united(const QBox &rectangle) const

    Returns the bounding rectangle of this rectangle and the given \a rectangle.

    \image qrect-unite.png

    \sa intersected()
*/

/*!
    \fn QBox QBox::operator&(const QBox &rectangle) const

    Returns the intersection of this rectangle and the given \a
    rectangle. Returns an empty rectangle if there is no intersection.

    \sa operator&=(), intersected()
*/

QBox QBox::operator&(const QBox &r) const noexcept
{
    if (isNull() || r.isNull())
        return QBox();

    int l1 = xp;
    int r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;

    int l2 = r.xp;
    int r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;

    if (l1 >= r2 || l2 >= r1)
        return QBox();

    int t1 = yp;
    int b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;

    int t2 = r.yp;
    int b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;

    if (t1 >= b2 || t2 >= b1)
        return QBox();

    QBox tmp;
    tmp.xp = qMax(l1, l2);
    tmp.yp = qMax(t1, t2);
    tmp.w = qMin(r1, r2) - tmp.xp;
    tmp.h = qMin(b1, b2) - tmp.yp;
    return tmp;
}

/*!
    \fn QBox QBox::intersected(const QBox &rectangle) const

    Returns the intersection of this rectangle and the given \a
    rectangle. Note that \c{r.intersected(s)} is equivalent to \c{r & s}.

    \image qrect-intersect.png

    \sa intersects(), united(), operator&=()
*/

/*!
    \fn bool QBox::intersects(const QBox &rectangle) const

    Returns \c true if this rectangle intersects with the given \a
    rectangle (i.e., there is at least one pixel that is within both
    rectangles), otherwise returns \c false.

    The intersection rectangle can be retrieved using the intersected()
    function.

    \sa contains()
*/

bool QBox::intersects(const QBox &r) const noexcept
{
    if (isNull() || r.isNull())
        return false;

    int l1 = xp;
    int r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;

    int l2 = r.xp;
    int r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;

    if (l1 >= r2 || l2 >= r1)
        return false;

    int t1 = yp;
    int b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;

    int t2 = r.yp;
    int b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;

    if (t1 >= b2 || t2 >= b1)
        return false;

    return true;
}

/*!
    \fn bool QBox::operator==(const QBox &r1, const QBox &r2)

    Returns \c true if the rectangles \a r1 and \a r2 are equal,
    otherwise returns \c false.
*/

/*!
    \fn bool QBox::operator!=(const QBox &r1, const QBox &r2)

    Returns \c true if the rectangles \a r1 and \a r2 are different, otherwise
    returns \c false.
*/

/*!
    \fn QBox operator+(const QBox &rectangle, const QMargins &margins)
    \relates QBox

    Returns the \a rectangle grown by the \a margins.
*/

/*!
    \fn QBox operator+(const QMargins &margins, const QBox &rectangle)
    \relates QBox
    \overload

    Returns the \a rectangle grown by the \a margins.
*/

/*!
    \fn QBox operator-(const QBox &lhs, const QMargins &rhs)
    \relates QBox

    Returns the \a lhs rectangle shrunk by the \a rhs margins.
*/

/*!
    \fn QBox QBox::marginsAdded(const QMargins &margins) const

    Returns a rectangle grown by the \a margins.

    \sa operator+=(), marginsRemoved(), operator-=()
*/

/*!
    \fn QBox QBox::operator+=(const QMargins &margins)

    Adds the \a margins to the rectangle, growing it.

    \sa marginsAdded(), marginsRemoved(), operator-=()
*/

/*!
    \fn QBox QBox::marginsRemoved(const QMargins &margins) const

    Removes the \a margins from the rectangle, shrinking it.

    \sa marginsAdded(), operator+=(), operator-=()
*/

/*!
    \fn QBox QBox::operator -=(const QMargins &margins)

    Returns a rectangle shrunk by the \a margins.

    \sa marginsRemoved(), operator+=(), marginsAdded()
*/

/*!
    \fn static QBox QBox::span(const QPoint &p1, const QPoint &p2)

    Returns a rectangle spanning the two points \a p1 and \a p2, including both and
    everything in between.
*/

/*!
    \fn QBox::toBoxF() const

    Returns this rectangle as a rectangle with floating point accuracy.

    \note This function, like the QBoxF(QBox) constructor, preserves the
    size() of the rectangle, not its bottomRight() corner.

    \sa QBoxF::toBox()
*/

/*****************************************************************************
  QBox stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QBox &rectangle)
    \relates QBox

    Writes the given \a rectangle to the given \a stream, and returns
    a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QBox &r)
{
    if (s.version() == 1)
        s << (qint16)r.left() << (qint16)r.top() << (qint16)r.right() << (qint16)r.bottom();
    else
        s << (qint32)r.left() << (qint32)r.top() << (qint32)r.right() << (qint32)r.bottom();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QBox &rectangle)
    \relates QBox

    Reads a rectangle from the given \a stream into the given \a
    rectangle, and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QBox &r)
{
    if (s.version() == 1) {
        qint16 x1, y1, x2, y2;
        s >> x1;
        s >> y1;
        s >> x2;
        s >> y2;
        r.setCoords(x1, y1, x2, y2);
    } else {
        qint32 x1, y1, x2, y2;
        s >> x1;
        s >> y1;
        s >> x2;
        s >> y2;
        r.setCoords(x1, y1, x2, y2);
    }
    return s;
}

#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QBox &r)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QBox" << '(';
    QtDebugUtils::formatQRect(dbg, r);
    dbg << ')';
    return dbg;
}
#endif

/*!
    \class QBoxF
    \inmodule QtCore
    \ingroup painting
    \reentrant

    \brief The QBoxF class defines a finite rectangle in the plane using
    floating point precision.

    A rectangle is normally expressed as a top-left corner and a
    size.  The size (width and height) of a QBoxF is always equivalent
    to the mathematical rectangle that forms the basis for its
    rendering.

    A QBoxF can be constructed with a set of left, top, width and
    height coordinates, or from a QPointF and a QSizeF.  The following
    code creates two identical rectangles.

    \snippet code/src_corelib_tools_qrect.cpp 1

    There is also a third constructor creating a QBoxF from a QBox,
    and a corresponding toBox() function that returns a QBox object
    based on the values of this rectangle (note that the coordinates
    in the returned rectangle are rounded to the nearest integer).

    The QBoxF class provides a collection of functions that return
    the various rectangle coordinates, and enable manipulation of
    these. QBoxF also provides functions to move the rectangle
    relative to the various coordinates. In addition there is a
    moveTo() function that moves the rectangle, leaving its top left
    corner at the given coordinates. Alternatively, the translate()
    function moves the rectangle the given offset relative to the
    current position, and the translated() function returns a
    translated copy of this rectangle.

    The size() function returns the rectangle's dimensions as a
    QSizeF. The dimensions can also be retrieved separately using the
    width() and height() functions. To manipulate the dimensions use
    the setSize(), setWidth() or setHeight() functions. Alternatively,
    the size can be changed by applying either of the functions
    setting the rectangle coordinates, for example, setBottom() or
    setRight().

    The contains() function tells whether a given point is inside the
    rectangle or not, and the intersects() function returns \c true if
    this rectangle intersects with a given rectangle (otherwise
    false). The QBoxF class also provides the intersected() function
    which returns the intersection rectangle, and the united() function
    which returns the rectangle that encloses the given rectangle and
    this:

    \table
    \row
    \li \inlineimage qrect-intersect.png
    \li \inlineimage qrect-unite.png
    \row
    \li intersected()
    \li united()
    \endtable

    The isEmpty() function returns \c true if the rectangle's width or
    height is less than, or equal to, 0. Note that an empty rectangle
    is not valid: The isValid() function returns \c true if both width
    and height is larger than 0. A null rectangle (isNull() == true)
    on the other hand, has both width and height set to 0.

    Note that due to the way QBox and QBoxF are defined, an
    empty QBoxF is defined in essentially the same way as QBox.

    Finally, QBoxF objects can be streamed as well as compared.

    \tableofcontents

    \section1 Rendering

    When using an \l {QPainter::Antialiasing}{anti-aliased} painter,
    the boundary line of a QBoxF will be rendered symmetrically on both
    sides of the mathematical rectangle's boundary line. But when
    using an aliased painter (the default) other rules apply.

    Then, when rendering with a one pixel wide pen the QBoxF's boundary
    line will be rendered to the right and below the mathematical
    rectangle's boundary line.

    When rendering with a two pixels wide pen the boundary line will
    be split in the middle by the mathematical rectangle. This will be
    the case whenever the pen is set to an even number of pixels,
    while rendering with a pen with an odd number of pixels, the spare
    pixel will be rendered to the right and below the mathematical
    rectangle as in the one pixel case.

    \table
    \row
        \li \inlineimage qrect-diagram-zero.png
        \li \inlineimage qrectf-diagram-one.png
    \row
        \li Logical representation
        \li One pixel wide pen
    \row
        \li \inlineimage qrectf-diagram-two.png
        \li \inlineimage qrectf-diagram-three.png
    \row
        \li Two pixel wide pen
        \li Three pixel wide pen
    \endtable

    \section1 Coordinates

    The QBoxF class provides a collection of functions that return
    the various rectangle coordinates, and enable manipulation of
    these. QBoxF also provides functions to move the rectangle
    relative to the various coordinates.

    For example: the bottom(), setBottom() and moveBottom() functions:
    bottom() returns the y-coordinate of the rectangle's bottom edge,
    setBottom() sets the bottom edge of the rectangle to the given y
    coordinate (it may change the height, but will never change the
    rectangle's top edge) and moveBottom() moves the entire rectangle
    vertically, leaving the rectangle's bottom edge at the given y
    coordinate and its size unchanged.

    \image qrectf-coordinates.png

    It is also possible to add offsets to this rectangle's coordinates
    using the adjust() function, as well as retrieve a new rectangle
    based on adjustments of the original one using the adjusted()
    function. If either of the width and height is negative, use the
    normalized() function to retrieve a rectangle where the corners
    are swapped.

    In addition, QBoxF provides the getCoords() function which extracts
    the position of the rectangle's top-left and bottom-right corner,
    and the getRect() function which extracts the rectangle's top-left
    corner, width and height. Use the setCoords() and setRect()
    function to manipulate the rectangle's coordinates and dimensions
    in one go.

    \sa QBox, QRegion
*/

/*****************************************************************************
  QBoxF member functions
 *****************************************************************************/

/*!
    \fn QBoxF::QBoxF()

    Constructs a null rectangle.

    \sa isNull()
*/

/*!
    \fn QBoxF::QBoxF(const QPointF &topLeft, const QSizeF &size)

    Constructs a rectangle with the given \a topLeft corner and the given \a size.

    \sa setTopLeft(), setSize()
*/

/*!
    \fn QBoxF::QBoxF(const QPointF &topLeft, const QPointF &bottomRight)

    Constructs a rectangle with the given \a topLeft and \a bottomRight corners.

    \sa setTopLeft(), setBottomRight()
*/

/*!
    \fn QBoxF::QBoxF(qreal x, qreal y, qreal width, qreal height)

    Constructs a rectangle with (\a x, \a y) as its top-left corner and the
    given \a width and \a height. All parameters must be finite.

    \sa setRect()
*/

/*!
    \fn QBoxF::QBoxF(const QBox &rectangle)

    Constructs a QBoxF rectangle from the given QBox \a rectangle.

    \note This function, like QBox::toBoxF(), preserves the size() of
    \a rectangle, not its bottomRight() corner.

    \sa toBox(), QBox::toBoxF()
*/

/*!
    \fn bool QBoxF::isNull() const

    Returns \c true if the rectangle is a null rectangle, otherwise returns \c false.

    A null rectangle has both the width and the height set to 0. A
    null rectangle is also empty, and hence not valid.

    \sa isEmpty(), isValid()
*/

/*!
    \fn bool QBoxF::isEmpty() const

    Returns \c true if the rectangle is empty, otherwise returns \c false.

    An empty rectangle has width() <= 0 or height() <= 0.  An empty
    rectangle is not valid (i.e., isEmpty() == !isValid()).

    Use the normalized() function to retrieve a rectangle where the
    corners are swapped.

    \sa isNull(), isValid(), normalized()
*/

/*!
    \fn bool QBoxF::isValid() const

    Returns \c true if the rectangle is valid, otherwise returns \c false.

    A valid rectangle has a width() > 0 and height() > 0. Note that
    non-trivial operations like intersections are not defined for
    invalid rectangles. A valid rectangle is not empty (i.e., isValid()
    == !isEmpty()).

    \sa isNull(), isEmpty(), normalized()
*/

/*!
    Returns a normalized rectangle; i.e., a rectangle that has a
    non-negative width and height.

    If width() < 0 the function swaps the left and right corners, and
    it swaps the top and bottom corners if height() < 0.

    \sa isValid(), isEmpty()
*/

QBoxF QBoxF::normalized() const noexcept
{
    QBoxF r = *this;
    if (r.w < 0) {
        r.xp += r.w;
        r.w = -r.w;
    }
    if (r.h < 0) {
        r.yp += r.h;
        r.h = -r.h;
    }
    return r;
}

/*!
    \fn qreal QBoxF::x() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent
    to left().


    \sa setX(), y(), topLeft()
*/

/*!
    \fn qreal QBoxF::y() const

    Returns the y-coordinate of the rectangle's top edge. Equivalent
    to top().

    \sa setY(), x(), topLeft()
*/

/*!
    \fn void QBoxF::setLeft(qreal x)

    Sets the left edge of the rectangle to the given finite \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setX().

    \sa left(), moveLeft()
*/

/*!
    \fn void QBoxF::setTop(qreal y)

    Sets the top edge of the rectangle to the given finite \a y coordinate. May
    change the height, but will never change the bottom edge of the
    rectangle.

    Equivalent to setY().

    \sa top(), moveTop()
*/

/*!
    \fn void QBoxF::setRight(qreal x)

    Sets the right edge of the rectangle to the given finite \a x
    coordinate. May change the width, but will never change the left
    edge of the rectangle.

    \sa right(), moveRight()
*/

/*!
    \fn void QBoxF::setBottom(qreal y)

    Sets the bottom edge of the rectangle to the given finite \a y
    coordinate. May change the height, but will never change the top
    edge of the rectangle.

    \sa bottom(), moveBottom()
*/

/*!
    \fn void QBoxF::setX(qreal x)

    Sets the left edge of the rectangle to the given finite \a x
    coordinate. May change the width, but will never change the right
    edge of the rectangle.

    Equivalent to setLeft().

    \sa x(), setY(), setTopLeft()
*/

/*!
    \fn void QBoxF::setY(qreal y)

    Sets the top edge of the rectangle to the given finite \a y
    coordinate. May change the height, but will never change the
    bottom edge of the rectangle.

    Equivalent to setTop().

    \sa y(), setX(), setTopLeft()
*/

/*!
    \fn void QBoxF::setTopLeft(const QPointF &position)

    Set the top-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-right corner of the rectangle.

    \sa topLeft(), moveTopLeft()
*/

/*!
    \fn void QBoxF::setBottomRight(const QPointF &position)

    Set the bottom-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-left corner of the rectangle.

    \sa bottomRight(), moveBottomRight()
*/

/*!
    \fn void QBoxF::setTopRight(const QPointF &position)

    Set the top-right corner of the rectangle to the given \a
    position. May change the size, but will never change the
    bottom-left corner of the rectangle.

    \sa topRight(), moveTopRight()
*/

/*!
    \fn void QBoxF::setBottomLeft(const QPointF &position)

    Set the bottom-left corner of the rectangle to the given \a
    position. May change the size, but will never change the
    top-right corner of the rectangle.

    \sa bottomLeft(), moveBottomLeft()
*/

/*!
    \fn QPointF QBoxF::center() const

    Returns the center point of the rectangle.

    \sa moveCenter()
*/

/*!
    \fn void QBoxF::getRect(qreal *x, qreal *y, qreal *width, qreal *height) const

    Extracts the position of the rectangle's top-left corner to *\a x and
    *\a y, and its dimensions to *\a width and *\a height.

    \sa setRect(), getCoords()
*/

/*!
    \fn void QBoxF::getCoords(qreal *x1, qreal *y1, qreal *x2, qreal *y2) const

    Extracts the position of the rectangle's top-left corner to *\a x1
    and *\a y1, and the position of the bottom-right corner to *\a x2 and
    *\a y2.

    \sa setCoords(), getRect()
*/

/*!
    \fn void QBoxF::moveLeft(qreal x)

    Moves the rectangle horizontally, leaving the rectangle's left
    edge at the given finite \a x coordinate. The rectangle's size is
    unchanged.

    \sa left(), setLeft(), moveRight()
*/

/*!
    \fn void QBoxF::moveTop(qreal y)

    Moves the rectangle vertically, leaving the rectangle's top line
    at the given finite \a y coordinate. The rectangle's size is unchanged.

    \sa top(), setTop(), moveBottom()
*/

/*!
    \fn void QBoxF::moveRight(qreal x)

    Moves the rectangle horizontally, leaving the rectangle's right
    edge at the given finite \a x coordinate. The rectangle's size is
    unchanged.

    \sa right(), setRight(), moveLeft()
*/

/*!
    \fn void QBoxF::moveBottom(qreal y)

    Moves the rectangle vertically, leaving the rectangle's bottom
    edge at the given finite \a y coordinate. The rectangle's size is
    unchanged.

    \sa bottom(), setBottom(), moveTop()
*/

/*!
    \fn void QBoxF::moveTopLeft(const QPointF &position)

    Moves the rectangle, leaving the top-left corner at the given \a
    position. The rectangle's size is unchanged.

    \sa setTopLeft(), moveTop(), moveLeft()
*/

/*!
    \fn void QBoxF::moveBottomRight(const QPointF &position)

    Moves the rectangle, leaving the bottom-right corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomRight(), moveBottom(), moveRight()
*/

/*!
    \fn void QBoxF::moveTopRight(const QPointF &position)

    Moves the rectangle, leaving the top-right corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setTopRight(), moveTop(), moveRight()
*/

/*!
    \fn void QBoxF::moveBottomLeft(const QPointF &position)

    Moves the rectangle, leaving the bottom-left corner at the given
    \a position. The rectangle's size is unchanged.

    \sa setBottomLeft(), moveBottom(), moveLeft()
*/

/*!
    \fn void QBoxF::moveTo(qreal x, qreal y)

    Moves the rectangle, leaving the top-left corner at the given position (\a
    x, \a y). The rectangle's size is unchanged. Both parameters must be finite.

    \sa translate(), moveTopLeft()
*/

/*!
    \fn void QBoxF::moveTo(const QPointF &position)
    \overload

    Moves the rectangle, leaving the top-left corner at the given \a
    position.
*/

/*!
    \fn void QBoxF::translate(qreal dx, qreal dy)

    Moves the rectangle \a dx along the x-axis and \a dy along the y-axis,
    relative to the current position. Positive values move the rectangle to the
    right and downwards. Both parameters must be finite.

    \sa moveTopLeft(), moveTo(), translated()
*/

/*!
    \fn void QBoxF::translate(const QPointF &offset)
    \overload

    Moves the rectangle \a{offset}.\l{QPointF::x()}{x()} along the x
    axis and \a{offset}.\l{QPointF::y()}{y()} along the y axis,
    relative to the current position.
*/

/*!
    \fn QBoxF QBoxF::translated(qreal dx, qreal dy) const

    Returns a copy of the rectangle that is translated \a dx along the
    x axis and \a dy along the y axis, relative to the current
    position. Positive values move the rectangle to the right and
    down. Both parameters must be finite.

    \sa translate()
*/

/*!
    \fn QBoxF QBoxF::translated(const QPointF &offset) const
    \overload

    Returns a copy of the rectangle that is translated
    \a{offset}.\l{QPointF::x()}{x()} along the x axis and
    \a{offset}.\l{QPointF::y()}{y()} along the y axis, relative to the
    current position.
*/

/*!
    \fn QBoxF QBoxF::transposed() const

    Returns a copy of the rectangle that has its width and height
    exchanged:

    \snippet code/src_corelib_tools_qrect.cpp 3

    \sa QSizeF::transposed()
*/

/*!
    \fn void QBoxF::setRect(qreal x, qreal y, qreal width, qreal height)

    Sets the coordinates of the rectangle's top-left corner to (\a x, \a y), and
    its size to the given \a width and \a height. All parameters must be finite.

    \sa getRect(), setCoords()
*/

/*!
    \fn void QBoxF::setCoords(qreal x1, qreal y1, qreal x2, qreal y2)

    Sets the coordinates of the rectangle's top-left corner to (\a x1,
    \a y1), and the coordinates of its bottom-right corner to (\a x2,
    \a y2). All parameters must be finite.

    \sa getCoords(), setRect()
*/

/*!
    \fn QBoxF QBoxF::adjusted(qreal dx1, qreal dy1, qreal dx2, qreal dy2) const

    Returns a new rectangle with \a dx1, \a dy1, \a dx2 and \a dy2
    added respectively to the existing coordinates of this rectangle.
    All parameters must be finite.

    \sa adjust()
*/

/*! \fn void QBoxF::adjust(qreal dx1, qreal dy1, qreal dx2, qreal dy2)

    Adds \a dx1, \a dy1, \a dx2 and \a dy2 respectively to the
    existing coordinates of the rectangle. All parameters must be finite.

    \sa adjusted(), setRect()
*/
/*!
    \fn QSizeF QBoxF::size() const

    Returns the size of the rectangle.

    \sa setSize(), width(), height()
*/

/*!
    \fn qreal QBoxF::width() const

    Returns the width of the rectangle.

    \sa setWidth(), height(), size()
*/

/*!
    \fn qreal QBoxF::height() const

    Returns the height of the rectangle.

    \sa setHeight(), width(), size()
*/

/*!
    \fn void QBoxF::setWidth(qreal width)

    Sets the width of the rectangle to the given finite \a width. The right
    edge is changed, but not the left one.

    \sa width(), setSize()
*/

/*!
    \fn void QBoxF::setHeight(qreal height)

    Sets the height of the rectangle to the given finite \a height. The bottom
    edge is changed, but not the top one.

    \sa height(), setSize()
*/

/*!
    \fn void QBoxF::setSize(const QSizeF &size)

    Sets the size of the rectangle to the given finite \a size. The top-left
    corner is not moved.

    \sa size(), setWidth(), setHeight()
*/

/*!
    \fn bool QBoxF::contains(const QPointF &point) const

    Returns \c true if the given \a p is inside the rectangle,
    otherwise returns \c false. A point is considered to be inside
    the rectangle if its coordinates lie inside the rectangle or on the
    left or the top edge. An empty rectangle does not contain any point.

    \sa intersects()
*/

bool QBoxF::contains(const QPointF &p) const noexcept
{
    qreal l = xp;
    qreal r = xp;
    if (w < 0)
        l += w;
    else
        r += w;
    if (l == r) // null rect
        return false;

    if (p.x() < l || p.x() >= r)
        return false;

    qreal t = yp;
    qreal b = yp;
    if (h < 0)
        t += h;
    else
        b += h;
    if (t == b) // null rect
        return false;

    if (p.y() < t || p.y() >= b)
        return false;

    return true;
}

/*!
    \fn bool QBoxF::contains(qreal x, qreal y) const
    \overload

    Returns \c true if the given point (\a x, \a y) is inside the rectangle,
    otherwise returns \c false. A point is considered to be inside
    the rectangle if its coordinates lie inside the rectangle or on the
    left or the top edge. An empty rectangle does not contain any point.
*/

/*!
    \fn bool QBoxF::contains(const QBoxF &rectangle) const
    \overload

    Returns \c true if the given \a rectangle is inside this rectangle;
    otherwise returns \c false.
*/

bool QBoxF::contains(const QBoxF &r) const noexcept
{
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1) // null rect
        return false;

    qreal l2 = r.xp;
    qreal r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2) // null rect
        return false;

    if (l2 < l1 || r2 > r1)
        return false;

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1) // null rect
        return false;

    qreal t2 = r.yp;
    qreal b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2) // null rect
        return false;

    if (t2 < t1 || b2 > b1)
        return false;

    return true;
}

/*!
    \fn qreal QBoxF::left() const

    Returns the x-coordinate of the rectangle's left edge. Equivalent
    to x().

    \sa setLeft(), topLeft(), bottomLeft()
*/

/*!
    \fn qreal QBoxF::top() const

    Returns the y-coordinate of the rectangle's top edge. Equivalent
    to y().

    \sa setTop(), topLeft(), topRight()
*/

/*!
    \fn qreal QBoxF::right() const

    Returns the x-coordinate of the rectangle's right edge.

    \sa setRight(), topRight(), bottomRight()
*/

/*!
    \fn qreal QBoxF::bottom() const

    Returns the y-coordinate of the rectangle's bottom edge.

    \sa setBottom(), bottomLeft(), bottomRight()
*/

/*!
    \fn QPointF QBoxF::topLeft() const

    Returns the position of the rectangle's top-left corner.

    \sa setTopLeft(), top(), left()
*/

/*!
    \fn QPointF QBoxF::bottomRight() const

    Returns the position of the rectangle's  bottom-right corner.

    \sa setBottomRight(), bottom(), right()
*/

/*!
    \fn QPointF QBoxF::topRight() const

    Returns the position of the rectangle's top-right corner.

    \sa setTopRight(), top(), right()
*/

/*!
    \fn QPointF QBoxF::bottomLeft() const

    Returns the position of the rectangle's  bottom-left corner.

    \sa setBottomLeft(), bottom(), left()
*/

/*!
    \fn QBoxF& QBoxF::operator|=(const QBoxF &rectangle)

    Unites this rectangle with the given \a rectangle.

    \sa united(), operator|()
*/

/*!
    \fn QBoxF& QBoxF::operator&=(const QBoxF &rectangle)

    Intersects this rectangle with the given \a rectangle.

    \sa intersected(), operator&()
*/

/*!
    \fn QBoxF QBoxF::operator|(const QBoxF &rectangle) const

    Returns the bounding rectangle of this rectangle and the given \a rectangle.

    \sa united(), operator|=()
*/

QBoxF QBoxF::operator|(const QBoxF &r) const noexcept
{
    if (isNull())
        return r;
    if (r.isNull())
        return *this;

    qreal left = xp;
    qreal right = xp;
    if (w < 0)
        left += w;
    else
        right += w;

    if (r.w < 0) {
        left = qMin(left, r.xp + r.w);
        right = qMax(right, r.xp);
    } else {
        left = qMin(left, r.xp);
        right = qMax(right, r.xp + r.w);
    }

    qreal top = yp;
    qreal bottom = yp;
    if (h < 0)
        top += h;
    else
        bottom += h;

    if (r.h < 0) {
        top = qMin(top, r.yp + r.h);
        bottom = qMax(bottom, r.yp);
    } else {
        top = qMin(top, r.yp);
        bottom = qMax(bottom, r.yp + r.h);
    }

    return QBoxF(left, top, right - left, bottom - top);
}

/*!
    \fn QBoxF QBoxF::united(const QBoxF &rectangle) const

    Returns the bounding rectangle of this rectangle and the given \a
    rectangle.

    \image qrect-unite.png

    \sa intersected()
*/

/*!
    \fn QBoxF QBoxF::operator &(const QBoxF &rectangle) const

    Returns the intersection of this rectangle and the given \a
    rectangle. Returns an empty rectangle if there is no intersection.

    \sa operator&=(), intersected()
*/

QBoxF QBoxF::operator&(const QBoxF &r) const noexcept
{
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1) // null rect
        return QBoxF();

    qreal l2 = r.xp;
    qreal r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2) // null rect
        return QBoxF();

    if (l1 >= r2 || l2 >= r1)
        return QBoxF();

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1) // null rect
        return QBoxF();

    qreal t2 = r.yp;
    qreal b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2) // null rect
        return QBoxF();

    if (t1 >= b2 || t2 >= b1)
        return QBoxF();

    QBoxF tmp;
    tmp.xp = qMax(l1, l2);
    tmp.yp = qMax(t1, t2);
    tmp.w = qMin(r1, r2) - tmp.xp;
    tmp.h = qMin(b1, b2) - tmp.yp;
    return tmp;
}

/*!
    \fn QBoxF QBoxF::intersected(const QBoxF &rectangle) const

    Returns the intersection of this rectangle and the given \a
    rectangle. Note that \c {r.intersected(s)} is equivalent to \c
    {r & s}.

    \image qrect-intersect.png

    \sa intersects(), united(), operator&=()
*/

/*!
    \fn bool QBoxF::intersects(const QBoxF &rectangle) const

    Returns \c true if this rectangle intersects with the given \a
    rectangle (i.e. there is a non-empty area of overlap between
    them), otherwise returns \c false.

    The intersection rectangle can be retrieved using the intersected()
    function.

    \sa contains()
*/

bool QBoxF::intersects(const QBoxF &r) const noexcept
{
    qreal l1 = xp;
    qreal r1 = xp;
    if (w < 0)
        l1 += w;
    else
        r1 += w;
    if (l1 == r1) // null rect
        return false;

    qreal l2 = r.xp;
    qreal r2 = r.xp;
    if (r.w < 0)
        l2 += r.w;
    else
        r2 += r.w;
    if (l2 == r2) // null rect
        return false;

    if (l1 >= r2 || l2 >= r1)
        return false;

    qreal t1 = yp;
    qreal b1 = yp;
    if (h < 0)
        t1 += h;
    else
        b1 += h;
    if (t1 == b1) // null rect
        return false;

    qreal t2 = r.yp;
    qreal b2 = r.yp;
    if (r.h < 0)
        t2 += r.h;
    else
        b2 += r.h;
    if (t2 == b2) // null rect
        return false;

    if (t1 >= b2 || t2 >= b1)
        return false;

    return true;
}

/*!
    \fn QBox QBoxF::toBox() const

    Returns a QBox based on the values of this rectangle.  Note that the
    coordinates in the returned rectangle are rounded to the nearest integer.

    \sa QBoxF(), toAlignedRect(), QBox::toBoxF()
*/

/*!
    \fn QBox QBoxF::toAlignedRect() const

    Returns a QBox based on the values of this rectangle that is the
    smallest possible integer rectangle that completely contains this
    rectangle.

    \sa toBox()
*/

QBox QBoxF::toAlignedRect() const noexcept
{
    int xmin = int(qFloor(xp));
    int xmax = int(qCeil(xp + w));
    int ymin = int(qFloor(yp));
    int ymax = int(qCeil(yp + h));
    return QBox(xmin, ymin, xmax - xmin, ymax - ymin);
}

/*!
    \fn void QBoxF::moveCenter(const QPointF &position)

    Moves the rectangle, leaving the center point at the given \a
    position. The rectangle's size is unchanged.

    \sa center()
*/

/*!
    \fn bool QBoxF::operator==(const QBoxF &r1, const QBoxF &r2)

    Returns \c true if the rectangles \a r1 and \a r2 are \b approximately equal,
    otherwise returns \c false.

    \warning This function does not check for strict equality; instead,
    it uses a fuzzy comparison to compare the rectangles' coordinates.

    \sa qFuzzyCompare
*/

/*!
    \fn bool QBoxF::operator!=(const QBoxF &r1, const QBoxF &r2)

    Returns \c true if the rectangles \a r1 and \a r2 are sufficiently
    different, otherwise returns \c false.

    \warning This function does not check for strict inequality; instead,
    it uses a fuzzy comparison to compare the rectangles' coordinates.
*/

/*!
    \fn QBoxF operator+(const QBoxF &lhs, const QMarginsF &rhs)
    \relates QBoxF

    Returns the \a lhs rectangle grown by the \a rhs margins.
*/

/*!
    \fn QBoxF operator-(const QBoxF &lhs, const QMarginsF &rhs)
    \relates QBoxF

    Returns the \a lhs rectangle shrunk by the \a rhs margins.
*/

/*!
    \fn QBoxF operator+(const QMarginsF &lhs, const QBoxF &rhs)
    \relates QBoxF
    \overload

    Returns the \a lhs rectangle grown by the \a rhs margins.
*/

/*!
    \fn QBoxF QBoxF::marginsAdded(const QMarginsF &margins) const

    Returns a rectangle grown by the \a margins.

    \sa operator+=(), marginsRemoved(), operator-=()
*/

/*!
    \fn QBoxF QBoxF::marginsRemoved(const QMarginsF &margins) const

    Removes the \a margins from the rectangle, shrinking it.

    \sa marginsAdded(), operator+=(), operator-=()
*/

/*!
    \fn QBoxF QBoxF::operator+=(const QMarginsF &margins)

    Adds the \a margins to the rectangle, growing it.

    \sa marginsAdded(), marginsRemoved(), operator-=()
*/

/*!
    \fn QBoxF QBoxF::operator-=(const QMarginsF &margins)

    Returns a rectangle shrunk by the \a margins.

    \sa marginsRemoved(), operator+=(), marginsAdded()
*/

/*****************************************************************************
  QBoxF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QBoxF &rectangle)

    \relates QBoxF

    Writes the \a rectangle to the \a stream, and returns a reference to the
    stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QBoxF &r)
{
    s << double(r.x()) << double(r.y()) << double(r.width()) << double(r.height());
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QBoxF &rectangle)

    \relates QBoxF

    Reads a \a rectangle from the \a stream, and returns a reference to the
    stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QBoxF &r)
{
    double x, y, w, h;
    s >> x;
    s >> y;
    s >> w;
    s >> h;
    r.setRect(qreal(x), qreal(y), qreal(w), qreal(h));
    return s;
}

#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QBoxF &r)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QBoxF" << '(';
    QtDebugUtils::formatQRect(dbg, r);
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
