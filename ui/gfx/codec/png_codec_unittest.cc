// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(USE_SYSTEM_LIBPNG)
#include <png.h>
#else
#include "third_party/libpng/png.h"
#endif

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColorPriv.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "third_party/zlib/zlib.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/size.h"

namespace gfx {

namespace {

void MakeRGBImage(int w, int h, std::vector<unsigned char>* dat) {
  dat->resize(w * h * 3);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      unsigned char* org_px = &(*dat)[(y * w + x) * 3];
      org_px[0] = x * 3;      // r
      org_px[1] = x * 3 + 1;  // g
      org_px[2] = x * 3 + 2;  // b
    }
  }
}

// Set use_transparency to write data into the alpha channel, otherwise it will
// be filled with 0xff. With the alpha channel stripped, this should yield the
// same image as MakeRGBImage above, so the code below can make reference
// images for conversion testing.
void MakeRGBAImage(int w, int h, bool use_transparency,
                   std::vector<unsigned char>* dat) {
  dat->resize(w * h * 4);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      unsigned char* org_px = &(*dat)[(y * w + x) * 4];
      org_px[0] = x * 3;      // r
      org_px[1] = x * 3 + 1;  // g
      org_px[2] = x * 3 + 2;  // b
      if (use_transparency)
        org_px[3] = x*3 + 3;  // a
      else
        org_px[3] = 0xFF;     // a (opaque)
    }
  }
}

// User write function (to be passed to libpng by EncodeImage) which writes
// into a buffer instead of to a file.
void WriteImageData(png_structp png_ptr,
                    png_bytep data,
                    png_size_t length) {
  std::vector<unsigned char>& v =
      *static_cast<std::vector<unsigned char>*>(png_get_io_ptr(png_ptr));
  v.resize(v.size() + length);
  memcpy(&v[v.size() - length], data, length);
}

// User flush function; goes with WriteImageData, above.
void FlushImageData(png_structp /*png_ptr*/) {
}

// Libpng user error function which allows us to print libpng errors using
// Chrome's logging facilities instead of stderr.
void LogLibPNGError(png_structp png_ptr,
                    png_const_charp error_msg) {
  DLOG(ERROR) << "libpng encode error: " << error_msg;
  longjmp(png_jmpbuf(png_ptr), 1);
}

// Goes with LogLibPNGError, above.
void LogLibPNGWarning(png_structp png_ptr,
                      png_const_charp warning_msg) {
  DLOG(ERROR) << "libpng encode warning: " << warning_msg;
}

// Color types supported by EncodeImage. Required because neither libpng nor
// PNGCodec::Encode supports all of the required values.
enum ColorType {
  COLOR_TYPE_GRAY = PNG_COLOR_TYPE_GRAY,
  COLOR_TYPE_GRAY_ALPHA = PNG_COLOR_TYPE_GRAY_ALPHA,
  COLOR_TYPE_PALETTE = PNG_COLOR_TYPE_PALETTE,
  COLOR_TYPE_RGB = PNG_COLOR_TYPE_RGB,
  COLOR_TYPE_RGBA = PNG_COLOR_TYPE_RGBA,
  COLOR_TYPE_BGR,
  COLOR_TYPE_BGRA
};

