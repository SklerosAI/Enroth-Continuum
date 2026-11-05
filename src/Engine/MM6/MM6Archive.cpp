#include "MM6Archive.h"

#include <cstdint>

#include "Library/Binary/CommonSerialization.h"
#include "Library/Compression/Compression.h"
#include "Library/LodFormats/LodFormats.h"

#include "Utility/Exception.h"
#include "Utility/Streams/MemoryInputStream.h"

Blob decodeMM6ArchiveEntry(const Blob &source) {
    if (lod::detectCompressedData(source) || lod::detectCompressedPseudoImage(source))
        return lod::decodeMaybeCompressed(source);

    if (source.size() < 8)
        return Blob::share(source);

    MemoryInputStream stream(source.data(), source.size(), source.displayPath());
    uint32_t compressedSize = 0;
    uint32_t decompressedSize = 0;
    deserialize(stream, &compressedSize);
    deserialize(stream, &decompressedSize);

    if (compressedSize != source.size() - 8 || decompressedSize == 0)
        return Blob::share(source);
    if (decompressedSize > 64 * 1024 * 1024)
        throw Exception("MM6 archive entry '{}' exceeds the 64 MiB size limit", source.displayPath());

    return zlib::uncompress(source.subBlob(8, compressedSize), decompressedSize).withDisplayPath(source.displayPath());
}
