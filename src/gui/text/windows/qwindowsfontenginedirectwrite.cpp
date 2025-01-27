// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsfontenginedirectwrite_p.h"
#include "qwindowsfontdatabase_p.h"

#include <QtCore/QtEndian>
#include <QtCore/QVarLengthArray>
#include <QtCore/QFile>
#include <private/qstringiterator_p.h>
#include <QtCore/private/qsystemlibrary_p.h>
#include <QtCore/private/qwinregistry_p.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QtGui/qpainterpath.h>

#if QT_CONFIG(directwrite3)
#  include "qwindowsdirectwritefontdatabase_p.h"
#  include <dwrite_3.h>
#endif

#include <d2d1.h>

QT_BEGIN_NAMESPACE

// Clang does not consider __declspec(nothrow) as nothrow
QT_WARNING_DISABLE_CLANG("-Wmicrosoft-exception-spec")

// Convert from design units to logical pixels
#define DESIGN_TO_LOGICAL(DESIGN_UNIT_VALUE) \
    QFixed::fromReal((qreal(DESIGN_UNIT_VALUE) / qreal(m_unitsPerEm)) * fontDef.pixelSize)

namespace {

    class GeometrySink: public IDWriteGeometrySink
    {
        Q_DISABLE_COPY_MOVE(GeometrySink)
    public:
        GeometrySink(QPainterPath *path)
            : m_refCount(0), m_path(path)
        {
            Q_ASSERT(m_path != 0);
        }
        virtual ~GeometrySink() = default;

        IFACEMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT *beziers, UINT bezierCount) override;
        IFACEMETHOD_(void, AddLines)(const D2D1_POINT_2F *points, UINT pointCount) override;
        IFACEMETHOD_(void, BeginFigure)(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) override;
        IFACEMETHOD(Close)() override;
        IFACEMETHOD_(void, EndFigure)(D2D1_FIGURE_END figureEnd) override;
        IFACEMETHOD_(void, SetFillMode)(D2D1_FILL_MODE fillMode) override;
        IFACEMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT vertexFlags) override;

        IFACEMETHOD_(unsigned long, AddRef)() override;
        IFACEMETHOD_(unsigned long, Release)() override;
        IFACEMETHOD(QueryInterface)(IID const &riid, void **ppvObject) override;

    private:
        inline static QPointF fromD2D1_POINT_2F(const D2D1_POINT_2F &inp)
        {
            return QPointF(inp.x, inp.y);
        }

        unsigned long m_refCount;
        QPointF m_startPoint;
        QPainterPath *m_path;
    };

    void GeometrySink::AddBeziers(const D2D1_BEZIER_SEGMENT *beziers,
                                  UINT bezierCount) noexcept
    {
        for (uint i=0; i<bezierCount; ++i) {
            QPointF c1 = fromD2D1_POINT_2F(beziers[i].point1);
            QPointF c2 = fromD2D1_POINT_2F(beziers[i].point2);
            QPointF p2 = fromD2D1_POINT_2F(beziers[i].point3);

            m_path->cubicTo(c1, c2, p2);
        }
    }

    void GeometrySink::AddLines(const D2D1_POINT_2F *points, UINT pointsCount) noexcept
    {
        for (uint i=0; i<pointsCount; ++i)
            m_path->lineTo(fromD2D1_POINT_2F(points[i]));
    }

    void GeometrySink::BeginFigure(D2D1_POINT_2F startPoint,
                                   D2D1_FIGURE_BEGIN /*figureBegin*/) noexcept
    {
        m_startPoint = fromD2D1_POINT_2F(startPoint);
        m_path->moveTo(m_startPoint);
    }

    IFACEMETHODIMP GeometrySink::Close() noexcept
    {
        return E_NOTIMPL;
    }

    void GeometrySink::EndFigure(D2D1_FIGURE_END figureEnd) noexcept
    {
        if (figureEnd == D2D1_FIGURE_END_CLOSED)
            m_path->closeSubpath();
    }

    void GeometrySink::SetFillMode(D2D1_FILL_MODE fillMode) noexcept
    {
        m_path->setFillRule(fillMode == D2D1_FILL_MODE_ALTERNATE
                            ? Qt::OddEvenFill
                            : Qt::WindingFill);
    }

    void GeometrySink::SetSegmentFlags(D2D1_PATH_SEGMENT /*vertexFlags*/) noexcept
    {
        /* Not implemented */
    }

    IFACEMETHODIMP_(unsigned long) GeometrySink::AddRef() noexcept
    {
        return InterlockedIncrement(&m_refCount);
    }

    IFACEMETHODIMP_(unsigned long) GeometrySink::Release() noexcept
    {
        unsigned long newCount = InterlockedDecrement(&m_refCount);
        if (newCount == 0)
        {
            delete this;
            return 0;
        }

        return newCount;
    }

    IFACEMETHODIMP GeometrySink::QueryInterface(IID const &riid, void **ppvObject) noexcept
    {
        if (__uuidof(IDWriteGeometrySink) == riid) {
            *ppvObject = this;
        } else if (__uuidof(IUnknown) == riid) {
            *ppvObject = this;
        } else {
            *ppvObject = NULL;
            return E_FAIL;
        }

        AddRef();
        return S_OK;
    }

}

static DWRITE_MEASURING_MODE renderModeToMeasureMode(DWRITE_RENDERING_MODE renderMode)
{
    switch (renderMode) {
    case DWRITE_RENDERING_MODE_GDI_CLASSIC:
        return DWRITE_MEASURING_MODE_GDI_CLASSIC;
    case DWRITE_RENDERING_MODE_GDI_NATURAL:
        return DWRITE_MEASURING_MODE_GDI_NATURAL;
    default:
        return DWRITE_MEASURING_MODE_NATURAL;
    }
}

DWRITE_RENDERING_MODE QWindowsFontEngineDirectWrite::hintingPreferenceToRenderingMode(const QFontDef &fontDef) const
{
    if ((fontDef.styleStrategy & QFont::NoAntialias) && glyphFormat != QFontEngine::Format_ARGB)
        return DWRITE_RENDERING_MODE_ALIASED;

    QFont::HintingPreference hintingPreference = QFont::HintingPreference(fontDef.hintingPreference);
    if (!qFuzzyCompare(qApp->devicePixelRatio(), 1.0) && hintingPreference == QFont::PreferDefaultHinting) {
        // Microsoft documentation recommends using asymmetric rendering for small fonts
        // at pixel size 16 and less, and symmetric for larger fonts.
        hintingPreference = fontDef.pixelSize > 16.0
                ? QFont::PreferNoHinting
                : QFont::PreferVerticalHinting;
    }

    switch (hintingPreference) {
    case QFont::PreferNoHinting:
        return DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC;
    case QFont::PreferVerticalHinting:
        return DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL;
    default:
        return DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC;
    }
}

/*!
    \class QWindowsFontEngineDirectWrite
    \brief Windows font engine using Direct Write.
    \internal

    Font engine for subpixel positioned text on Windows Vista
    (with platform update) and later. If selected during
    configuration, the engine will be selected only when the hinting
    preference of a font is set to None or Vertical hinting, or
    when fontengine=directwrite is selected as platform option.
*/

