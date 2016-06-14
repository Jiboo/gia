/*
 * Gia: Hobby "gpu friendly" image file format.
 * Copyright (C) 2016 Jean-Baptiste "Jiboo" Lepesme
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>

namespace gia {

static const uint8_t SIGNATURE[6] = {0x90, 'G', 'I', 'A', '\n', '\n'};
static const uint8_t FORMAT_REVISION = 1;

enum pixel_format_t : uint8_t {
  BC4_L8        = 0x01,
  BC5_L8A8      = 0x02,
  BC1_R5G6B5    = 0x03,
  BC1_R5G6B5A1  = 0x04,
  BC2_R5G6B5A4  = 0x05,
  BC3_R5G6B5A8  = 0x06,
  BC7_R8G8B8A8  = 0x07,

  ASTC_4x4      = 0x10,
  ASTC_5x4      = 0x11,
  ASTC_5x5      = 0x12,
  ASTC_6x5      = 0x13,
  ASTC_6x6      = 0x14,
  ASTC_8x5      = 0x15,
  ASTC_8x6      = 0x16,
  ASTC_8x8      = 0x17,
  ASTC_10x5     = 0x18,
  ASTC_10x6     = 0x19,
  ASTC_10x8     = 0x1a,
  ASTC_10x10    = 0x1b,
  ASTC_12x10    = 0x1c,
  ASTC_12x12    = 0x1d,

  RAW_L8        = 0xf0,
  RAW_L4A4      = 0xf1,
  RAW_L8A8      = 0xf2,
  RAW_R5G6B5    = 0xf3,
  RAW_R8G8B8    = 0xf4,
  RAW_R5G5B5A1  = 0xf5,
  RAW_R4G4B4A4  = 0xf6,
  RAW_R8G8B8A8  = 0xf7,
};

enum image_flags_t : uint8_t {
  /** This image can be treated as opaque. */
  OPAQUE            = 0x01,

  /** The color channel valued should be treated as signed. */
  SIGNED            = 0x02,

  /** RGB channels are represented in sRGB colorspace. */
  SRGB              = 0x04,

  /** Color components of this image are pre-multiplied by the alpha one. */
  PREMULTIPLIED     = 0x08,

  /** This image has multiple mip levels. */
  MIPMAPED          = 0x10,

  /** This image has multiple slices. */
  ANIMATED          = 0x20,
};

struct Header {
  uint8_t version_ = gia::FORMAT_REVISION;

  image_flags_t flags_;

  /** Texture count in this file. Can't either be 0 nor 0xff.
   * Although GIA can store multiple textures per file, this is used only for
   * chroma subsampling. */
  uint8_t textures_;

  /** Intended display density of the image, in px/inch. */
  uint8_t density_;

  /** DCT image hash (see phash.org) */
  uint64_t phash_;

  /** CRC64 of all the blocks in the archive. */
  uint64_t crc_;

  /** Image colors swatches, works similarly to Android's Palette.
   * From index 0 to 6: vibrant, vibrant+, vibrant-, muted, muted+, muted-. */
  uint32_t swatches_[6];
};

enum texture_flags_t : uint8_t {
  /** This texture is luminance, has an extra texture for chromas(+alpha). */
  CHROMA_SUBSAMPLED = 0x01,

  /** This texture is palette coded, has an extra texture for palette book. */
  PALETTED          = 0x02,

  /** This texture should be blended with Header::colors_[0] when rendered.
   * Only useful for L/LA textures, so that we can add some color on it. */
  TINTED            = 0x04,
};

struct TextureInfo {
  texture_flags_t flags_;

  /** ID of this texture. */
  uint8_t id_;

  /** Extra texture required for rendering (chroma texture, or palette). */
  uint8_t extraID_;

  /** Number of mip levels. 1 == no mipmapping. */
  uint8_t mipLevels_;

  /** Number of frames for this texture. 1 == no animation */
  uint16_t slices_;

  // Some padding to align this struct to 4 bytes
  uint16_t reserved_;

  /** Size of the texture (maximum lod mipmap). */
  uint16_t width_, height_;
};

enum block_flags_t: uint8_t {
  /** Block data is compressed using LZ4. */
  COMPRESSED   = 0x01,

  /** Block data is pre-filtered using an equivalent of the PNG "sub" filter
   * method. Will need an extra step when en/decoding. Note: raw only, probably
   * not a good idea with block based formats. */
  SUB_FILTERED = 0x02,

  /** Storage will be cleared to the specified color. */
  CLEAR        = 0x04,

  /** Storage will be cleared to the previous slice. Can't be set on slice 0.
   * No blending with previous frame, all channels are replaced. */
  COPY         = 0x08,
};

struct Block {
  /** Format of this block, a block for the same texture+mip+slice might be
   * present in the file, with a different format. */
  pixel_format_t format_;

  block_flags_t flags_;

  uint8_t textureId_;
  uint8_t mipLevel_;
  uint16_t slice_;

  /** Time in ms that this frame should long.
   * If 0, the animation should stop looping. */
  uint16_t delay_;

  uint16_t width_, height_;

  /** Data bounds in the image (might be smaller than this mip size), you can
   * either clear the rest of the data, or copy them from the previous frame
   * using flags. If neither clear nor copy flags are set, the extent is
   * ignored, we expect the full pixel data for it. */
  uint16_t extentX1_, extentY1_, extentX2_, extentY2_;

  /** Clear color used before copying data if the clear flag is set. */
  uint32_t clearRGBA_;

  /** The size in the archive file of the data (so after compression, if
   * enabled). Can be 0, for image with just one color, we can just clear. */
  uint32_t dataSize_;

  /** The format has an hidden uint32_t here if the COMPRESSED flag is enabled,
   * representing the size of the uncompressed data. */

  /** Then comes the data in the file. Followed by other blocks. */
};

}  // namespace gia
