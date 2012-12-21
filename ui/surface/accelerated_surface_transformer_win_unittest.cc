// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <d3d9.h>
#include <random>

#include "base/basictypes.h"
#include "base/hash.h"
#include "base/scoped_native_library.h"
#include "base/stringprintf.h"
#include "base/win/scoped_comptr.h"
#include "base/win/windows_version.h"
#include "testing/gtest/include/gtest/gtest-param-test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/rect.h"
#include "ui/surface/accelerated_surface_transformer_win.h"
#include "ui/surface/accelerated_surface_win.h"
#include "ui/surface/d3d9_utils_win.h"

namespace d3d_utils = ui_surface_d3d9_utils;

using base::win::ScopedComPtr;
using std::uniform_int_distribution;

// Provides a reference rasterizer (all rendering done by software emulation)
// Direct3D device, for use by unit tests.
//
// This class is parameterized so that it runs only on Vista+. See
// WindowsVersionIfVistaOrBetter() for details on this works.
class AcceleratedSurfaceTransformerTest : public testing::TestWithParam<int> {
 public:
  AcceleratedSurfaceTransformerTest() {};

  IDirect3DDevice9Ex* device() { return device_.get(); }

  virtual void SetUp() {
    if (!d3d_module_.is_valid()) {
      if (!d3d_utils::LoadD3D9(&d3d_module_)) {
        GTEST_FAIL();
        return;
      }
    }
    if (!d3d_utils::CreateDevice(d3d_module_,
                                 D3DDEVTYPE_HAL,
                                 D3DPRESENT_INTERVAL_IMMEDIATE,
                                 device_.Receive())) {
      GTEST_FAIL();
      return;
    }
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    SeedRandom("default");
  }

  virtual void TearDown() {
    device_ = NULL;
  }

  void SeedRandom(const char* seed) {
    rng_.seed(base::Hash(seed));
    random_dword_.reset();
  }

  // Locks and fills a surface with a checkerboard pattern where the colors
  // are random but the total image pattern is horizontally and vertically
  // symmetric.
  template<typename ColorType>
  void FillSymmetricRandomCheckerboard(
    IDirect3DSurface9* lockable_surface,
    const gfx::Size& size,
    int checker_square_size) {

    D3DLOCKED_RECT locked_rect;
    ASSERT_HRESULT_SUCCEEDED(
        lockable_surface->LockRect(&locked_rect, NULL, D3DLOCK_DISCARD));
    ColorType* surface = reinterpret_cast<ColorType*>(locked_rect.pBits);
    ASSERT_EQ(0, locked_rect.Pitch % sizeof(ColorType));
    int pitch = locked_rect.Pitch / sizeof(ColorType);

    for (int y = 0; y <= size.height() / 2; y += checker_square_size) {
      for (int x = 0; x <= size.width() / 2; x += checker_square_size) {
        ColorType color = static_cast<ColorType>(RandomColor());
        int y_limit = std::min(size.height() / 2, y + checker_square_size - 1);
        int x_limit = std::min(size.width() / 2, x + checker_square_size - 1);
        for (int y_lo = y; y_lo <= y_limit; y_lo++) {
          for (int x_lo = x; x_lo <= x_limit; x_lo++) {
            int y_hi = size.height() - 1 - y_lo;
            int x_hi = size.width() - 1 - x_lo;
            surface[x_lo + y_lo*pitch] = color;
            surface[x_lo + y_hi*pitch] = color;
            surface[x_hi + y_lo*pitch] = color;
            surface[x_hi + y_hi*pitch] = color;
          }
        }
      }
    }

    lockable_surface->UnlockRect();
  }

  template<typename ColorType>
  void FillRandomCheckerboard(
    IDirect3DSurface9* lockable_surface,
    const gfx::Size& size,
    int checker_square_size) {

    D3DLOCKED_RECT locked_rect;
    ASSERT_HRESULT_SUCCEEDED(
        lockable_surface->LockRect(&locked_rect, NULL, D3DLOCK_DISCARD));
    ColorType* surface = reinterpret_cast<ColorType*>(locked_rect.pBits);
    ASSERT_EQ(0, locked_rect.Pitch % sizeof(ColorType));
    int pitch = locked_rect.Pitch / sizeof(ColorType);

    for (int y = 0; y <= size.height(); y += checker_square_size) {
      for (int x = 0; x <= size.width(); x += checker_square_size) {
        ColorType color = static_cast<ColorType>(RandomColor());
        int y_limit = std::min(size.height(), y + checker_square_size);
        int x_limit = std::min(size.width(), x + checker_square_size);
        for (int square_y = y; square_y < y_limit; square_y++) {
          for (int square_x = x; square_x < x_limit; square_x++) {
            surface[square_x + square_y*pitch] = color;
          }
        }
      }
    }

    lockable_surface->UnlockRect();
  }