QWindowsFontEngineDirectWrite::QWindowsFontEngineDirectWrite(IDWriteFontFace *directWriteFontFace,
                                                             qreal pixelSize,
                                                             const QSharedPointer<QWindowsFontEngineData> &d)
    : QFontEngine(DirectWrite)
    , m_fontEngineData(d)
    , m_directWriteFontFace(directWriteFontFace)
    , m_directWriteBitmapRenderTarget(0)
    , m_lineThickness(-1)
    , m_unitsPerEm(-1)
    , m_capHeight(-1)
    , m_xHeight(-1)
{
    qCDebug(lcQpaFonts) << __FUNCTION__ << pixelSize;

    Q_ASSERT(m_directWriteFontFace);

    m_fontEngineData->directWriteFactory->AddRef();
    m_directWriteFontFace->AddRef();

    IDWriteRenderingParams *renderingParams = nullptr;
    if (SUCCEEDED(m_fontEngineData->directWriteFactory->CreateRenderingParams(&renderingParams))) {
        m_pixelGeometry = renderingParams->GetPixelGeometry();
        renderingParams->Release();
    }

    fontDef.pixelSize = pixelSize;
    collectMetrics();
    cache_cost = m_xHeight.toInt() * m_xHeight.toInt() * 2000;
}

QWindowsFontEngineDirectWrite::~QWindowsFontEngineDirectWrite()
{
    qCDebug(lcQpaFonts) << __FUNCTION__;

    m_fontEngineData->directWriteFactory->Release();
    m_directWriteFontFace->Release();

    if (m_directWriteBitmapRenderTarget != 0)
        m_directWriteBitmapRenderTarget->Release();

    if (!m_uniqueFamilyName.isEmpty()) {
        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        static_cast<QWindowsFontDatabase *>(pfdb)->derefUniqueFont(m_uniqueFamilyName);
    }
}

#ifndef Q_CC_MINGW
typedef IDWriteLocalFontFileLoader QIdWriteLocalFontFileLoader;

static UUID uuidIdWriteLocalFontFileLoader()
{
    return __uuidof(IDWriteLocalFontFileLoader);
}
#else // !Q_CC_MINGW
DECLARE_INTERFACE_(QIdWriteLocalFontFileLoader, IDWriteFontFileLoader)
{
    STDMETHOD(GetFilePathLengthFromKey)(THIS_ void const *, UINT32, UINT32*) PURE;
    STDMETHOD(GetFilePathFromKey)(THIS_ void const *, UINT32, WCHAR *, UINT32) PURE;
    STDMETHOD(GetLastWriteTimeFromKey)(THIS_ void const *, UINT32, FILETIME *) PURE;
};

static UUID uuidIdWriteLocalFontFileLoader()
{
    static const UUID result = { 0xb2d9f3ec, 0xc9fe, 0x4a11, {0xa2, 0xec, 0xd8, 0x62, 0x8, 0xf7, 0xc0, 0xa2}};
    return result;
}
#endif // Q_CC_MINGW

QString QWindowsFontEngineDirectWrite::filenameFromFontFile(IDWriteFontFile *fontFile)
{
    IDWriteFontFileLoader *loader = nullptr;

    HRESULT hr = fontFile->GetLoader(&loader);
    if (FAILED(hr)) {
        qErrnoWarning("%s: GetLoader failed", __FUNCTION__);
        return QString();
    }

    QIdWriteLocalFontFileLoader *localLoader = nullptr;
    hr = loader->QueryInterface(uuidIdWriteLocalFontFileLoader(),
                                reinterpret_cast<void **>(&localLoader));

    const void *fontFileReferenceKey = nullptr;
    UINT32 fontFileReferenceKeySize = 0;
    if (SUCCEEDED(hr)) {
        hr = fontFile->GetReferenceKey(&fontFileReferenceKey,
                                       &fontFileReferenceKeySize);
        if (FAILED(hr))
            qErrnoWarning(hr, "%s: GetReferenceKey failed", __FUNCTION__);
    }

    UINT32 filePathLength = 0;
    if (SUCCEEDED(hr)) {
        hr = localLoader->GetFilePathLengthFromKey(fontFileReferenceKey,
                                                   fontFileReferenceKeySize,
                                                   &filePathLength);
        if (FAILED(hr))
            qErrnoWarning(hr, "GetFilePathLength failed", __FUNCTION__);
    }

    QString ret;
    if (SUCCEEDED(hr) && filePathLength > 0) {
        QVarLengthArray<wchar_t> filePath(filePathLength + 1);

        hr = localLoader->GetFilePathFromKey(fontFileReferenceKey,
                                             fontFileReferenceKeySize,
                                             filePath.data(),
                                             filePathLength + 1);
        if (FAILED(hr))
            qErrnoWarning(hr, "%s: GetFilePathFromKey failed", __FUNCTION__);
        else
            ret = QString::fromWCharArray(filePath.data());
    }

    if (localLoader != nullptr)
        localLoader->Release();

    if (loader != nullptr)
        loader->Release();
    return ret;
}

HFONT QWindowsFontEngineDirectWrite::createHFONT() const
{
    if (m_fontEngineData == nullptr || m_directWriteFontFace == nullptr)
        return NULL;

    LOGFONT lf;
    HRESULT hr = m_fontEngineData->directWriteGdiInterop->ConvertFontFaceToLOGFONT(m_directWriteFontFace,
                                                                                   &lf);
    if (SUCCEEDED(hr)) {
        lf.lfHeight = -qRound(fontDef.pixelSize);
        return CreateFontIndirect(&lf);
    } else {
        return NULL;
    }
}

void QWindowsFontEngineDirectWrite::initializeHeightMetrics() const
{
    DWRITE_FONT_METRICS metrics;
    m_directWriteFontFace->GetMetrics(&metrics);

    m_ascent = DESIGN_TO_LOGICAL(metrics.ascent);
    m_descent = DESIGN_TO_LOGICAL(metrics.descent);
    m_leading = DESIGN_TO_LOGICAL(metrics.lineGap);

    QFontEngine::initializeHeightMetrics();
}