// PNG encoder used for testing. Required because PNGCodec::Encode doesn't do
// interlaced, palette-based, or grayscale images, but PNGCodec::Decode is
// actually asked to decode these types of images by Chrome.
bool EncodeImage(const std::vector<unsigned char>& input,
                 const int width,
                 const int height,
                 ColorType output_color_type,
                 std::vector<unsigned char>* output,
                 const int interlace_type = PNG_INTERLACE_NONE,
                 std::vector<png_color>* palette = 0,
                 std::vector<unsigned char>* palette_alpha = 0) {
  struct ScopedPNGStructs {
    ScopedPNGStructs(png_struct** s, png_info** i) : s_(s), i_(i) {}
    ~ScopedPNGStructs() { png_destroy_write_struct(s_, i_); }
    png_struct** s_;
    png_info** i_;
  };

  DCHECK(output);
  png_struct* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL, NULL, NULL);
  if (!png_ptr)
    return false;

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, NULL);
    return false;
  }

  ScopedPNGStructs scoped_png_structs(&png_ptr, &info_ptr);

  if (setjmp(png_jmpbuf(png_ptr)))
    return false;

  png_set_error_fn(png_ptr, NULL, LogLibPNGError, LogLibPNGWarning);

  int input_rowbytes = 0;
  int transforms = PNG_TRANSFORM_IDENTITY;

  switch (output_color_type) {
    case COLOR_TYPE_GRAY:
      input_rowbytes = width;
      break;
    case COLOR_TYPE_GRAY_ALPHA:
      input_rowbytes = width * 2;
      break;
    case COLOR_TYPE_PALETTE:
      if (!palette)
        return false;
      input_rowbytes = width;
      break;
    case COLOR_TYPE_RGB:
      input_rowbytes = width * 3;
      break;
    case COLOR_TYPE_RGBA:
      input_rowbytes = width * 4;
      break;
    case COLOR_TYPE_BGR:
      input_rowbytes = width * 3;
      output_color_type = static_cast<ColorType>(PNG_COLOR_TYPE_RGB);
      transforms |= PNG_TRANSFORM_BGR;
      break;
    case COLOR_TYPE_BGRA:
      input_rowbytes = width * 4;
      output_color_type = static_cast<ColorType>(PNG_COLOR_TYPE_RGBA);
      transforms |= PNG_TRANSFORM_BGR;
      break;
  };

  std::vector<png_bytep> row_pointers(height);
  for (int y = 0 ; y < height; y++) {
    row_pointers[y] = const_cast<unsigned char*>(&input[y * input_rowbytes]);
  }
  png_set_rows(png_ptr, info_ptr, &row_pointers[0]);
  png_set_write_fn(png_ptr, output, WriteImageData, FlushImageData);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, output_color_type,
               interlace_type, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  if (output_color_type == COLOR_TYPE_PALETTE) {
    png_set_PLTE(png_ptr, info_ptr, &palette->front(), palette->size());
    if (palette_alpha) {
      png_set_tRNS(png_ptr,
                   info_ptr,
                   &palette_alpha->front(),
                   palette_alpha->size(),
                   NULL);
    }
  }

  png_write_png(png_ptr, info_ptr, transforms, NULL);

  return true;
}

}  // namespace

// Returns true if each channel of the given two colors are "close." This is
// used for comparing colors where rounding errors may cause off-by-one.
bool ColorsClose(uint32_t a, uint32_t b) {
  return abs(static_cast<int>(SkColorGetB(a) - SkColorGetB(b))) < 2 &&
         abs(static_cast<int>(SkColorGetG(a) - SkColorGetG(b))) < 2 &&
         abs(static_cast<int>(SkColorGetR(a) - SkColorGetR(b))) < 2 &&
         abs(static_cast<int>(SkColorGetA(a) - SkColorGetA(b))) < 2;
}

// Returns true if the RGB components are "close."
bool NonAlphaColorsClose(uint32_t a, uint32_t b) {
  return abs(static_cast<int>(SkColorGetB(a) - SkColorGetB(b))) < 2 &&
         abs(static_cast<int>(SkColorGetG(a) - SkColorGetG(b))) < 2 &&
         abs(static_cast<int>(SkColorGetR(a) - SkColorGetR(b))) < 2;
}

void MakeTestSkBitmap(int w, int h, SkBitmap* bmp) {
  bmp->setConfig(SkBitmap::kARGB_8888_Config, w, h);
  bmp->allocPixels();

  uint32_t* src_data = bmp->getAddr32(0, 0);
  for (int i = 0; i < w * h; i++) {
    src_data[i] = SkPreMultiplyARGB(i % 255, i % 250, i % 245, i % 240);
  }
}

TEST(PNGCodec, EncodeDecodeRGB) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(PNGCodec::Encode(&original[0], PNGCodec::FORMAT_RGB,
                               Size(w, h), w * 3, false,
                               std::vector<PNGCodec::Comment>(),
                               &encoded));

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGB, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be equal
  ASSERT_TRUE(original == decoded);
}

TEST(PNGCodec, EncodeDecodeRGBA) {
  const int w = 20, h = 20;

  // create an image with known values, a must be opaque because it will be
  // lost during encoding
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, true, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(PNGCodec::Encode(&original[0], PNGCodec::FORMAT_RGBA,
                               Size(w, h), w * 4, false,
                               std::vector<PNGCodec::Comment>(),
                               &encoded));

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGBA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be exactly equal
  ASSERT_TRUE(original == decoded);
}