  // Approximate color-equality check. Allows for some rounding error.
  bool AssertSameColor(DWORD color_a, DWORD color_b) {
    if (color_a == color_b)
      return true;
    uint8* a = reinterpret_cast<uint8*>(&color_a);
    uint8* b = reinterpret_cast<uint8*>(&color_b);
    int max_error = 0;
    for (int i = 0; i < 4; i++)
      max_error = std::max(max_error,
          std::abs(static_cast<int>(a[i]) - b[i]));

    if (max_error <= kAbsoluteColorErrorTolerance)
      return true;

    EXPECT_EQ(StringPrintf("%d %d %d %d", a[0], a[1], a[2], a[3]),
              StringPrintf("%d %d %d %d", b[0], b[1], b[2], b[3]));

    return false;
  }

  // Asserts that an image is symmetric with respect to itself: both
  // horizontally and vertically, within the tolerance of AssertSameColor.
  template<typename ColorType>
  void AssertSymmetry(IDirect3DSurface9* lockable_surface,
                      const gfx::Size& size) {
    D3DLOCKED_RECT locked_rect;
    ASSERT_HRESULT_SUCCEEDED(
        lockable_surface->LockRect(&locked_rect, NULL, D3DLOCK_READONLY));
    ASSERT_EQ(0, locked_rect.Pitch % sizeof(ColorType));
    int pitch = locked_rect.Pitch / sizeof(ColorType);
    ColorType* surface = reinterpret_cast<ColorType*>(locked_rect.pBits);
    for (int y_lo = 0; y_lo < size.height() / 2; y_lo++) {
      int y_hi = size.height() - 1 - y_lo;
      for (int x_lo = 0; x_lo < size.width() / 2; x_lo++) {
        int x_hi = size.width() - 1 - x_lo;
        if (!AssertSameColor(surface[x_lo + y_lo*pitch],
                             surface[x_hi + y_lo*pitch])) {
          FAIL() << StringPrintf("Pixels (%d, %d) vs. (%d, %d)",
                                 x_lo, y_lo, x_hi, y_lo);
        }
        if (!AssertSameColor(surface[x_hi + y_lo*pitch],
                             surface[x_hi + y_hi*pitch])) {
          FAIL() << StringPrintf("Pixels (%d, %d) vs. (%d, %d)",
                                 x_hi, y_lo, x_hi, y_hi);
        }
        if (!AssertSameColor(surface[x_hi + y_hi*pitch],
                               surface[x_lo + y_hi*pitch])) {
          FAIL() << StringPrintf("Pixels (%d, %d) vs. (%d, %d)",
                                 x_hi, y_hi, x_lo, y_hi);
        }
      }
    }
    lockable_surface->UnlockRect();
  }

  // Asserts that the actual image is a bit-identical, vertically mirrored
  // copy of the expected image.
  template<typename ColorType>
  void AssertIsInvertedCopy(const gfx::Size& size,
                            IDirect3DSurface9* expected,
                            IDirect3DSurface9* actual) {

    D3DLOCKED_RECT locked_expected, locked_actual;
    ASSERT_HRESULT_SUCCEEDED(
        expected->LockRect(&locked_expected, NULL, D3DLOCK_READONLY));
    ASSERT_HRESULT_SUCCEEDED(
        actual->LockRect(&locked_actual, NULL, D3DLOCK_READONLY));
    ASSERT_EQ(0, locked_expected.Pitch % sizeof(ColorType));
    int pitch = locked_expected.Pitch / sizeof(ColorType);
    ColorType* expected_image =
        reinterpret_cast<ColorType*>(locked_expected.pBits);
    ColorType* actual_image =
        reinterpret_cast<ColorType*>(locked_actual.pBits);
    for (int y = 0; y < size.height(); y++) {
      int y_actual = size.height() - 1 - y;
      ASSERT_EQ(0,
          memcmp(&expected_image[y*pitch], &actual_image[y_actual*pitch],
                 sizeof(ColorType) * size.width()))
          << StringPrintf("Rows not equal: %d vs. %d", y, y_actual);
    }
    expected->UnlockRect();
    actual->UnlockRect();
  }

 protected:
  static const int kAbsoluteColorErrorTolerance = 4;

  DWORD RandomColor() {
    return random_dword_(rng_);
  }