void QWindowsFontEngineDirectWrite::collectMetrics()
{
    DWRITE_FONT_METRICS metrics;

    m_directWriteFontFace->GetMetrics(&metrics);
    m_unitsPerEm = metrics.designUnitsPerEm;

    m_lineThickness = DESIGN_TO_LOGICAL(metrics.underlineThickness);
    m_capHeight = DESIGN_TO_LOGICAL(metrics.capHeight);
    m_xHeight = DESIGN_TO_LOGICAL(metrics.xHeight);
    m_underlinePosition = DESIGN_TO_LOGICAL(metrics.underlinePosition);

    IDWriteFontFile *fontFile = nullptr;
    UINT32 numberOfFiles = 1;
    if (SUCCEEDED(m_directWriteFontFace->GetFiles(&numberOfFiles, &fontFile))) {
        m_faceId.filename = QFile::encodeName(filenameFromFontFile(fontFile));
        fontFile->Release();
    }

    QByteArray table = getSfntTable(QFont::Tag("hhea").value());
    const int advanceWidthMaxLocation = 10;
    if (table.size() >= advanceWidthMaxLocation + int(sizeof(quint16))) {
        quint16 advanceWidthMax = qFromBigEndian<quint16>(table.constData() + advanceWidthMaxLocation);
        m_maxAdvanceWidth = DESIGN_TO_LOGICAL(advanceWidthMax);
    }

    loadKerningPairs(emSquareSize() / QFixed::fromReal(fontDef.pixelSize));

#if QT_CONFIG(directwrite3)
    IDWriteFontFace5 *face5;
    if (SUCCEEDED(m_directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace5),
                                       reinterpret_cast<void **>(&face5)))) {

        IDWriteFontResource *fontResource;
        if (SUCCEEDED(face5->GetFontResource(&fontResource))) {
            const UINT32 fontAxisCount = fontResource->GetFontAxisCount();
            if (fontAxisCount > 0) {
                QVarLengthArray<DWRITE_FONT_AXIS_VALUE, 8> axisValues(fontAxisCount);
                HRESULT hres = fontResource->GetDefaultFontAxisValues(axisValues.data(), fontAxisCount);

                QVarLengthArray<DWRITE_FONT_AXIS_RANGE, 8> axisRanges(fontAxisCount);
                if (SUCCEEDED(hres))
                    hres = fontResource->GetFontAxisRanges(axisRanges.data(), fontAxisCount);

                if (SUCCEEDED(hres)) {
                    for (UINT32 i = 0; i < fontAxisCount; ++i) {
                        const DWRITE_FONT_AXIS_VALUE &value = axisValues.at(i);
                        const DWRITE_FONT_AXIS_RANGE &range = axisRanges.at(i);

                        if (range.minValue < range.maxValue) {
                            QFontVariableAxis axis;
                            if (auto maybeTag = QFont::Tag::fromValue(qToBigEndian<UINT32>(value.axisTag))) {
                                axis.setTag(*maybeTag);
                            } else {
                                qWarning() << "QWindowsFontEngineDirectWrite::collectMetrics: Invalid tag" << value.axisTag;
                            }

                            axis.setDefaultValue(value.value);
                            axis.setMaximumValue(range.maxValue);
                            axis.setMinimumValue(range.minValue);

                            IDWriteLocalizedStrings *names;
                            if (SUCCEEDED(fontResource->GetAxisNames(i, &names))) {
                                wchar_t defaultLocale[LOCALE_NAME_MAX_LENGTH];
                                bool hasDefaultLocale = GetUserDefaultLocaleName(defaultLocale, LOCALE_NAME_MAX_LENGTH) != 0;

                                QString name = hasDefaultLocale
                                                   ? QWindowsDirectWriteFontDatabase::localeString(names, defaultLocale)
                                                   : QString();
                                if (name.isEmpty()) {
                                    wchar_t englishLocale[] = L"en-us";
                                    name = QWindowsDirectWriteFontDatabase::localeString(names, englishLocale);
                                }

                                axis.setName(name);
                                names->Release();
                            }

                            m_variableAxes.append(axis);
                        }
                    }
                }
            }
            fontResource->Release();
        }
        face5->Release();
    }
#endif
}

QFixed QWindowsFontEngineDirectWrite::underlinePosition() const
{
    if (m_underlinePosition > 0)
        return m_underlinePosition;
    else
        return QFontEngine::underlinePosition();
}

QFixed QWindowsFontEngineDirectWrite::lineThickness() const
{
    if (m_lineThickness > 0)
        return m_lineThickness;
    else
        return QFontEngine::lineThickness();
}

bool QWindowsFontEngineDirectWrite::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    bool ret = false;

    const void *tableData = 0;
    UINT32 tableSize;
    void *tableContext = 0;
    BOOL exists;
    HRESULT hr = m_directWriteFontFace->TryGetFontTable(qbswap<quint32>(tag),
                                                        &tableData, &tableSize,
                                                        &tableContext, &exists);
    if (SUCCEEDED(hr)) {
        if (exists) {
            ret = true;
            if (buffer && *length >= tableSize)
                memcpy(buffer, tableData, tableSize);
            *length = tableSize;
            Q_ASSERT(int(*length) > 0);
        }
        m_directWriteFontFace->ReleaseFontTable(tableContext);
    } else {
        qErrnoWarning("%s: TryGetFontTable failed", __FUNCTION__);
    }

    return ret;
}

QFixed QWindowsFontEngineDirectWrite::emSquareSize() const
{
    if (m_unitsPerEm > 0)
        return m_unitsPerEm;
    else
        return QFontEngine::emSquareSize();
}

glyph_t QWindowsFontEngineDirectWrite::glyphIndex(uint ucs4) const
{
    UINT16 glyphIndex;

    HRESULT hr = m_directWriteFontFace->GetGlyphIndicesW(&ucs4, 1, &glyphIndex);
    if (FAILED(hr)) {
        qErrnoWarning("%s: glyphIndex failed", __FUNCTION__);
        glyphIndex = 0;
    }

    return glyphIndex;
}

int QWindowsFontEngineDirectWrite::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs,
                                                int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return -1;
    }

    QVarLengthArray<UINT32> codePoints(len);
    int actualLength = 0;
    QStringIterator it(str, str + len);
    while (it.hasNext())
        codePoints[actualLength++] = it.next();

    QVarLengthArray<UINT16> glyphIndices(actualLength);
    HRESULT hr = m_directWriteFontFace->GetGlyphIndicesW(codePoints.data(), actualLength,
                                                         glyphIndices.data());
    if (FAILED(hr)) {
        qErrnoWarning("%s: GetGlyphIndicesW failed", __FUNCTION__);
        return -1;
    }

    int mappedGlyphs = 0;
    for (int i = 0; i < actualLength; ++i) {
        glyphs->glyphs[i] = glyphIndices.at(i);
        if (glyphs->glyphs[i] != 0 || isIgnorableChar(codePoints.at(i)))
            mappedGlyphs++;
    }

    *nglyphs = actualLength;
    glyphs->numGlyphs = actualLength;

    if (!(flags & GlyphIndicesOnly))
        recalcAdvances(glyphs, {});

    return mappedGlyphs;
}

QFontEngine::FaceId QWindowsFontEngineDirectWrite::faceId() const
{
    return m_faceId;
}

void QWindowsFontEngineDirectWrite::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags shaperFlags) const
{
    QVarLengthArray<UINT16> glyphIndices(glyphs->numGlyphs);

    // ### Caching?
    for(int i=0; i<glyphs->numGlyphs; i++)
        glyphIndices[i] = UINT16(glyphs->glyphs[i]);

    QVarLengthArray<DWRITE_GLYPH_METRICS> glyphMetrics(glyphIndices.size());

    HRESULT hr;
    DWRITE_RENDERING_MODE renderMode = hintingPreferenceToRenderingMode(fontDef);
    bool needsDesignMetrics = shaperFlags & QFontEngine::DesignMetrics;
    if (!needsDesignMetrics && (renderMode == DWRITE_RENDERING_MODE_GDI_CLASSIC
                             || renderMode == DWRITE_RENDERING_MODE_GDI_NATURAL
                             || renderMode == DWRITE_RENDERING_MODE_ALIASED)) {
        hr = m_directWriteFontFace->GetGdiCompatibleGlyphMetrics(float(fontDef.pixelSize),
                                                                 1.0f,
                                                                 NULL,
                                                                 renderMode == DWRITE_RENDERING_MODE_GDI_NATURAL,
                                                                 glyphIndices.data(),
                                                                 glyphIndices.size(),
                                                                 glyphMetrics.data());
    } else {
        hr = m_directWriteFontFace->GetDesignGlyphMetrics(glyphIndices.data(),
                                                          glyphIndices.size(),
                                                          glyphMetrics.data());
    }
    if (SUCCEEDED(hr)) {
        qreal stretch = fontDef.stretch != QFont::AnyStretch ? fontDef.stretch / 100.0 : 1.0;
        for (int i = 0; i < glyphs->numGlyphs; ++i)
            glyphs->advances[i] = DESIGN_TO_LOGICAL(glyphMetrics[i].advanceWidth * stretch);
    } else {
        qErrnoWarning("%s: GetDesignGlyphMetrics failed", __FUNCTION__);
    }
}