TEST(PNGCodec, EncodeDecodeBGRA) {
  const int w = 20, h = 20;

  // Create an image with known values, alpha must be opaque because it will be
  // lost during encoding.
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, true, &original);

  // Encode.
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(PNGCodec::Encode(&original[0], PNGCodec::FORMAT_BGRA,
                               Size(w, h), w * 4, false,
                               std::vector<PNGCodec::Comment>(),
                               &encoded));

  // Decode, it should have the same size as the original.
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_BGRA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be exactly equal.
  ASSERT_TRUE(original == decoded);
}

TEST(PNGCodec, DecodeInterlacedRGB) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_RGB,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGB, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be equal
  ASSERT_EQ(original, decoded);
}

TEST(PNGCodec, DecodeInterlacedRGBA) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, false, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_RGBA,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGBA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be equal
  ASSERT_EQ(original, decoded);
}

TEST(PNGCodec, DecodeInterlacedRGBADiscardAlpha) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, false, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_RGBA,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // decode
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGB, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(decoded.size(), w * h * 3U);

  // Images must be equal
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      unsigned char* orig_px = &original[(y * w + x) * 4];
      unsigned char* dec_px = &decoded[(y * w + x) * 3];
      ASSERT_EQ(dec_px[0], orig_px[0]);
      ASSERT_EQ(dec_px[1], orig_px[1]);
      ASSERT_EQ(dec_px[2], orig_px[2]);
    }
  }
}

TEST(PNGCodec, DecodeInterlacedBGR) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_BGR,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_BGRA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(decoded.size(), w * h * 4U);

  // Images must be equal
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      unsigned char* orig_px = &original[(y * w + x) * 3];
      unsigned char* dec_px = &decoded[(y * w + x) * 4];
      ASSERT_EQ(dec_px[0], orig_px[0]);
      ASSERT_EQ(dec_px[1], orig_px[1]);
      ASSERT_EQ(dec_px[2], orig_px[2]);
    }
  }
}

TEST(PNGCodec, DecodeInterlacedBGRA) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, false, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_BGRA,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  ASSERT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_BGRA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be equal
  ASSERT_EQ(original, decoded);
}

// Not encoding an interlaced PNG from SkBitmap because we don't do it
// anywhere, and the ability to do that requires more code changes.
TEST(PNGCodec, DecodeInterlacedRGBtoSkBitmap) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_RGB,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // Decode the encoded string.
  SkBitmap decoded_bitmap;
  ASSERT_TRUE(PNGCodec::Decode(&encoded.front(), encoded.size(),
                               &decoded_bitmap));

  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      const unsigned char* original_pixel = &original[(y * w + x) * 3];
      const uint32_t original_pixel_sk = SkPackARGB32(0xFF,
                                                      original_pixel[0],
                                                      original_pixel[1],
                                                      original_pixel[2]);
      const uint32_t decoded_pixel = decoded_bitmap.getAddr32(0, y)[x];
      ASSERT_EQ(original_pixel_sk, decoded_pixel);
    }
  }
}

TEST(PNGCodec, DecodeInterlacedRGBAtoSkBitmap) {
  const int w = 20, h = 20;

  // create an image with known values
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, false, &original);

  // encode
  std::vector<unsigned char> encoded;
  ASSERT_TRUE(EncodeImage(original,
                          w, h,
                          COLOR_TYPE_RGBA,
                          &encoded,
                          PNG_INTERLACE_ADAM7));

  // Decode the encoded string.
  SkBitmap decoded_bitmap;
  ASSERT_TRUE(PNGCodec::Decode(&encoded.front(), encoded.size(),
                               &decoded_bitmap));

  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      const unsigned char* original_pixel = &original[(y * w + x) * 4];
      const uint32_t original_pixel_sk = SkPackARGB32(original_pixel[3],
                                                      original_pixel[0],
                                                      original_pixel[1],
                                                      original_pixel[2]);
      const uint32_t decoded_pixel = decoded_bitmap.getAddr32(0, y)[x];
      ASSERT_EQ(original_pixel_sk, decoded_pixel);
    }
  }
}

