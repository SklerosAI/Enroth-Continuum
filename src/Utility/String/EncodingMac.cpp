#include "Encoding.h"

// Local-only macOS adapter. The MM7 production call sites use UTF-8, ASCII,
// and Windows-1251. Apple iconv's other legacy encoding maps, especially CJK,
// are not byte-for-byte equivalent to the existing ztd.text implementation.
// Keep this adapter opt-in until those maps have an independent parity source.

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <iconv.h>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

constexpr char32_t replacement = 0xfffd;

struct DecodedUtf8 {
    char32_t value;
    size_t bytes;
    bool valid;
};

DecodedUtf8 decodeUtf8(const char *input, size_t size, bool allowSurrogates) {
    assert(size);
    const auto first = static_cast<uint8_t>(input[0]);
    if (first < 0x80)
        return {first, 1, true};

    size_t length = 0;
    char32_t value = 0;
    if (first >= 0xc2 && first <= 0xdf) {
        length = 2;
        value = first & 0x1f;
    } else if (first >= 0xe0 && first <= 0xef) {
        length = 3;
        value = first & 0x0f;
    } else if (first >= 0xf0 && first <= 0xf4) {
        length = 4;
        value = first & 0x07;
    } else {
        return {replacement, 1, false};
    }

    for (size_t i = 1; i < length; ++i) {
        if (i == size)
            return {replacement, i, false};
        const auto next = static_cast<uint8_t>(input[i]);
        if ((next & 0xc0) != 0x80)
            return {replacement, i, false};
        value = (value << 6) | (next & 0x3f);
    }

    const bool overlong = (length == 2 && value < 0x80) ||
                          (length == 3 && value < 0x800) ||
                          (length == 4 && value < 0x10000);
    if (overlong || value > 0x10ffff || (!allowSurrogates && value >= 0xd800 && value <= 0xdfff))
        return {replacement, length, false};
    return {value, length, true};
}

void appendUtf8(std::string &output, char32_t value, bool allowSurrogates = false) {
    if (value > 0x10ffff || (!allowSurrogates && value >= 0xd800 && value <= 0xdfff))
        value = replacement;
    if (value <= 0x7f) {
        output.push_back(static_cast<char>(value));
    } else if (value <= 0x7ff) {
        output.push_back(static_cast<char>(0xc0 | (value >> 6)));
        output.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    } else if (value <= 0xffff) {
        output.push_back(static_cast<char>(0xe0 | (value >> 12)));
        output.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    } else {
        output.push_back(static_cast<char>(0xf0 | (value >> 18)));
        output.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    }
}

void appendUtf16(std::u16string &output, char32_t value, bool allowSurrogates = false) {
    if (value > 0x10ffff || (!allowSurrogates && value >= 0xd800 && value <= 0xdfff))
        value = replacement;
    if (value <= 0xffff) {
        output.push_back(static_cast<char16_t>(value));
    } else {
        value -= 0x10000;
        output.push_back(static_cast<char16_t>(0xd800 + (value >> 10)));
        output.push_back(static_cast<char16_t>(0xdc00 + (value & 0x3ff)));
    }
}

std::string utf16ToUtf8(std::u16string_view input, bool preserveUnpaired) {
    std::string output;
    output.reserve(input.size() * 3);
    for (size_t i = 0; i < input.size(); ++i) {
        char32_t value = input[i];
        if (value >= 0xd800 && value <= 0xdbff && i + 1 < input.size() &&
            input[i + 1] >= 0xdc00 && input[i + 1] <= 0xdfff) {
            value = 0x10000 + ((value - 0xd800) << 10) + (input[++i] - 0xdc00);
        }
        appendUtf8(output, value, preserveUnpaired);
    }
    return output;
}

std::u16string utf8ToUtf16(std::string_view input, bool preserveSurrogates) {
    std::u16string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size();) {
        const DecodedUtf8 decoded = decodeUtf8(input.data() + i, input.size() - i, preserveSurrogates);
        appendUtf16(output, decoded.value, preserveSurrogates);
        i += decoded.bytes;
    }
    return output;
}