void QWindowsFontEngineDirectWrite::getUnscaledGlyph(glyph_t glyph,
                                                     QPainterPath *path,
                                                     glyph_metrics_t *metric)
{
    float advance = 0.0f;
    UINT16 g = glyph;
    DWRITE_GLYPH_OFFSET offset;
    offset.advanceOffset = 0;
    offset.ascenderOffset = 0;
    GeometrySink geometrySink(path);
    HRESULT hr = m_directWriteFontFace->GetGlyphRunOutline(m_unitsPerEm,
                                                           &g,
                                                           &advance,
                                                           &offset,
                                                           1,
                                                           false,
                                                           false,
                                                           &geometrySink);
    if (FAILED(hr)) {
        qErrnoWarning("%s: GetGlyphRunOutline failed", __FUNCTION__);
        return;
    }

    DWRITE_GLYPH_METRICS glyphMetrics;
    hr = m_directWriteFontFace->GetDesignGlyphMetrics(&g, 1, &glyphMetrics);
    if (FAILED(hr)) {
        qErrnoWarning("%s: GetDesignGlyphMetrics failed", __FUNCTION__);
        return;
    }

    QFixed advanceWidth = QFixed(int(glyphMetrics.advanceWidth));
    QFixed leftSideBearing = QFixed(glyphMetrics.leftSideBearing);
    QFixed rightSideBearing = QFixed(glyphMetrics.rightSideBearing);
    QFixed advanceHeight = QFixed(int(glyphMetrics.advanceHeight));
    QFixed verticalOriginY = QFixed(glyphMetrics.verticalOriginY);
    QFixed topSideBearing = QFixed(glyphMetrics.topSideBearing);
    QFixed bottomSideBearing = QFixed(glyphMetrics.bottomSideBearing);
    QFixed width = advanceWidth - leftSideBearing - rightSideBearing;
    QFixed height = advanceHeight - topSideBearing - bottomSideBearing;
    *metric = glyph_metrics_t(leftSideBearing,
                              -verticalOriginY + topSideBearing,
                              width,
                              height,
                              advanceWidth,
                              0);
}

void QWindowsFontEngineDirectWrite::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nglyphs,
                                             QPainterPath *path, QTextItem::RenderFlags flags)
{
    Q_UNUSED(flags);
    QVarLengthArray<UINT16> glyphIndices(nglyphs);
    QVarLengthArray<DWRITE_GLYPH_OFFSET> glyphOffsets(nglyphs);
    QVarLengthArray<FLOAT> glyphAdvances(nglyphs);

    for (int i=0; i<nglyphs; ++i) {
        glyphIndices[i] = glyphs[i];
        glyphOffsets[i].advanceOffset  = positions[i].x.toReal();
        glyphOffsets[i].ascenderOffset = -positions[i].y.toReal();
        glyphAdvances[i] = 0.0;
    }

    GeometrySink geometrySink(path);
    HRESULT hr = m_directWriteFontFace->GetGlyphRunOutline(
                fontDef.pixelSize,
                glyphIndices.data(),
                glyphAdvances.data(),
                glyphOffsets.data(),
                nglyphs,
                false,
                false,
                &geometrySink
                );

    if (FAILED(hr))
        qErrnoWarning("%s: GetGlyphRunOutline failed", __FUNCTION__);
}

glyph_metrics_t QWindowsFontEngineDirectWrite::boundingBox(const QGlyphLayout &glyphs)
{
    if (glyphs.numGlyphs == 0)
        return glyph_metrics_t();
    QFixed w = 0;
    for (int i = 0; i < glyphs.numGlyphs; ++i)
        w += glyphs.effectiveAdvance(i);

    const QFixed leftBearing = firstLeftBearing(glyphs);
    return glyph_metrics_t(leftBearing, -ascent(), w - leftBearing - lastRightBearing(glyphs),
            ascent() + descent(), w, 0);
}

glyph_metrics_t QWindowsFontEngineDirectWrite::boundingBox(glyph_t g)
{
    UINT16 glyphIndex = g;

    DWRITE_GLYPH_METRICS glyphMetrics;
    HRESULT hr = m_directWriteFontFace->GetDesignGlyphMetrics(&glyphIndex, 1, &glyphMetrics);
    if (SUCCEEDED(hr)) {
        QFixed advanceWidth = DESIGN_TO_LOGICAL(glyphMetrics.advanceWidth);
        QFixed leftSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.leftSideBearing);
        QFixed rightSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.rightSideBearing);
        QFixed advanceHeight = DESIGN_TO_LOGICAL(glyphMetrics.advanceHeight);
        QFixed verticalOriginY = DESIGN_TO_LOGICAL(glyphMetrics.verticalOriginY);
        QFixed topSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.topSideBearing);
        QFixed bottomSideBearing = DESIGN_TO_LOGICAL(glyphMetrics.bottomSideBearing);
        QFixed width = advanceWidth - leftSideBearing - rightSideBearing;
        QFixed height = advanceHeight - topSideBearing - bottomSideBearing;
        return glyph_metrics_t(leftSideBearing,
                               -verticalOriginY + topSideBearing,
                               width,
                               height,
                               advanceWidth,
                               0);
    } else {
        qErrnoWarning("%s: GetDesignGlyphMetrics failed", __FUNCTION__);
    }

    return glyph_metrics_t();
}

QFixed QWindowsFontEngineDirectWrite::capHeight() const
{
    if (m_capHeight <= 0)
        return calculatedCapHeight();

    return m_capHeight;
}

QFixed QWindowsFontEngineDirectWrite::xHeight() const
{
    return m_xHeight;
}

qreal QWindowsFontEngineDirectWrite::maxCharWidth() const
{
    return m_maxAdvanceWidth.toReal();
}

QImage QWindowsFontEngineDirectWrite::alphaMapForGlyph(glyph_t glyph,
                                                       const QFixedPoint &subPixelPosition,
                                                       const QTransform &t)
{
    QImage im = imageForGlyph(glyph, subPixelPosition, glyphMargin(Format_A8), t);

    if (!im.isNull()) {
        QImage alphaMap(im.width(), im.height(), QImage::Format_Alpha8);

        for (int y=0; y<im.height(); ++y) {
            const uint *src = reinterpret_cast<const uint *>(im.constScanLine(y));
            uchar *dst = alphaMap.scanLine(y);
            for (int x=0; x<im.width(); ++x) {
                *dst = 255 - (m_fontEngineData->pow_gamma[qGray(0xffffffff - *src)] * 255. / 2047.);
                ++dst;
                ++src;
            }
        }

        return alphaMap;
    } else {
        return QFontEngine::alphaMapForGlyph(glyph, t);
    }
}

