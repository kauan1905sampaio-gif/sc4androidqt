#pragma once
#include "ScFile.h"
#include <QVector>

class ScCombiner {
public:
    // Combine exports from 'source' (by exportIds) into 'base'.
    // All dependent movieclips, shapes, textures, matrices and color-spaces
    // are imported with ID remapping to avoid collisions.
    static ScFile combine(const ScFile &base, const ScFile &source,
                          const QVector<int> &exportIds);
};
