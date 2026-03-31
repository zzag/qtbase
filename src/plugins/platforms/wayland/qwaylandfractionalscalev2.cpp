// Copyright (C) 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandfractionalscalev2_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandFractionalScaleV2::QWaylandFractionalScaleV2(struct ::xx_fractional_scale_v2 *object)
    : QtWayland::xx_fractional_scale_v2(object)
{}


QWaylandFractionalScaleV2::~QWaylandFractionalScaleV2()
{
    destroy();
}

void QWaylandFractionalScaleV2::setClientToCompositorScale(qreal scale)
{
    if (mClientToCompositorScaleFactor != scale) {
        mClientToCompositorScaleFactor = scale;
        set_scale_factor(scale * (1UL << 24));
    }
}

void QWaylandFractionalScaleV2::xx_fractional_scale_v2_scale_factor(uint32_t scale_8_24)
{
    const qreal scale = scale_8_24 / qreal(1UL << 24);
    if (mCompositorToClientScaleFactor != scale) {
        mCompositorToClientScaleFactor = scale;
        Q_EMIT compositorToClientScaleFactorChanged();
    }
}

}

QT_END_NAMESPACE
