#include "Decompressor.h"
#include <QDebug>

// LZMA decompression using Qt's built-in zlib (for LZMA we use a minimal decoder)
// For full LZMA/ZSTD support, link against liblzma and libzstd in CMakeLists.txt.
// The stubs below show exactly where to plug them in.

std::optional<QByteArray> Decompressor::decompress(const QByteArray &data, Compression c) {
    switch (c) {
    case Compression::None:
        return data;

    case Compression::LZMA: {
        // Requires: target_link_libraries(SCEditor PRIVATE lzma)
        // #include <lzma.h>
        // lzma_stream strm = LZMA_STREAM_INIT;
        // lzma_auto_decoder(&strm, UINT64_MAX, 0);
        // ... feed data, collect output ...
        qWarning() << "LZMA: link liblzma and uncomment decompression code in Decompressor.cpp";
        return std::nullopt;
    }

    case Compression::ZSTD: {
        // Requires: target_link_libraries(SCEditor PRIVATE zstd)
        // #include <zstd.h>
        // size_t frameSize = ZSTD_getFrameContentSize(data.constData(), data.size());
        // QByteArray out(frameSize, 0);
        // ZSTD_decompress(out.data(), frameSize, data.constData(), data.size());
        // return out;
        qWarning() << "ZSTD: link libzstd and uncomment decompression code in Decompressor.cpp";
        return std::nullopt;
    }

    case Compression::LZHAM:
        qWarning() << "LZHAM: no public C++ implementation. Decompress on PC first.";
        return std::nullopt;

    default:
        return std::nullopt;
    }
}
