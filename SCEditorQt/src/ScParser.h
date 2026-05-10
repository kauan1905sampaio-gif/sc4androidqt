#pragma once
#include "ScFile.h"
#include <QByteArray>
#include <QString>
#include <optional>

class ScParser {
public:
    static std::optional<ScFile> parse(const QByteArray &data, const QString &path);
private:
    static ScFile parseTags(const QByteArray &payload,
                            const QString &path, int version,
                            Compression compression, bool isNewFormat);
    static PixelFormat pixelFormatFromType(int type);
    static int bppForFormat(PixelFormat fmt);
};
