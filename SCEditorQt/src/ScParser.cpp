#include "ScParser.h"
#include "Decompressor.h"
#include <QDataStream>
#include <QDebug>

// ── Tag IDs ──────────────────────────────────────────────────────────────────
static const int TAG_EOF             = 0;
static const int TAG_TEXTURE         = 1;
static const int TAG_SHAPE           = 2;
static const int TAG_MOVIECLIP       = 3;
static const int TAG_SHAPE_CHUNK     = 6;
static const int TAG_EXPORT          = 7;
static const int TAG_MATRIX          = 8;
static const int TAG_COLOR_SPACE     = 9;
static const int TAG_MOVIECLIP_FRAME = 11;
static const int TAG_MOVIECLIP_V2    = 12;
static const int TAG_TEXTFIELD       = 15;
static const int TAG_SHAPE_V2        = 18;
static const int TAG_TEXTURE_META    = 24;   // new format: w/h/format
static const int TAG_TEXTURE_NEW     = 94;   // new format: raw pixels (0x5e)

// ── Helpers ───────────────────────────────────────────────────────────────────
static quint16 readU16BE(const QByteArray &d, int &pos) {
    quint16 v = (static_cast<quint8>(d[pos]) << 8) | static_cast<quint8>(d[pos+1]);
    pos += 2; return v;
}
static quint32 readU32BE(const QByteArray &d, int &pos) {
    quint32 v = (static_cast<quint8>(d[pos])   << 24) |
                (static_cast<quint8>(d[pos+1]) << 16) |
                (static_cast<quint8>(d[pos+2]) << 8)  |
                 static_cast<quint8>(d[pos+3]);
    pos += 4; return v;
}
static quint16 readU16LE(const QByteArray &d, int &pos) {
    quint16 v = static_cast<quint8>(d[pos]) | (static_cast<quint8>(d[pos+1]) << 8);
    pos += 2; return v;
}
static qint32 readI32LE(const QByteArray &d, int &pos) {
    quint32 u = static_cast<quint8>(d[pos])        |
               (static_cast<quint8>(d[pos+1]) << 8)  |
               (static_cast<quint8>(d[pos+2]) << 16) |
               (static_cast<quint8>(d[pos+3]) << 24);
    pos += 4; return static_cast<qint32>(u);
}
static quint8 readU8(const QByteArray &d, int &pos) {
    return static_cast<quint8>(d[pos++]);
}
static QString readStr(const QByteArray &d, int &pos) {
    int len = readU16LE(d, pos);
    QString s = QString::fromUtf8(d.mid(pos, len));
    pos += len; return s;
}
static QByteArray readBytes(const QByteArray &d, int &pos, int n) {
    QByteArray b = d.mid(pos, n); pos += n; return b;
}

// ── Header detection ──────────────────────────────────────────────────────────
std::optional<ScFile> ScParser::parse(const QByteArray &data, const QString &path) {
    if (data.size() < 8) return std::nullopt;

    // Magic: always 'S'=0x53 'C'=0x43
    if (static_cast<quint8>(data[0]) != 0x53 || static_cast<quint8>(data[1]) != 0x43) {
        qWarning() << "Invalid SC magic";
        return std::nullopt;
    }

    int pos = 2;
    int versionRaw = readU16BE(data, pos);
    int version = versionRaw;
    Compression compression = Compression::None;
    bool isNewFormat = false;
    QByteArray payload;

    if (versionRaw == 3 || versionRaw == 4) {
        // Classic format
        quint32 compId = readU32BE(data, pos);
        switch (compId) {
            case 0x04: compression = Compression::LZHAM; break;
            case 0x01: compression = Compression::LZMA;  break;
            case 0x28: compression = Compression::ZSTD;  break;
            default:   compression = Compression::None;  break;
        }
        if (versionRaw == 4) pos += 16; // skip MD5
        QByteArray compressed = data.mid(pos);
        if (compression != Compression::None) {
            auto dec = Decompressor::decompress(compressed, compression);
            if (!dec) return std::nullopt;
            payload = *dec;
        } else {
            payload = compressed;
        }
    } else if (versionRaw == 0) {
        // New format (Brawl Stars / newer games)
        isNewFormat = true;
        version = 0;
        /*int subVersion =*/ readU16BE(data, pos); // 0x0001
        quint32 hashLen = readU32BE(data, pos);     // usually 16
        pos += hashLen;                              // skip MD5
        payload = data.mid(pos);                    // uncompressed tags
    } else {
        qWarning() << "Unsupported SC version:" << versionRaw;
        return std::nullopt;
    }

    return parseTags(payload, path, version, compression, isNewFormat);
}

