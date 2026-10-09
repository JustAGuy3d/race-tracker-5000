#pragma once
// Colours shared by all screens (0xRRGGBB; convert with rgb()).

#include <cstdint>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace theme {

inline uint32_t rgb(uint32_t hex) { return lgfx::color888((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF); }

constexpr uint32_t C_BG     = 0x0B0B0F;
constexpr uint32_t C_ROW_A  = 0x15151C;
constexpr uint32_t C_ROW_B  = 0x1C1C25;
constexpr uint32_t C_HEADER = 0x15151E;
constexpr uint32_t C_BUTTON = 0x3A3A48;
constexpr uint32_t C_TEXT   = 0xF2F2F2;
constexpr uint32_t C_DIM    = 0x8A8A96;
constexpr uint32_t C_RED    = 0xE10600;
constexpr uint32_t C_PB     = 0x2ECC40;
constexpr uint32_t C_PURPLE = 0xB36BFF;
constexpr uint32_t C_PIT    = 0x3FC8E4;

}  // namespace theme