const char *encodingName(TextEncoding encoding) {
    switch (encoding) {
    case ENCODING_BYTES:
    case ENCODING_UTF8: return "UTF-8";
    case ENCODING_ASCII: return "ASCII";
    case ENCODING_UTF16_BE: return "UTF-16BE";
    case ENCODING_UTF16_LE: return "UTF-16LE";
    case ENCODING_UTF32_BE: return "UTF-32BE";
    case ENCODING_UTF32_LE: return "UTF-32LE";
    case ENCODING_ISO_8859_1: return "ISO-8859-1";
    case ENCODING_ISO_8859_2: return "ISO-8859-2";
    case ENCODING_ISO_8859_3: return "ISO-8859-3";
    case ENCODING_ISO_8859_4: return "ISO-8859-4";
    case ENCODING_ISO_8859_5: return "ISO-8859-5";
    case ENCODING_ISO_8859_6: return "ISO-8859-6";
    case ENCODING_ISO_8859_7: return "ISO-8859-7";
    case ENCODING_ISO_8859_8: return "ISO-8859-8";
    case ENCODING_ISO_8859_10: return "ISO-8859-10";
    case ENCODING_ISO_8859_13: return "ISO-8859-13";
    case ENCODING_ISO_8859_15: return "ISO-8859-15";
    case ENCODING_ISO_8859_16: return "ISO-8859-16";
    case ENCODING_WINDOWS_1251: return "CP1251";
    case ENCODING_WINDOWS_1252: return "CP1252";
    case ENCODING_WINDOWS_1253: return "CP1253";
    case ENCODING_WINDOWS_1255: return "CP1255";
    case ENCODING_WINDOWS_1256: return "CP1256";
    case ENCODING_WINDOWS_1257: return "CP1257";
    case ENCODING_WINDOWS_1258: return "CP1258";
    case ENCODING_IBM865: return "CP865";
    case ENCODING_IBM866: return "CP866";
    case ENCODING_BIG5: return "BIG5-HKSCS:2001";
    case ENCODING_EUC_KR: return "EUC-KR";
    case ENCODING_GB18030: return "GB18030";
    case ENCODING_SHIFT_JIS: return "SHIFT_JIS";
    case ENCODING_KOI8_R: return "KOI8-R";
    default: throw std::invalid_argument("Unsupported text encoding");
    }
}

class Converter {
public:
    Converter(const char *to, const char *from) : _handle(iconv_open(to, from)) {
        if (_handle == reinterpret_cast<iconv_t>(-1))
            throw std::runtime_error("Mac iconv does not support a required encoding");
    }
    ~Converter() { iconv_close(_handle); }
    Converter(const Converter &) = delete;
    Converter &operator=(const Converter &) = delete;
    iconv_t get() const { return _handle; }

private:
    iconv_t _handle;
};

char32_t unmappedSingleByte(TextEncoding encoding, uint8_t value) {
    // Preserve two explicit public API regression cases while keeping the
    // broader iconv mapping differences visible for later standards-based work.
    if (encoding == ENCODING_WINDOWS_1251 && value == 0x98)
        return 0xffff;
    if (encoding == ENCODING_WINDOWS_1253 && value == 0xaa)
        return 0xffff;
    return replacement;
}

std::optional<std::string> convertOne(iconv_t converter, std::string_view source) {
    std::array<char, 32> buffer = {};
    char *input = const_cast<char *>(source.data());
    size_t remaining = source.size();
    char *target = buffer.data();
    size_t available = buffer.size();
    iconv(converter, nullptr, nullptr, nullptr, nullptr);
    const size_t result = iconv(converter, &input, &remaining, &target, &available);
    if (result == static_cast<size_t>(-1) || remaining)
        return std::nullopt;
    return std::string(buffer.data(), target);
}

