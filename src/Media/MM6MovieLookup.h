#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "Engine/GameTitle.h"

#include "Library/Vid/VidReader.h"

// The original MM6 VID directories contain extensionless Smacker entries.
// MM7 keeps its existing .bik/.smk lookup order and archive preference.
inline Blob lookupMovieBlob(GameTitle title, const VidReader &first, const VidReader &second,
                            std::string_view name) {
    if (name.empty())
        return {};

    if (title == GameTitle::MM6) {
        const bool inFirst = first.isOpen() && first.exists(name);
        const bool inSecond = second.isOpen() && second.exists(name);
        if (inFirst && inSecond)
            throw std::runtime_error("MM6 movie name occurs in both VID archives");
        if (!inFirst && !inSecond)
            return {};
        Blob movie = inFirst ? first.read(name) : second.read(name);
        if (!movie.str().starts_with("SMK"))
            throw std::runtime_error("MM6 movie payload is not Smacker");
        return movie;
    }

    const std::string bik = std::string(name) + ".bik";
    const std::string smk = std::string(name) + ".smk";
    if (first.isOpen()) {
        if (first.exists(bik)) return first.read(bik);
        if (first.exists(smk)) return first.read(smk);
    }
    if (second.isOpen()) {
        if (second.exists(bik)) return second.read(bik);
        if (second.exists(smk)) return second.read(smk);
    }
    return {};
}