// Test that corrupted data decompression causes failures.
TEST(PNGCodec, DecodeCorrupted) {
  int w = 20, h = 20;

  // Make some random data (an uncompressed image).
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  // It should fail when given non-JPEG compressed data.
  std::vector<unsigned char> output;
  int outw, outh;
  EXPECT_FALSE(PNGCodec::Decode(&original[0], original.size(),
                                PNGCodec::FORMAT_RGB, &output,
                                &outw, &outh));

  // Make some compressed data.
  std::vector<unsigned char> compressed;
  ASSERT_TRUE(PNGCodec::Encode(&original[0], PNGCodec::FORMAT_RGB,
                               Size(w, h), w * 3, false,
                               std::vector<PNGCodec::Comment>(),
                               &compressed));

  // Try decompressing a truncated version.
  EXPECT_FALSE(PNGCodec::Decode(&compressed[0], compressed.size() / 2,
                                PNGCodec::FORMAT_RGB, &output,
                                &outw, &outh));

  // Corrupt it and try decompressing that.
  for (int i = 10; i < 30; i++)
    compressed[i] = i;
  EXPECT_FALSE(PNGCodec::Decode(&compressed[0], compressed.size(),
                                PNGCodec::FORMAT_RGB, &output,
                                &outw, &outh));
}

TEST(PNGCodec, StripAddAlpha) {
  const int w = 20, h = 20;

  // These should be the same except one has a 0xff alpha channel.
  std::vector<unsigned char> original_rgb;
  MakeRGBImage(w, h, &original_rgb);
  std::vector<unsigned char> original_rgba;
  MakeRGBAImage(w, h, false, &original_rgba);

  // Encode RGBA data as RGB.
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(PNGCodec::Encode(&original_rgba[0], PNGCodec::FORMAT_RGBA,
                               Size(w, h), w * 4, true,
                               std::vector<PNGCodec::Comment>(),
                               &encoded));

  // Decode the RGB to RGBA.
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGBA, &decoded,
                               &outw, &outh));

  // Decoded and reference should be the same (opaque alpha).
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original_rgba.size(), decoded.size());
  ASSERT_EQ(original_rgba, decoded);

  // Encode RGBA to RGBA.
  EXPECT_TRUE(PNGCodec::Encode(&original_rgba[0], PNGCodec::FORMAT_RGBA,
                               Size(w, h), w * 4, false,
                               std::vector<PNGCodec::Comment>(),
                               &encoded));

  // Decode the RGBA to RGB.
  EXPECT_TRUE(PNGCodec::Decode(&encoded[0], encoded.size(),
                               PNGCodec::FORMAT_RGB, &decoded,
                               &outw, &outh));

  // It should be the same as our non-alpha-channel reference.
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original_rgb.size(), decoded.size());
  ASSERT_EQ(original_rgb, decoded);
}

TEST(PNGCodec, EncodeBGRASkBitmap) {
  const int w = 20, h = 20;

  SkBitmap original_bitmap;
  MakeTestSkBitmap(w, h, &original_bitmap);

  // Encode the bitmap.
  std::vector<unsigned char> encoded;
  PNGCodec::EncodeBGRASkBitmap(original_bitmap, false, &encoded);

  // Decode the encoded string.
  SkBitmap decoded_bitmap;
  EXPECT_TRUE(PNGCodec::Decode(&encoded.front(), encoded.size(),
                               &decoded_bitmap));

  // Compare the original bitmap and the output bitmap. We use ColorsClose
  // as SkBitmaps are considered to be pre-multiplied, the unpremultiplication
  // (in Encode) and repremultiplication (in Decode) can be lossy.
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      uint32_t original_pixel = original_bitmap.getAddr32(0, y)[x];
      uint32_t decoded_pixel = decoded_bitmap.getAddr32(0, y)[x];
      EXPECT_TRUE(ColorsClose(original_pixel, decoded_pixel));
    }
  }
}

