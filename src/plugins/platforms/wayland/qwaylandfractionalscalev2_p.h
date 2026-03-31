// Copyright (C) 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDFRACTIONALSCALE_V2_P_H
#define QWAYLANDFRACTIONALSCALE_V2_P_H

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

#include <QtWaylandClient/private/qwayland-xx-fractional-scale-v2.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QObject>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandFractionalScaleV2 : public QObject, public QtWayland::xx_fractional_scale_v2
{
    Q_OBJECT

public:
    explicit QWaylandFractionalScaleV2(struct ::xx_fractional_scale_v2 *object);
    ~QWaylandFractionalScaleV2() override;

    std::optional<qreal> compositorToClientScale() const { return mCompositorToClientScaleFactor; }
    std::optional<qreal> clientToCompositorScale() const { return mClientToCompositorScaleFactor; }

    void setClientToCompositorScale(qreal scale);

Q_SIGNALS:
    void compositorToClientScaleFactorChanged();

protected:
    void xx_fractional_scale_v2_scale_factor(uint32_t scale_8_24) override;

private:
    std::optional<qreal> mCompositorToClientScaleFactor;
    std::optional<qreal> mClientToCompositorScaleFactor;
};

}

QT_END_NAMESPACE

#endif