QImage QWindowsFontEngineDirectWrite::alphaMapForGlyph(glyph_t glyph,
                                                       const QFixedPoint &subPixelPosition)
{
    return alphaMapForGlyph(glyph, subPixelPosition, QTransform());
}

bool QWindowsFontEngineDirectWrite::supportsHorizontalSubPixelPositions() const
{
    DWRITE_RENDERING_MODE renderMode = hintingPreferenceToRenderingMode(fontDef);
    return  (!isColorFont()
            && renderMode != DWRITE_RENDERING_MODE_GDI_CLASSIC
            && renderMode != DWRITE_RENDERING_MODE_GDI_NATURAL
            && renderMode != DWRITE_RENDERING_MODE_ALIASED);
}

QFontEngine::Properties QWindowsFontEngineDirectWrite::properties() const
{
    IDWriteFontFace2 *directWriteFontFace2;
    if (SUCCEEDED(m_directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace2),
                                                        reinterpret_cast<void **>(&directWriteFontFace2)))) {
        DWRITE_FONT_METRICS1 metrics;
        directWriteFontFace2->GetMetrics(&metrics);

        Properties p = QFontEngine::properties();
        p.emSquare = metrics.designUnitsPerEm;
        p.boundingBox = QRectF(metrics.glyphBoxLeft,
                               -metrics.glyphBoxTop,
                               metrics.glyphBoxRight - metrics.glyphBoxLeft,
                               metrics.glyphBoxTop - metrics.glyphBoxBottom);
        p.ascent = metrics.ascent;
        p.descent = metrics.descent;
        p.leading = metrics.lineGap;
        p.capHeight = metrics.capHeight;
        p.lineWidth = metrics.underlineThickness;

        directWriteFontFace2->Release();
        return p;
    } else {
        return QFontEngine::properties();
    }
}

bool QWindowsFontEngineDirectWrite::renderColr0GlyphRun(QImage *image,
                                                        const DWRITE_COLOR_GLYPH_RUN *colorGlyphRun,
                                                        const DWRITE_MATRIX &transform,
                                                        DWRITE_RENDERING_MODE renderMode,
                                                        DWRITE_MEASURING_MODE measureMode,
                                                        DWRITE_GRID_FIT_MODE gridFitMode,
                                                        QColor color,
                                                        QRect boundingRect) const
{
    ComPtr<IDWriteFactory2> factory2;
    HRESULT hr = m_fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory2),
                                                                      &factory2);
    if (FAILED(hr))
        return false;

    ComPtr<IDWriteGlyphRunAnalysis> colorGlyphsAnalysis;
    hr = factory2->CreateGlyphRunAnalysis(
        &colorGlyphRun->glyphRun,
        &transform,
        renderMode,
        measureMode,
        gridFitMode,
        DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE,
        0.0, 0.0,
        &colorGlyphsAnalysis
        );

    if (FAILED(hr)) {
        qErrnoWarning(hr, "%s: CreateGlyphRunAnalysis failed for color run", __FUNCTION__);
        return false;
    }

    float r, g, b, a;
    if (colorGlyphRun->paletteIndex == 0xFFFF) {
        r = float(color.redF());
        g = float(color.greenF());
        b = float(color.blueF());
        a = 1.0;
    } else {
        r = qBound(0.0f, colorGlyphRun->runColor.r, 1.0f);
        g = qBound(0.0f, colorGlyphRun->runColor.g, 1.0f);
        b = qBound(0.0f, colorGlyphRun->runColor.b, 1.0f);
        a = qBound(0.0f, colorGlyphRun->runColor.a, 1.0f);
    }

    if (!qFuzzyIsNull(a))
        renderGlyphRun(image, r, g, b, a, colorGlyphsAnalysis.Get(), boundingRect, renderMode);

    return true;
}

QImage QWindowsFontEngineDirectWrite::renderColorGlyph(DWRITE_GLYPH_RUN *glyphRun,
                                                       const DWRITE_MATRIX &transform,
                                                       DWRITE_RENDERING_MODE renderMode,
                                                       DWRITE_MEASURING_MODE measureMode,
                                                       DWRITE_GRID_FIT_MODE gridFitMode,
                                                       QColor color,
                                                       QRect boundingRect) const
{
    QImage ret;

#if QT_CONFIG(directwrite3)
    ComPtr<IDWriteFactory4> factory4;
    HRESULT hr = m_fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory4),
                                                                      &factory4);
    if (SUCCEEDED(hr)) {
        const DWRITE_GLYPH_IMAGE_FORMATS supportedBitmapFormats =
            DWRITE_GLYPH_IMAGE_FORMATS(DWRITE_GLYPH_IMAGE_FORMATS_PNG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_JPEG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TIFF);

        const DWRITE_GLYPH_IMAGE_FORMATS glyphFormats =
            DWRITE_GLYPH_IMAGE_FORMATS(DWRITE_GLYPH_IMAGE_FORMATS_COLR
                                        | supportedBitmapFormats
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE
                                        | DWRITE_GLYPH_IMAGE_FORMATS_CFF);

        ComPtr<IDWriteColorGlyphRunEnumerator1> enumerator;
        hr = factory4->TranslateColorGlyphRun(D2D1::Point2F(0.0f, 0.0f),
                                              glyphRun,
                                              NULL,
                                              glyphFormats,
                                              measureMode,
                                              NULL,
                                              0,
                                              &enumerator);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "%s: TranslateGlyphRun failed", __FUNCTION__);
            return ret;
        }

        BOOL ok = true;
        while (SUCCEEDED(hr) && ok) {
            hr = enumerator->MoveNext(&ok);
            if (!ok)
                break;

            const DWRITE_COLOR_GLYPH_RUN1 *colorGlyphRun = nullptr;
            hr = enumerator->GetCurrentRun(&colorGlyphRun);
            if (FAILED(hr)) {
                qErrnoWarning(hr, "%s: IDWriteColorGlyphRunEnumerator::GetCurrentRun failed", __FUNCTION__);
                return QImage{};
            }

            if (colorGlyphRun->glyphImageFormat == DWRITE_GLYPH_IMAGE_FORMATS_NONE) {
                break;
            } else if (colorGlyphRun->glyphImageFormat & DWRITE_GLYPH_IMAGE_FORMATS_COLR) {
                if (ret.isNull()) {
                    ret = QImage(boundingRect.width() - 1,
                                 boundingRect.height() - 1,
                                 QImage::Format_ARGB32_Premultiplied);
                    ret.fill(0);
                }

                if (!renderColr0GlyphRun(&ret,
                                         reinterpret_cast<const DWRITE_COLOR_GLYPH_RUN *>(colorGlyphRun), // Broken inheritance in MinGW
                                         transform,
                                         renderMode,
                                         measureMode,
                                         gridFitMode,
                                         color,
                                         boundingRect)) {
                    return QImage{};
                }
            } else if (colorGlyphRun->glyphImageFormat & supportedBitmapFormats) {
                ComPtr<IDWriteFontFace4> face4;
                if (SUCCEEDED(m_directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace4),
                                                                    &face4))) {
                    DWRITE_GLYPH_IMAGE_DATA data;
                    void *ctx;
                    Q_ASSERT(glyphRun->glyphCount == 1);
                    HRESULT hr = face4->GetGlyphImageData(glyphRun->glyphIndices[0],
                                                          fontDef.pixelSize,
                                                          DWRITE_GLYPH_IMAGE_FORMATS(colorGlyphRun->glyphImageFormat & supportedBitmapFormats),
                                                          &data,
                                                          &ctx);
                    if (FAILED(hr)) {
                        qErrnoWarning("%s: GetGlyphImageData failed", __FUNCTION__);
                        return QImage{};
                    }

                    const char *format;
                    switch (colorGlyphRun->glyphImageFormat) {
                    case DWRITE_GLYPH_IMAGE_FORMATS_JPEG:
                        format = "JPEG";
                        break;
                    case DWRITE_GLYPH_IMAGE_FORMATS_TIFF:
                        format = "TIFF";
                        break;
                    default:
                        format = "PNG";
                        break;
                    };

                    ret = QImage::fromData(reinterpret_cast<const uchar *>(data.imageData),
                                           data.imageDataSize,
                                           format);

                    QTransform matrix(transform.m11, transform.m12,
                                      transform.m21, transform.m22,
                                      transform.dx, transform.dy);

                    const qreal scale = fontDef.pixelSize / data.pixelsPerEm;
                    matrix.scale(scale, scale);

                    if (!matrix.isIdentity())
                        ret = ret.transformed(matrix, Qt::SmoothTransformation);

                    face4->ReleaseGlyphImageData(ctx);
                }

            } else {
                qCDebug(lcQpaFonts) << "Found glyph run with unsupported format"
                                    << colorGlyphRun->glyphImageFormat;
            }
        }
    }
