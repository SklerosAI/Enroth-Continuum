#pragma once

class InputStream;
class Blob;
struct OutdoorLocation_MM7;

// Decode an MM6 ODM into the common outdoor snapshot used by the renderer.
// MM6 omits the MM7 normal tables and uses smaller decoration and spawn records.
void deserializeMM6Outdoor(InputStream &src, OutdoorLocation_MM7 *dst);
void deserializeMM6Outdoor(const Blob &src, OutdoorLocation_MM7 *dst);