std::string encodeFromUtf8(std::string_view source, TextEncoding encoding) {
    Converter forward(encodingName(encoding), "UTF-8");
    Converter reverse("UTF-8", encodingName(encoding));
    std::string output;
    output.reserve(source.size());

    for (size_t i = 0; i < source.size();) {
        const DecodedUtf8 decoded = decodeUtf8(source.data() + i, source.size() - i, false);
        std::string scalar;
        appendUtf8(scalar, decoded.value);
        i += decoded.bytes;

        const std::optional<std::string> encoded = convertOne(forward.get(), scalar);
        if (encoded) {
            const std::optional<std::string> decodedBack = convertOne(reverse.get(), *encoded);
            if (decodedBack && *decodedBack == scalar) {
                output += *encoded;
                continue;
            }
        }
        output.push_back('?');
    }
    return output;
}

std::string transcode(std::string_view source, TextEncoding encoding, bool toUtf8) {
    if (!toUtf8)
        return encodeFromUtf8(source, encoding);
    Converter converter("UTF-8", encodingName(encoding));
    std::string output;
    output.reserve(source.size() * 2);
    char *input = const_cast<char *>(source.data());
    size_t remaining = source.size();

    while (remaining) {
        std::array<char, 4096> buffer = {};
        char *target = buffer.data();
        size_t available = buffer.size();
        const size_t result = iconv(converter.get(), &input, &remaining, &target, &available);
        output.append(buffer.data(), target);
        if (result != static_cast<size_t>(-1))
            continue;
        if (errno == E2BIG)
            continue;
        if (errno != EILSEQ && errno != EINVAL)
            throw std::runtime_error("Mac iconv conversion failed");

        const int error = errno;
        const auto badByte = static_cast<uint8_t>(*input);
        appendUtf8(output, unmappedSingleByte(encoding, badByte));
        size_t consumed = 1;
        if (encoding == ENCODING_UTF8 || encoding == ENCODING_BYTES)
            consumed = decodeUtf8(input, remaining, false).bytes;
        else if (error == EINVAL)
            consumed = remaining;
        else if (encoding == ENCODING_UTF16_BE || encoding == ENCODING_UTF16_LE)
            consumed = std::min<size_t>(2, remaining);
        else if (encoding == ENCODING_UTF32_BE || encoding == ENCODING_UTF32_LE)
            consumed = std::min<size_t>(4, remaining);
        input += consumed;
        remaining -= consumed;
        iconv(converter.get(), nullptr, nullptr, nullptr, nullptr);
    }
    return output;
}

} // namespace

std::string txt::utf16ToUtf8(std::u16string_view input) {
    return ::utf16ToUtf8(input, false);
}

std::u16string txt::utf8ToUtf16(std::string_view input) {
    return ::utf8ToUtf16(input, false);
}

std::string txt::wtf16ToWtf8(std::u16string_view input) {
    return ::utf16ToUtf8(input, true);
}

std::u16string txt::wtf8ToWtf16(std::string_view input) {
    return ::utf8ToUtf16(input, true);
}

std::string txt::encodedToUtf8(std::string_view input, TextEncoding encoding) {
    return transcode(input, encoding, true);
}

std::string txt::utf8ToEncoded(std::string_view input, TextEncoding encoding) {
    return transcode(input, encoding, false);
}

std::u32string txt::encodedToUtf32(std::string_view input, TextEncoding encoding) {
    const std::string utf8 = encodedToUtf8(input, encoding);
    std::u32string output;
    output.reserve(utf8.size());
    for (size_t i = 0; i < utf8.size();) {
        const DecodedUtf8 decoded = decodeUtf8(utf8.data() + i, utf8.size() - i, false);
        output.push_back(decoded.value);
        i += decoded.bytes;
    }
    return output;
}

std::string txt::utf32ToEncoded(std::u32string_view input, TextEncoding encoding) {
    std::string utf8;
    utf8.reserve(input.size() * 4);
    for (char32_t value : input)
        appendUtf8(utf8, value);
    return utf8ToEncoded(utf8, encoding);
}

char32_t txt::encodedToChar32(char value, TextEncoding encoding) {
    const std::u32string decoded = encodedToUtf32(std::string_view(&value, 1), encoding);
    assert(decoded.size() <= 1);
    return decoded.empty() ? replacement : decoded.front();
}
