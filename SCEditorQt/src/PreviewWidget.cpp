#include <utility>
#include <QPainterPath>
#include "PreviewWidget.h"
#include <QPainter>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

PreviewWidget::PreviewWidget(QWidget *parent) : QWidget(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PreviewWidget::onTick);

    auto *layout = new QVBoxLayout(this);

    // Canvas area (this widget itself paints)
    layout->addStretch(1);

    // Controls
    auto *ctrl = new QHBoxLayout;
    m_prevBtn  = new QPushButton("◀");
    m_playBtn  = new QPushButton("▶ Play");
    m_nextBtn  = new QPushButton("▶");
    m_frameLabel = new QLabel("Frame 0/0");
    ctrl->addWidget(m_prevBtn);
    ctrl->addWidget(m_playBtn);
    ctrl->addWidget(m_nextBtn);
    ctrl->addWidget(m_frameLabel, 1);
    layout->addLayout(ctrl);

    connect(m_playBtn, &QPushButton::clicked, this, &PreviewWidget::onPlayPause);
    connect(m_prevBtn, &QPushButton::clicked, this, &PreviewWidget::onPrev);
    connect(m_nextBtn, &QPushButton::clicked, this, &PreviewWidget::onNext);
}

void PreviewWidget::setFile(const ScFile &file) {
    m_file = file; m_frame = 0;
    buildTextures();
    update();
}

void PreviewWidget::selectExport(int exportId) {
    m_exportId = exportId; m_frame = 0;
    m_playing = false; m_timer->stop();
    m_playBtn->setText("▶ Play");
    update();
}

void PreviewWidget::buildTextures() {
    m_texImages.clear();
    for (const auto &tex : m_file.textures)
        m_texImages.append(pixelsToImage(tex));
}

QImage PreviewWidget::pixelsToImage(const ScTexture &tex) {
    if (tex.data.isEmpty() || tex.width <= 0 || tex.height <= 0)
        return QImage();
    QImage img(tex.width, tex.height, QImage::Format_RGBA8888);
    int w = tex.width, h = tex.height;
    const auto *src = reinterpret_cast<const quint8*>(tex.data.constData());
    for (int y = 0; y < h; y++) {
        quint8 *dst = img.scanLine(y);
        for (int x = 0; x < w; x++) {
            quint8 r=255,g=255,b=255,a=255;
            switch (tex.pixelFormat) {
            case PixelFormat::RGBA8888:
                r=*src++; g=*src++; b=*src++; a=*src++; break;
            case PixelFormat::RGBA4444: {
                quint16 p = src[0]|(src[1]<<8); src+=2;
                r=((p>>12)&0xf)*17; g=((p>>8)&0xf)*17;
                b=((p>>4)&0xf)*17; a=((p)&0xf)*17; break; }
            case PixelFormat::RGB565: {
                quint16 p = src[0]|(src[1]<<8); src+=2;
                r=((p>>11)&0x1f)<<3; g=((p>>5)&0x3f)<<2; b=(p&0x1f)<<3; break; }
            case PixelFormat::LA88:
                r=g=b=*src++; a=*src++; break;
            case PixelFormat::L8:
                r=g=b=a=*src++; break;
            }
            dst[0]=r; dst[1]=g; dst[2]=b; dst[3]=a; dst+=4;
        }
    }
    return img;
}

static const Movieclip *findMc(const ScFile &f, int expId) {
    for (const auto &exp : f.exports)
        if (exp.id == expId)
            for (const auto &mc : f.movieclips)
                if (mc.id == exp.movieclipId) return &mc;
    return nullptr;
}

void PreviewWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x0E,0x0E,0x14));

    const Movieclip *mc = findMc(m_file, m_exportId);
    if (!mc || mc->frames.isEmpty()) {
        p.setPen(QColor(0x98,0x98,0xB0));
        p.drawText(rect(), Qt::AlignCenter, "No export selected\nDouble-click an export in Viewer");
        return;
    }

    m_frameLabel->setText(QString("Frame %1/%2  |  %3 fps")
        .arg(m_frame+1).arg(mc->frames.size()).arg(mc->fps));

    const auto &frame = mc->frames[m_frame];
    int cx = width()/2, cy = height()/2;

    for (const auto &el : frame.elements) {
        const Shape *shape = nullptr;
        for (const auto &s : m_file.shapes) if (s.id == el.shapeId) { shape=&s; break; }
        if (!shape) continue;

        // Affine matrix
        float ma=1,mb=0,mc2=0,md=1,mtx=0,mty=0;
        if (el.matrixIndex < m_file.matrices.size()) {
            const auto &m = m_file.matrices[el.matrixIndex];
            ma=m.a; mb=m.b; mc2=m.c; md=m.d; mtx=m.tx; mty=m.ty;
        }

        for (const auto &chunk : shape->chunks) {
            if (chunk.textureIndex >= m_texImages.size()) continue;
            const QImage &tex = m_texImages[chunk.textureIndex];
            if (tex.isNull() || chunk.vertices.size() < 3) continue;

            // Build transformed polygon
            QPolygonF poly;
            for (const auto &v : chunk.vertices) {
                float tx2 = ma*v.x + mb*v.y + mtx;
                float ty2 = mc2*v.x + md*v.y + mty;
                poly << QPointF(cx+tx2, cy+ty2);
            }

            // Bounding box crop from texture
            float minU=1,maxU=0,minV=1,maxV=0;
            for (const auto &uv : chunk.uvCoords) {
                minU=qMin(minU,uv.u); maxU=qMax(maxU,uv.u);
                minV=qMin(minV,uv.v); maxV=qMax(maxV,uv.v);
            }
            QRectF srcRect(minU*tex.width(), minV*tex.height(),
                           (maxU-minU)*tex.width(), (maxV-minV)*tex.height());
            QRectF dstRect = poly.boundingRect();

            p.save();
            QPainterPath path; path.addPolygon(poly); p.setClipPath(path);
            p.drawImage(dstRect, tex, srcRect);
            p.restore();
        }
    }
}

void PreviewWidget::onPlayPause() {
    const Movieclip *mc = findMc(m_file, m_exportId);
    if (!mc) return;
    m_playing = !m_playing;
    if (m_playing) {
        m_timer->start(1000 / qMax(1, mc->fps));
        m_playBtn->setText("⏸ Pause");
    } else {
        m_timer->stop();
        m_playBtn->setText("▶ Play");
    }
}

void PreviewWidget::onTick() {
    const Movieclip *mc = findMc(m_file, m_exportId);
    if (!mc || mc->frames.isEmpty()) return;
    m_frame = (m_frame + 1) % mc->frames.size();
    update();
}

void PreviewWidget::onPrev() {
    const Movieclip *mc = findMc(m_file, m_exportId);
    if (!mc || mc->frames.isEmpty()) return;
    m_frame = (m_frame - 1 + mc->frames.size()) % mc->frames.size();
    update();
}

void PreviewWidget::onNext() {
    const Movieclip *mc = findMc(m_file, m_exportId);
    if (!mc || mc->frames.isEmpty()) return;
    m_frame = (m_frame + 1) % mc->frames.size();
    update();
}
