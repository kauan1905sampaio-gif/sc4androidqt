#pragma once
#include "ScFile.h"
#include <QByteArray>
#include <optional>

class Decompressor {
public:
    static std::optional<QByteArray> decompress(const QByteArray &data, Compression c);
};