TEST(PNGCodec, EncodeBGRASkBitmapDiscardTransparency) {
  const int w = 20, h = 20;

  SkBitmap original_bitmap;
  MakeTestSkBitmap(w, h, &original_bitmap);

  // Encode the bitmap.
  std::vector<unsigned char> encoded;
  PNGCodec::EncodeBGRASkBitmap(original_bitmap, true, &encoded);

  // Decode the encoded string.
  SkBitmap decoded_bitmap;
  EXPECT_TRUE(PNGCodec::Decode(&encoded.front(), encoded.size(),
                               &decoded_bitmap));

  // Compare the original bitmap and the output bitmap. We need to
  // unpremultiply original_pixel, as the decoded bitmap doesn't have an alpha
  // channel.
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      uint32_t original_pixel = original_bitmap.getAddr32(0, y)[x];
      uint32_t unpremultiplied =
          SkUnPreMultiply::PMColorToColor(original_pixel);
      uint32_t decoded_pixel = decoded_bitmap.getAddr32(0, y)[x];
      EXPECT_TRUE(NonAlphaColorsClose(unpremultiplied, decoded_pixel))
          << "Original_pixel: ("
          << SkColorGetR(unpremultiplied) << ", "
          << SkColorGetG(unpremultiplied) << ", "
          << SkColorGetB(unpremultiplied) << "), "
          << "Decoded pixel: ("
          << SkColorGetR(decoded_pixel) << ", "
          << SkColorGetG(decoded_pixel) << ", "
          << SkColorGetB(decoded_pixel) << ")";
    }
  }
}

TEST(PNGCodec, EncodeWithComment) {
  const int w = 10, h = 10;

  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  std::vector<unsigned char> encoded;
  std::vector<PNGCodec::Comment> comments;
  comments.push_back(PNGCodec::Comment("key", "text"));
  comments.push_back(PNGCodec::Comment("test", "something"));
  comments.push_back(PNGCodec::Comment("have some", "spaces in both"));
  EXPECT_TRUE(PNGCodec::Encode(&original[0], PNGCodec::FORMAT_RGB,
                               Size(w, h), w * 3, false, comments, &encoded));

  // Each chunk is of the form length (4 bytes), chunk type (tEXt), data,
  // checksum (4 bytes).  Make sure we find all of them in the encoded
  // results.
  const unsigned char kExpected1[] =
      "\x00\x00\x00\x08tEXtkey\x00text\x9e\xe7\x66\x51";
  const unsigned char kExpected2[] =
      "\x00\x00\x00\x0etEXttest\x00something\x29\xba\xef\xac";
  const unsigned char kExpected3[] =
      "\x00\x00\x00\x18tEXthave some\x00spaces in both\x8d\x69\x34\x2d";

  EXPECT_NE(std::search(encoded.begin(), encoded.end(), kExpected1,
                        kExpected1 + arraysize(kExpected1)),
            encoded.end());
  EXPECT_NE(std::search(encoded.begin(), encoded.end(), kExpected2,
                        kExpected2 + arraysize(kExpected2)),
            encoded.end());
  EXPECT_NE(std::search(encoded.begin(), encoded.end(), kExpected3,
                        kExpected3 + arraysize(kExpected3)),
            encoded.end());
}

TEST(PNGCodec, EncodeDecodeWithVaryingCompressionLevels) {
  const int w = 20, h = 20;

  // create an image with known values, a must be opaque because it will be
  // lost during encoding
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, true, &original);

  // encode
  std::vector<unsigned char> encoded_fast;
  EXPECT_TRUE(PNGCodec::EncodeWithCompressionLevel(
        &original[0], PNGCodec::FORMAT_RGBA, Size(w, h), w * 4, false,
        std::vector<PNGCodec::Comment>(), Z_BEST_SPEED, &encoded_fast));

  std::vector<unsigned char> encoded_best;
  EXPECT_TRUE(PNGCodec::EncodeWithCompressionLevel(
        &original[0], PNGCodec::FORMAT_RGBA, Size(w, h), w * 4, false,
        std::vector<PNGCodec::Comment>(), Z_BEST_COMPRESSION, &encoded_best));

  // Make sure the different compression settings actually do something; the
  // sizes should be different.
  EXPECT_NE(encoded_fast.size(), encoded_best.size());

  // decode, it should have the same size as the original
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(PNGCodec::Decode(&encoded_fast[0], encoded_fast.size(),
                               PNGCodec::FORMAT_RGBA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  EXPECT_TRUE(PNGCodec::Decode(&encoded_best[0], encoded_best.size(),
                               PNGCodec::FORMAT_RGBA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  // Images must be exactly equal
  ASSERT_TRUE(original == decoded);
}


}  // namespace gfx