#endif // QT_CONFIG(directwrite3)

    if (ret.isNull()) {
        ComPtr<IDWriteFactory2> factory2;
        HRESULT hr = m_fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory2),
                                                                          &factory2);
        if (FAILED(hr))
            return ret;

        ComPtr<IDWriteColorGlyphRunEnumerator> enumerator;
        hr = factory2->TranslateColorGlyphRun(0.0f,
                                              0.0f,
                                              glyphRun,
                                              NULL,
                                              measureMode,
                                              NULL,
                                              0,
                                              &enumerator);
        if (SUCCEEDED(hr)) {
            ret = QImage(boundingRect.width() - 1,
                         boundingRect.height() - 1,
                         QImage::Format_ARGB32_Premultiplied);
            ret.fill(0);

            BOOL ok = true;
            while (SUCCEEDED(hr) && ok) {
                hr = enumerator->MoveNext(&ok);
                if (FAILED(hr)) {
                    qErrnoWarning(hr, "%s: IDWriteColorGlyphRunEnumerator::MoveNext failed", __FUNCTION__);
                    return QImage{};
                }

                if (ok) {
                    const DWRITE_COLOR_GLYPH_RUN *colorGlyphRun = nullptr;
                    hr = enumerator->GetCurrentRun(&colorGlyphRun);
                    if (FAILED(hr)) { // No colored runs, only outline
                        qErrnoWarning(hr, "%s: IDWriteColorGlyphRunEnumerator::GetCurrentRun failed", __FUNCTION__);
                        return QImage{};
                    }

                    if (!renderColr0GlyphRun(&ret,
                                             colorGlyphRun,
                                             transform,
                                             renderMode,
                                             measureMode,
                                             gridFitMode,
                                             color,
                                             boundingRect)) {
                        return QImage{};
                    }
                }
            }
        }
    }

    return ret;
}

QImage QWindowsFontEngineDirectWrite::imageForGlyph(glyph_t t,
                                                    const QFixedPoint &subPixelPosition,
                                                    int margin,
                                                    const QTransform &originalTransform,
                                                    const QColor &color)
{
    UINT16 glyphIndex = t;
    FLOAT glyphAdvance = 0;

    DWRITE_GLYPH_OFFSET glyphOffset;
    glyphOffset.advanceOffset = 0;
    glyphOffset.ascenderOffset = 0;

    DWRITE_GLYPH_RUN glyphRun;
    glyphRun.fontFace = m_directWriteFontFace;
    glyphRun.fontEmSize = fontDef.pixelSize;
    glyphRun.glyphCount = 1;
    glyphRun.glyphIndices = &glyphIndex;
    glyphRun.glyphAdvances = &glyphAdvance;
    glyphRun.isSideways = false;
    glyphRun.bidiLevel = 0;
    glyphRun.glyphOffsets = &glyphOffset;

    QTransform xform = originalTransform;
    if (fontDef.stretch != 100 && fontDef.stretch != QFont::AnyStretch)
        xform.scale(fontDef.stretch / 100.0, 1.0);

    DWRITE_MATRIX transform;
    transform.dx = subPixelPosition.x.toReal();
    transform.dy = 0;
    transform.m11 = xform.m11();
    transform.m12 = xform.m12();
    transform.m21 = xform.m21();
    transform.m22 = xform.m22();

    DWRITE_RENDERING_MODE renderMode = hintingPreferenceToRenderingMode(fontDef);
    DWRITE_MEASURING_MODE measureMode = renderModeToMeasureMode(renderMode);
    DWRITE_GRID_FIT_MODE gridFitMode = fontDef.hintingPreference == QFont::PreferNoHinting
            ? DWRITE_GRID_FIT_MODE_DISABLED
            : DWRITE_GRID_FIT_MODE_DEFAULT;

    ComPtr<IDWriteFactory2> factory2;
    HRESULT hr = m_fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory2),
                                                                      &factory2);
    ComPtr<IDWriteGlyphRunAnalysis> glyphAnalysis;
    if (!SUCCEEDED(hr)) {
        qErrnoWarning(hr, "%s: Failed to query IDWriteFactory2 interface.", __FUNCTION__);
        hr = m_fontEngineData->directWriteFactory->CreateGlyphRunAnalysis(
                    &glyphRun,
                    1.0f,
                    &transform,
                    renderMode,
                    measureMode,
                    0.0, 0.0,
                    &glyphAnalysis
                    );
    } else {
        hr = factory2->CreateGlyphRunAnalysis(
                    &glyphRun,
                    &transform,
                    renderMode,
                    measureMode,
                    gridFitMode,
                    DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE,
                    0.0, 0.0,
                    &glyphAnalysis
                    );
    }

    if (SUCCEEDED(hr)) {
        QRect rect = alphaTextureBounds(t, transform);
        if (rect.isEmpty())
            rect = colorBitmapBounds(t, transform);

        if (rect.isEmpty()) {
            qCDebug(lcQpaFonts) << __FUNCTION__ << "Cannot get alpha texture bounds. Falling back to slower rendering path.";
            return QImage();
        }

        QRect boundingRect = QRect(QPoint(rect.left() - margin,
                                          rect.top() - margin),
                                   QPoint(rect.right() + margin,
                                          rect.bottom() + margin));

        QImage image;
        if (glyphFormat == QFontEngine::Format_ARGB) {
            image = renderColorGlyph(&glyphRun,
                                     transform,
                                     renderMode,
                                     measureMode,
                                     gridFitMode,
                                     color,
                                     boundingRect);
        }

        // Not a color glyph, fall back to regular glyph rendering
        if (image.isNull()) {
            // -1 due to Qt's off-by-one definition of a QRect
            image = QImage(boundingRect.width() - 1,
                           boundingRect.height() - 1,
                           QImage::Format_RGB32);
            image.fill(0xffffffff);

            float r, g, b, a;
            if (glyphFormat == QFontEngine::Format_ARGB) {
                r = float(color.redF());
                g = float(color.greenF());
                b = float(color.blueF());
                a = float(color.alphaF());
            } else {
                r = g = b = a = 0.0;
            }

            renderGlyphRun(&image,
                           r,
                           g,
                           b,
                           a,
                           glyphAnalysis.Get(),
                           boundingRect,
                           renderMode);
        }

        return image;
    } else {
        qErrnoWarning(hr, "%s: CreateGlyphRunAnalysis failed", __FUNCTION__);
        return QImage();
    }
}