  void DoResizeBilinearTest(AcceleratedSurfaceTransformer* gpu_ops,
                            const gfx::Size& src_size,
                            const gfx::Size& dst_size,
                            int checkerboard_size) {

    SCOPED_TRACE(
        StringPrintf("Resizing %dx%d -> %dx%d at checkerboard size of %d",
                     src_size.width(), src_size.height(),
                     dst_size.width(), dst_size.height(),
                     checkerboard_size));

    base::win::ScopedComPtr<IDirect3DSurface9> src, dst;
    ASSERT_TRUE(d3d_utils::CreateTemporaryLockableSurface(
        device(), src_size, src.Receive()));
    ASSERT_TRUE(d3d_utils::CreateTemporaryLockableSurface(
        device(), dst_size, dst.Receive()));

    FillSymmetricRandomCheckerboard<DWORD>(src, src_size, checkerboard_size);

    ASSERT_TRUE(gpu_ops->ResizeBilinear(src, gfx::Rect(src_size), dst));

    AssertSymmetry<DWORD>(dst, dst_size);
  }

  void DoCopyInvertedTest(AcceleratedSurfaceTransformer* gpu_ops,
                          const gfx::Size& size) {

    SCOPED_TRACE(
        StringPrintf("CopyInverted @ %dx%d", size.width(), size.height()));

    base::win::ScopedComPtr<IDirect3DSurface9> checkerboard, src, dst;
    base::win::ScopedComPtr<IDirect3DTexture9> src_texture;
    ASSERT_TRUE(d3d_utils::CreateTemporaryLockableSurface(device(), size,
        checkerboard.Receive()));
    ASSERT_TRUE(d3d_utils::CreateTemporaryRenderTargetTexture(device(), size,
        src_texture.Receive(), src.Receive()));
    ASSERT_TRUE(d3d_utils::CreateTemporaryLockableSurface(device(), size,
        dst.Receive()));

    FillRandomCheckerboard<DWORD>(checkerboard, size, 1);
    ASSERT_HRESULT_SUCCEEDED(
        device()->StretchRect(checkerboard, NULL, src, NULL, D3DTEXF_NONE));
    ASSERT_TRUE(gpu_ops->CopyInverted(src_texture, dst, size));
    AssertIsInvertedCopy<DWORD>(size, checkerboard, dst);
  }

  uniform_int_distribution<DWORD> random_dword_;
  std::mt19937 rng_;
  base::ScopedNativeLibrary d3d_module_;
  base::win::ScopedComPtr<IDirect3DDevice9Ex> device_;
};

TEST_P(AcceleratedSurfaceTransformerTest, Init) {
  AcceleratedSurfaceTransformer gpu_ops;
  ASSERT_TRUE(gpu_ops.Init(device()));
};

TEST_P(AcceleratedSurfaceTransformerTest, TestConsistentRandom) {
  // This behavior should be the same for every execution on every machine.
  // Otherwise tests might be flaky and impossible to debug.
  SeedRandom("AcceleratedSurfaceTransformerTest.TestConsistentRandom");
  ASSERT_EQ(2922058934, RandomColor());

  SeedRandom("AcceleratedSurfaceTransformerTest.TestConsistentRandom");
  ASSERT_EQ(2922058934, RandomColor());
  ASSERT_EQ(4050239976, RandomColor());

  SeedRandom("DifferentSeed");
  ASSERT_EQ(3904108833, RandomColor());
}

TEST_P(AcceleratedSurfaceTransformerTest, MixedOperations) {
  SeedRandom("MixedOperations");

  AcceleratedSurfaceTransformer t;
  ASSERT_TRUE(t.Init(device()));

  DoResizeBilinearTest(&t, gfx::Size(256, 256), gfx::Size(255, 255), 1);
  DoResizeBilinearTest(&t, gfx::Size(256, 256), gfx::Size(255, 255), 2);
  DoCopyInvertedTest(&t, gfx::Size(20, 107));
  DoResizeBilinearTest(&t, gfx::Size(256, 256), gfx::Size(255, 255), 5);
  DoResizeBilinearTest(&t, gfx::Size(256, 256), gfx::Size(64, 64), 5);
  DoResizeBilinearTest(&t, gfx::Size(255, 255), gfx::Size(3, 3), 1);
  DoCopyInvertedTest(&t, gfx::Size(1412, 124));
  DoResizeBilinearTest(&t, gfx::Size(255, 255), gfx::Size(257, 257), 1);
  DoResizeBilinearTest(&t, gfx::Size(255, 255), gfx::Size(257, 257), 2);

  DoCopyInvertedTest(&t, gfx::Size(1512, 7));
  DoResizeBilinearTest(&t, gfx::Size(255, 255), gfx::Size(257, 257), 5);
  DoResizeBilinearTest(&t, gfx::Size(150, 256), gfx::Size(126, 256), 8);
  DoCopyInvertedTest(&t, gfx::Size(1521, 3));
  DoResizeBilinearTest(&t, gfx::Size(150, 256), gfx::Size(126, 256), 1);
  DoCopyInvertedTest(&t, gfx::Size(33, 712));
  DoResizeBilinearTest(&t, gfx::Size(150, 256), gfx::Size(126, 8), 8);
  DoCopyInvertedTest(&t, gfx::Size(33, 2));
  DoResizeBilinearTest(&t, gfx::Size(200, 256), gfx::Size(126, 8), 8);
}

