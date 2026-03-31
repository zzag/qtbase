// Copyright (C) 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWAYLANDHIGHDPI_P_H
#define QWAYLANDHIGHDPI_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QRect>
#include <QtGui/QRegion>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

static inline QRect scaledAndRoundedRect(const QRectF &rect, qreal scale)
{
    return QRect(QPoint(std::round(rect.x() * scale),
                        std::round(rect.y() * scale)),
                 QPoint(std::round((rect.x() + rect.width()) * scale) - 1,
                        std::round((rect.y() + rect.height()) * scale) - 1));
}

static inline QRect scaledAndRoundedRect(const QRect &rect, qreal scale)
{
    if (scale == 1)
        return rect;

    return QRect(QPoint(std::round(rect.x() * scale),
                        std::round(rect.y() * scale)),
                 QPoint(std::round((rect.x() + rect.width()) * scale) - 1,
                        std::round((rect.y() + rect.height()) * scale) - 1));
}

static inline QRegion scaledAndRoundedRegion(const QRegion &region, qreal scale)
{
    if (scale == 1)
        return region;

    QRegion scaled;
    for (const QRect &rect : region)
        scaled += scaledAndRoundedRect(rect, scale);

    return scaled;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDHIGHDPI_P_H