void QWindowsFontEngineDirectWrite::renderGlyphRun(QImage *destination,
                                                   float r,
                                                   float g,
                                                   float b,
                                                   float a,
                                                   IDWriteGlyphRunAnalysis *glyphAnalysis,
                                                   const QRect &boundingRect,
                                                   DWRITE_RENDERING_MODE renderMode) const
{
    const int width = destination->width();
    const int height = destination->height();

    r *= 255.0;
    g *= 255.0;
    b *= 255.0;

    const int size = width * height * 3;
    if (size > 0) {
        RECT rect;
        rect.left = boundingRect.left();
        rect.top = boundingRect.top();
        rect.right = boundingRect.right();
        rect.bottom = boundingRect.bottom();

        QVarLengthArray<BYTE, 1024> alphaValueArray(size);
        BYTE *alphaValues = alphaValueArray.data();
        memset(alphaValues, 0, size);

        HRESULT hr = glyphAnalysis->CreateAlphaTexture(renderMode == DWRITE_RENDERING_MODE_ALIASED
                                                               ? DWRITE_TEXTURE_ALIASED_1x1
                                                               : DWRITE_TEXTURE_CLEARTYPE_3x1,
                                                       &rect,
                                                       alphaValues,
                                                       size);
        if (SUCCEEDED(hr)) {
            if (destination->hasAlphaChannel()) { // Color glyphs
                for (int y = 0; y < height; ++y) {
                    uint *dest = reinterpret_cast<uint *>(destination->scanLine(y));
                    BYTE *src = alphaValues + width * 3 * y;

                    for (int x = 0; x < width; ++x) {
                        float redAlpha = a * *src++ / 255.0;
                        float greenAlpha = a * *src++ / 255.0;
                        float blueAlpha = a * *src++ / 255.0;
                        float averageAlpha = (redAlpha + greenAlpha + blueAlpha) / 3.0;

                        if (m_pixelGeometry == DWRITE_PIXEL_GEOMETRY_BGR)
                            qSwap(redAlpha, blueAlpha);

                        QRgb currentRgb = dest[x];
                        dest[x] = qRgba(qRound(qRed(currentRgb) * (1.0 - averageAlpha) + averageAlpha * r),
                                        qRound(qGreen(currentRgb) * (1.0 - averageAlpha) + averageAlpha * g),
                                        qRound(qBlue(currentRgb) * (1.0 - averageAlpha) + averageAlpha * b),
                                        qRound(qAlpha(currentRgb) * (1.0 - averageAlpha) + averageAlpha * 255));
                    }
                }
            } else if (renderMode == DWRITE_RENDERING_MODE_ALIASED) {
                for (int y = 0; y < height; ++y) {
                    uint *dest = reinterpret_cast<uint *>(destination->scanLine(y));
                    BYTE *src = alphaValues + width * y;

                    for (int x = 0; x < width; ++x) {
                        int alpha = *(src++);
                        dest[x] = (alpha << 16) + (alpha << 8) + alpha;
                    }
                }
            } else {
                for (int y = 0; y < height; ++y) {
                    uint *dest = reinterpret_cast<uint *>(destination->scanLine(y));
                    BYTE *src = alphaValues + width * 3 * y;

                    for (int x = 0; x < width; ++x) {
                        BYTE redAlpha   = *(src + 0);
                        BYTE greenAlpha = *(src + 1);
                        BYTE blueAlpha  = *(src + 2);

                        if (m_pixelGeometry == DWRITE_PIXEL_GEOMETRY_BGR)
                            qSwap(redAlpha, blueAlpha);

                        dest[x] = qRgb(redAlpha, greenAlpha, blueAlpha);
                        src += 3;
                    }
                }
            }
        } else {
            qErrnoWarning("%s: CreateAlphaTexture failed", __FUNCTION__);
        }
    }
}

QImage QWindowsFontEngineDirectWrite::alphaRGBMapForGlyph(glyph_t t,
                                                          const QFixedPoint &subPixelPosition,
                                                          const QTransform &xform)
{
    QImage mask = imageForGlyph(t,
                                subPixelPosition,
                                glyphMargin(QFontEngine::Format_A32),
                                xform);

    if (mask.isNull()) {
        mask = QFontEngine::renderedPathForGlyph(t, Qt::white);
        if (!xform.isIdentity())
            mask = mask.transformed(xform);
    }

    return mask.depth() == 32
           ? mask
           : mask.convertToFormat(QImage::Format_RGB32);
}

QFontEngine *QWindowsFontEngineDirectWrite::cloneWithSize(qreal pixelSize) const
{
    QWindowsFontEngineDirectWrite *fontEngine = new QWindowsFontEngineDirectWrite(m_directWriteFontFace,
                                                                                  pixelSize,
                                                                                  m_fontEngineData);

    fontEngine->fontDef = fontDef;
    fontEngine->fontDef.pixelSize = pixelSize;
    if (!m_uniqueFamilyName.isEmpty()) {
        fontEngine->setUniqueFamilyName(m_uniqueFamilyName);
        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        static_cast<QWindowsFontDatabase *>(pfdb)->refUniqueFont(m_uniqueFamilyName);
    }

    return fontEngine;
}

Qt::HANDLE QWindowsFontEngineDirectWrite::handle() const
{
    return m_directWriteFontFace;
}

void QWindowsFontEngineDirectWrite::initFontInfo(const QFontDef &request,
                                                 int dpi)
{
    fontDef = request;

    if (fontDef.pointSize < 0)
        fontDef.pointSize = fontDef.pixelSize * 72. / dpi;
    else if (fontDef.pixelSize == -1)
        fontDef.pixelSize = qRound(fontDef.pointSize * dpi / 72.);

    m_faceId.variableAxes = request.variableAxisValues;

#if QT_CONFIG(directwrite3)
    IDWriteFontFace3 *face3 = nullptr;
    if (SUCCEEDED(m_directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace3),
                                                        reinterpret_cast<void **>(&face3)))) {
        IDWriteLocalizedStrings *names;
        if (SUCCEEDED(face3->GetFaceNames(&names))) {
            wchar_t englishLocale[] = L"en-us";
            fontDef.styleName = QWindowsDirectWriteFontDatabase::localeString(names, englishLocale);
            names->Release();
        }

        // Color font
        if (face3->IsColorFont())
            glyphFormat = QFontEngine::Format_ARGB;

        face3->Release();
    }
