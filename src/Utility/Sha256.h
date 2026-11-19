#pragma once

#include <string>
#include <string_view>

/**
 * @param data                          Bytes to hash.
 * @return                              SHA-256 of `data`, as 64 lowercase hex digits.
 */
std::string sha256(std::string_view data);
