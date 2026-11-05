#pragma once

#include "Utility/Memory/Blob.h"

// MM6 map entries use an 8-byte size header followed by zlib data. MM7's
// 16-byte mvii wrapper is handled separately by lod::decodeMaybeCompressed.
Blob decodeMM6ArchiveEntry(const Blob &source);