#endif
}

QString QWindowsFontEngineDirectWrite::fontNameSubstitute(const QString &familyName)
{
    const QString substitute =
        QWinRegistryKey(HKEY_LOCAL_MACHINE,
                        LR"(Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes)")
        .stringValue(familyName);
    return substitute.isEmpty() ? familyName : substitute;
}

QRect QWindowsFontEngineDirectWrite::alphaTextureBounds(glyph_t glyph,
                                                        const DWRITE_MATRIX &transform)
{
    UINT16 glyphIndex = glyph;
    FLOAT glyphAdvance = 0;

    DWRITE_GLYPH_OFFSET glyphOffset;
    glyphOffset.advanceOffset = 0;
    glyphOffset.ascenderOffset = 0;

    DWRITE_GLYPH_RUN glyphRun;
    glyphRun.fontFace = m_directWriteFontFace;
    glyphRun.fontEmSize = fontDef.pixelSize;
    glyphRun.glyphCount = 1;
    glyphRun.glyphIndices = &glyphIndex;
    glyphRun.glyphAdvances = &glyphAdvance;
    glyphRun.isSideways = false;
    glyphRun.bidiLevel = 0;
    glyphRun.glyphOffsets = &glyphOffset;

    DWRITE_RENDERING_MODE renderMode = hintingPreferenceToRenderingMode(fontDef);
    DWRITE_MEASURING_MODE measureMode = renderModeToMeasureMode(renderMode);
    DWRITE_GRID_FIT_MODE gridFitMode = fontDef.hintingPreference == QFont::PreferNoHinting
                                           ? DWRITE_GRID_FIT_MODE_DISABLED
                                           : DWRITE_GRID_FIT_MODE_DEFAULT;

    ComPtr<IDWriteFactory2> factory2 = nullptr;
    HRESULT hr = m_fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory2),
                                                                      &factory2);

    ComPtr<IDWriteGlyphRunAnalysis> glyphAnalysis;
    if (SUCCEEDED(hr)) {
        hr = factory2->CreateGlyphRunAnalysis(
            &glyphRun,
            &transform,
            renderMode,
            measureMode,
            gridFitMode,
            DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE,
            0.0, 0.0,
            &glyphAnalysis
            );
    } else {
        hr = m_fontEngineData->directWriteFactory->CreateGlyphRunAnalysis(
            &glyphRun,
            1.0f,
            &transform,
            renderMode,
            measureMode,
            0.0, 0.0,
            &glyphAnalysis
            );
    }

    if (SUCCEEDED(hr)) {
        RECT rect;
        hr = glyphAnalysis->GetAlphaTextureBounds(renderMode == DWRITE_RENDERING_MODE_ALIASED
                                                      ? DWRITE_TEXTURE_ALIASED_1x1
                                                      : DWRITE_TEXTURE_CLEARTYPE_3x1,
                                                  &rect);
        if (FAILED(hr) || rect.left == rect.right || rect.top == rect.bottom)
            return QRect{};

        return QRect(QPoint(rect.left, rect.top), QPoint(rect.right, rect.bottom));
    } else {
        return QRect{};
    }
}

QRect QWindowsFontEngineDirectWrite::colorBitmapBounds(glyph_t glyph, const DWRITE_MATRIX &transform)
{
#if QT_CONFIG(directwrite3)
    ComPtr<IDWriteFontFace4> face4;
    if (SUCCEEDED(m_directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace4),
                                                        &face4))) {
        DWRITE_GLYPH_IMAGE_FORMATS formats = face4->GetGlyphImageFormats();

        const DWRITE_GLYPH_IMAGE_FORMATS supportedBitmapFormats =
            DWRITE_GLYPH_IMAGE_FORMATS(DWRITE_GLYPH_IMAGE_FORMATS_PNG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_JPEG
                                        | DWRITE_GLYPH_IMAGE_FORMATS_TIFF);

        if (formats & supportedBitmapFormats) {
            DWRITE_GLYPH_IMAGE_DATA data;
            void *ctx;
            HRESULT hr = face4->GetGlyphImageData(glyph,
                                                  fontDef.pixelSize,
                                                  DWRITE_GLYPH_IMAGE_FORMATS(formats & supportedBitmapFormats),
                                                  &data,
                                                  &ctx);
            if (FAILED(hr)) {
                qErrnoWarning("%s: GetGlyphImageData failed", __FUNCTION__);
                return QRect{};
            }

            QRect rect(-data.horizontalLeftOrigin.x,
                       -data.horizontalLeftOrigin.y,
                       data.pixelSize.width,
                       data.pixelSize.height);

            QTransform matrix(transform.m11, transform.m12,
                              transform.m21, transform.m22,
                              transform.dx, transform.dy);

            // GetGlyphImageData returns the closest matching size, which we need to scale down
            const qreal scale = fontDef.pixelSize / data.pixelsPerEm;
            matrix.scale(scale, scale);

            rect = matrix.mapRect(rect);
            face4->ReleaseGlyphImageData(ctx);

            return rect;
        }
    }

    return QRect{};
#else
    Q_UNUSED(glyph);
    Q_UNUSED(transform);
    return QRect{};
#endif // QT_CONFIG(directwrite3)
}

glyph_metrics_t QWindowsFontEngineDirectWrite::alphaMapBoundingBox(glyph_t glyph,
                                                                   const QFixedPoint &subPixelPosition,
                                                                   const QTransform &originalTransform,
                                                                   GlyphFormat format)
{
    QTransform matrix = originalTransform;
    if (fontDef.stretch != 100 && fontDef.stretch != QFont::AnyStretch)
        matrix.scale(fontDef.stretch / 100.0, 1.0);

    glyph_metrics_t bbox = QFontEngine::boundingBox(glyph, matrix); // To get transformed advance

    DWRITE_MATRIX transform;
    transform.dx = subPixelPosition.x.toReal();
    transform.dy = 0;
    transform.m11 = matrix.m11();
    transform.m12 = matrix.m12();
    transform.m21 = matrix.m21();
    transform.m22 = matrix.m22();

    // Try general approach first (works with regular truetype glyphs as well as COLRv0)
    QRect rect = alphaTextureBounds(glyph, transform);

    // If this fails, we check if it is an embedded color bitmap
    if (rect.isEmpty())
        rect = colorBitmapBounds(glyph, transform);

    // If we are unable to find metrics, just return the design metrics
    if (rect.isEmpty())
        return bbox;

    int margin = glyphMargin(format);
    return glyph_metrics_t(rect.left(),
                           rect.top(),
                           rect.right() - rect.left() + margin * 2,
                           rect.bottom() - rect.top() + margin * 2,
                           bbox.xoff, bbox.yoff);
}

QImage QWindowsFontEngineDirectWrite::bitmapForGlyph(glyph_t glyph,
                                                     const QFixedPoint &subPixelPosition,
                                                     const QTransform &t,
                                                     const QColor &color)
{
    return imageForGlyph(glyph, subPixelPosition, glyphMargin(QFontEngine::Format_ARGB), t, color);
}

QList<QFontVariableAxis> QWindowsFontEngineDirectWrite::variableAxes() const
{
    return m_variableAxes;
}

QT_END_NAMESPACE
