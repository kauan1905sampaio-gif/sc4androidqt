#include <utility>
#include "ScCombiner.h"
#include <QSet>
#include <QMap>
#include <algorithm>

// Helpers — explicit types instead of 'auto' in template parameter
static int maxExportId(const QVector<Export> &vec) {
    int m = 0;
    for (const Export &v : vec) m = qMax(m, v.id);
    return m;
}
static int maxMcId(const QVector<Movieclip> &vec) {
    int m = 0;
    for (const Movieclip &v : vec) m = qMax(m, v.id);
    return m;
}
static int maxShapeId(const QVector<Shape> &vec) {
    int m = 0;
    for (const Shape &v : vec) m = qMax(m, v.id);
    return m;
}

static void collectMcIds(int id, const ScFile &src, QSet<int> &out) {
    if (out.contains(id)) return;
    out.insert(id);
    for (const Movieclip &mc : src.movieclips) {
        if (mc.id != id) continue;
        for (const MovieclipFrame &fr : mc.frames) {
            for (const FrameElement &el : fr.elements) {
                bool isMc = std::any_of(
                    src.movieclips.begin(), src.movieclips.end(),
                    [&](const Movieclip &m){ return m.id == el.shapeId; });
                if (isMc) collectMcIds(el.shapeId, src, out);
            }
        }
    }
}

ScFile ScCombiner::combine(const ScFile &base, const ScFile &source,
                           const QVector<int> &exportIds)
{
    ScFile result = base;

    int mcOffset  = maxMcId(base.movieclips)   + 1;
    int shpOffset = maxShapeId(base.shapes)     + 1;

    // Collect needed movieclip IDs
    QSet<int> mcIds;
    for (const Export &exp : source.exports)
        if (exportIds.contains(exp.id))
            collectMcIds(exp.movieclipId, source, mcIds);

    // Collect needed shape IDs
    QSet<int> shapeIds;
    for (int mid : std::as_const(mcIds)) {
        for (const Movieclip &mc : source.movieclips) {
            if (mc.id != mid) continue;
            for (const MovieclipFrame &fr : mc.frames)
                for (const FrameElement &el : fr.elements)
                    shapeIds.insert(el.shapeId);
        }
    }

    // Textures
    QMap<int,int> texMap;
    for (int i = 0; i < source.textures.size(); i++) {
        texMap.insert(i, result.textures.size());
        ScTexture t = source.textures[i];
        t.index = result.textures.size();
        result.textures.append(t);
    }

    // Shapes
    QMap<int,int> shapeMap;
    for (const Shape &s : source.shapes) {
        if (!shapeIds.contains(s.id)) continue;
        int newId = s.id + shpOffset;
        shapeMap.insert(s.id, newId);
        Shape ns = s;
        ns.id = newId;
        for (ShapeChunk &c : ns.chunks)
            c.textureIndex = texMap.value(c.textureIndex, c.textureIndex);
        result.shapes.append(ns);
    }

    // Matrices
    QMap<int,int> matMap;
    for (int i = 0; i < source.matrices.size(); i++) {
        matMap.insert(i, result.matrices.size());
        Matrix m = source.matrices[i];
        m.id = result.matrices.size();
        result.matrices.append(m);
    }

    // ColorSpaces
    QMap<int,int> csMap;
    for (int i = 0; i < source.colorSpaces.size(); i++) {
        csMap.insert(i, result.colorSpaces.size());
        ColorSpace cs = source.colorSpaces[i];
        cs.id = result.colorSpaces.size();
        result.colorSpaces.append(cs);
    }

    // Movieclips
    QMap<int,int> mcMap;
    for (const Movieclip &mc : source.movieclips) {
        if (!mcIds.contains(mc.id)) continue;
        int newId = mc.id + mcOffset;
        mcMap.insert(mc.id, newId);
        Movieclip nmc = mc;
        nmc.id = newId;
        for (MovieclipFrame &fr : nmc.frames) {
            for (FrameElement &el : fr.elements) {
                el.shapeId         = shapeMap.value(el.shapeId,         el.shapeId);
                el.matrixIndex     = matMap.value(el.matrixIndex,        el.matrixIndex);
                el.colorSpaceIndex = csMap.value(el.colorSpaceIndex,     el.colorSpaceIndex);
            }
        }
        result.movieclips.append(nmc);
    }

    // Exports
    for (const Export &exp : source.exports) {
        if (!exportIds.contains(exp.id)) continue;
        Export ne = exp;
        ne.id          = maxExportId(result.exports) + 1;
        ne.movieclipId = mcMap.value(exp.movieclipId, exp.movieclipId);
        result.exports.append(ne);
    }

    return result;
}