// ── Tag-stream parser ─────────────────────────────────────────────────────────
ScFile ScParser::parseTags(const QByteArray &payload, const QString &path,
                           int version, Compression compression, bool /*isNewFormat*/)
{
    ScFile file;
    file.path        = path;
    file.version     = version;
    file.compression = compression;

    int pos = 0;
    struct TexMeta { int width, height; PixelFormat fmt; };
    std::optional<TexMeta> pendingMeta;

    while (pos < payload.size()) {
        if (pos + 5 > payload.size()) break;
        int tagId   = readU8(payload, pos);
        if (tagId == TAG_EOF) break;
        int blockLen = readI32LE(payload, pos);
        if (blockLen < 0 || pos + blockLen > payload.size()) break;

        QByteArray block = readBytes(payload, pos, blockLen);
        int bp = 0; // block position

        switch (tagId) {

        case TAG_TEXTURE: {
            int type   = readU8(block, bp);
            int width  = readU16LE(block, bp);
            int height = readU16LE(block, bp);
            PixelFormat fmt = pixelFormatFromType(type);
            int bpp    = bppForFormat(fmt);
            int size   = width * height * bpp;
            ScTexture tex;
            tex.index       = file.textures.size();
            tex.width       = width;
            tex.height      = height;
            tex.pixelFormat = fmt;
            tex.data        = block.mid(bp, size);
            file.textures.append(tex);
            break;
        }

        case TAG_TEXTURE_META: {
            int type   = readU8(block, bp);
            int width  = readU16LE(block, bp);
            int height = readU16LE(block, bp);
            pendingMeta = { width, height, pixelFormatFromType(type) };
            break;
        }

        case TAG_TEXTURE_NEW: {
            ScTexture tex;
            tex.index = file.textures.size();
            tex.data  = block;
            if (pendingMeta) {
                tex.width       = pendingMeta->width;
                tex.height      = pendingMeta->height;
                tex.pixelFormat = pendingMeta->fmt;
                pendingMeta.reset();
            } else {
                int side = static_cast<int>(std::sqrt(blockLen / 4.0));
                tex.width = tex.height = side;
                tex.pixelFormat = PixelFormat::RGBA8888;
            }
            file.textures.append(tex);
            break;
        }

        case TAG_SHAPE:
        case TAG_SHAPE_V2: {
            Shape s;
            s.id = readU16LE(block, bp);
            if (bp + 2 <= block.size()) readU16LE(block, bp); // chunk count hint
            file.shapes.append(s);
            break;
        }

        case TAG_SHAPE_CHUNK: {
            ShapeChunk chunk;
            chunk.id           = readU16LE(block, bp);
            chunk.textureIndex = readU8(block, bp);
            int count          = readU8(block, bp);
            for (int i = 0; i < count; i++) {
                Vertex v;
                v.x = readI32LE(block, bp) / 20.0f;
                v.y = readI32LE(block, bp) / 20.0f;
                chunk.vertices.append(v);
            }
            for (int i = 0; i < count; i++) {
                UvCoord uv;
                uv.u = readU16LE(block, bp) / 65535.0f;
                uv.v = readU16LE(block, bp) / 65535.0f;
                chunk.uvCoords.append(uv);
            }
            if (!file.shapes.isEmpty())
                file.shapes.last().chunks.append(chunk);
            break;
        }

        case TAG_MOVIECLIP:
        case TAG_MOVIECLIP_V2: {
            Movieclip mc;
            mc.id  = readU16LE(block, bp);
            mc.fps = readU8(block, bp);
            file.movieclips.append(mc);
            break;
        }

        case TAG_MOVIECLIP_FRAME: {
            MovieclipFrame frame;
            frame.name       = readStr(block, bp);
            int elemCount    = readU16LE(block, bp);
            for (int i = 0; i < elemCount; i++) {
                FrameElement el;
                el.shapeId         = readU16LE(block, bp);
                el.matrixIndex     = readU16LE(block, bp);
                el.colorSpaceIndex = readU16LE(block, bp);
                frame.elements.append(el);
            }
            if (!file.movieclips.isEmpty())
                file.movieclips.last().frames.append(frame);
            break;
        }

        case TAG_EXPORT: {
            int count = readU16LE(block, bp);
            QVector<int> mcIds;
            for (int i = 0; i < count; i++) mcIds.append(readU16LE(block, bp));
            for (int i = 0; i < count; i++) {
                Export exp;
                exp.id          = i;
                exp.name        = readStr(block, bp);
                exp.movieclipId = mcIds[i];
                file.exports.append(exp);
            }
            break;
        }

        case TAG_MATRIX: {
            Matrix m;
            m.id = file.matrices.size();
            m.a  = readI32LE(block, bp) / 1024.0f;
            m.b  = readI32LE(block, bp) / 1024.0f;
            m.c  = readI32LE(block, bp) / 1024.0f;
            m.d  = readI32LE(block, bp) / 1024.0f;
            m.tx = readI32LE(block, bp) / 20.0f;
            m.ty = readI32LE(block, bp) / 20.0f;
            file.matrices.append(m);
            break;
        }

        case TAG_COLOR_SPACE: {
            ColorSpace cs;
            cs.id   = file.colorSpaces.size();
            cs.rMul = readU8(block, bp); cs.gMul = readU8(block, bp);
            cs.bMul = readU8(block, bp); cs.aMul = readU8(block, bp);
            cs.rAdd = readU8(block, bp); cs.gAdd = readU8(block, bp);
            cs.bAdd = readU8(block, bp); cs.aAdd = readU8(block, bp);
            file.colorSpaces.append(cs);
            break;
        }

        case TAG_TEXTFIELD: {
            TextField tf;
            tf.id       = readU16LE(block, bp);
            tf.text     = readStr(block, bp);
            tf.fontName = readStr(block, bp);
            tf.fontSize = readU16LE(block, bp);
            readI32LE(block, bp); // color
            int flags   = readU8(block, bp);
            tf.bold     = flags & 0x01;
            tf.italic   = flags & 0x02;
            file.textFields.append(tf);
            break;
        }

        default: break; // unknown tag — block already consumed
        }
    }
    return file;
}

PixelFormat ScParser::pixelFormatFromType(int type) {
    switch (type) {
        case 2:  return PixelFormat::RGBA4444;
        case 4:  return PixelFormat::RGB565;
        case 6:  return PixelFormat::LA88;
        case 10: return PixelFormat::L8;
        default: return PixelFormat::RGBA8888;
    }
}

int ScParser::bppForFormat(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::RGBA8888: return 4;
        case PixelFormat::L8:       return 1;
        default:                    return 2;
    }
}