// Tests ResizeBilinear with 16K wide/hight src and dst surfaces.
TEST_P(AcceleratedSurfaceTransformerTest, LargeSurfaces) {
  SeedRandom("LargeSurfaces");

  AcceleratedSurfaceTransformer gpu_ops;
  ASSERT_TRUE(gpu_ops.Init(device()));

  const int lo = 256;
  const int hi = 16384;

  DoResizeBilinearTest(&gpu_ops, gfx::Size(hi, lo), gfx::Size(lo, lo), 1);
  DoResizeBilinearTest(&gpu_ops, gfx::Size(lo, hi), gfx::Size(lo, lo), 1);
  DoResizeBilinearTest(&gpu_ops, gfx::Size(lo, lo), gfx::Size(hi, lo), lo);
  DoResizeBilinearTest(&gpu_ops, gfx::Size(lo, lo), gfx::Size(lo, hi), lo);
  DoCopyInvertedTest(&gpu_ops, gfx::Size(hi, lo));
  DoCopyInvertedTest(&gpu_ops, gfx::Size(lo, hi));
}

// Exercises ResizeBilinear with random minification cases where the
// aspect ratio does not change.
TEST_P(AcceleratedSurfaceTransformerTest, MinifyUniform) {
  SeedRandom("MinifyUniform");

  AcceleratedSurfaceTransformer gpu_ops;
  ASSERT_TRUE(gpu_ops.Init(device()));

  int dims[] = { 21, 63, 64, 65, 99, 127, 128, 129, 192, 255, 256, 257};
  int checkerboards[] = {1, 2, 3, 9};
  uniform_int_distribution<int> dim(0, arraysize(dims) - 1);
  uniform_int_distribution<int> checkerboard(0, arraysize(checkerboards) - 1);

  for (int i = 0; i < 300; i++) {
    // Widths are picked so that dst is smaller than src.
    int dst_width = dims[dim(rng_)];
    int src_width = dims[dim(rng_)];
    if (src_width < dst_width)
      std::swap(dst_width, src_width);

    // src_width is picked to preserve aspect ratio.
    int dst_height = dims[dim(rng_)];
    int src_height = static_cast<int>(
        static_cast<int64>(src_width) * dst_height / dst_width);

    int checkerboard_size = checkerboards[checkerboard(rng_)];

    DoResizeBilinearTest(&gpu_ops,
        gfx::Size(src_width, src_height),  // Src size (larger)
        gfx::Size(dst_width, dst_height),  // Dst size (smaller)
        checkerboard_size);
  }
};

// Exercises ResizeBilinear with random magnification cases where the
// aspect ratio does not change.
//
// Disabled. This test relies on an assertion that resizing preserves
// symmetry in the image, but for the current implementation of ResizeBilinear,
// this does not seem to be true.
TEST_P(AcceleratedSurfaceTransformerTest, DISABLED_MagnifyUniform) {
  SeedRandom("MagnifyUniform");

  AcceleratedSurfaceTransformer gpu_ops;
  ASSERT_TRUE(gpu_ops.Init(device()));

  int dims[] = {63, 64, 65, 99, 127, 128, 129, 192, 255, 256, 257};
  int checkerboards[] = {1, 2, 3, 9};
  uniform_int_distribution<int> dim(0, arraysize(dims) - 1);
  uniform_int_distribution<int> checkerboard(0, arraysize(checkerboards) - 1);

  for (int i = 0; i < 50; i++) {
    // Widths are picked so that b is smaller than a.
    int dst_width = dims[dim(rng_)];
    int src_width = dims[dim(rng_)];
    if (dst_width < src_width)
      std::swap(src_width, dst_width);

    int dst_height = dims[dim(rng_)];
    int src_height = static_cast<int>(
        static_cast<int64>(src_width) * dst_height / dst_width);

    int checkerboard_size = checkerboards[checkerboard(rng_)];

    DoResizeBilinearTest(&gpu_ops,
        gfx::Size(src_width, src_height),  // Src size (smaller)
        gfx::Size(dst_width, dst_height),  // Dst size (larger)
        checkerboard_size);
  }
};

namespace {

// Used to suppress test on Windows versions prior to Vista.
std::vector<int> WindowsVersionIfVistaOrBetter() {
  std::vector<int> result;
  if (base::win::GetVersion() >= base::win::VERSION_VISTA) {
    result.push_back(base::win::GetVersion());
  }
  return result;
}

}  // namespace

INSTANTIATE_TEST_CASE_P(VistaAndUp,
                        AcceleratedSurfaceTransformerTest,
                        ::testing::ValuesIn(WindowsVersionIfVistaOrBetter()));
