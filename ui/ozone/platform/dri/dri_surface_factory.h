// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRI_DRI_SURFACE_FACTORY_H_
#define UI_OZONE_PLATFORM_DRI_DRI_SURFACE_FACTORY_H_

#include "base/memory/scoped_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/ozone/surface_factory_ozone.h"
#include "ui/ozone/ozone_export.h"

namespace gfx {
class SurfaceOzoneCanvas;
}

namespace ui {

class DriSurface;
class DriWrapper;
class HardwareDisplayController;

// SurfaceFactoryOzone implementation on top of DRM/KMS using dumb buffers.
// This implementation is used in conjunction with the software rendering
// path.
class OZONE_EXPORT DriSurfaceFactory : public gfx::SurfaceFactoryOzone {
 public:
  static const gfx::AcceleratedWidget kDefaultWidgetHandle;

  DriSurfaceFactory();
  virtual ~DriSurfaceFactory();

  virtual HardwareState InitializeHardware() OVERRIDE;
  virtual void ShutdownHardware() OVERRIDE;

  virtual gfx::AcceleratedWidget GetAcceleratedWidget() OVERRIDE;

  virtual scoped_ptr<gfx::SurfaceOzoneCanvas> CreateCanvasForWidget(
      gfx::AcceleratedWidget w) OVERRIDE;

  virtual bool LoadEGLGLES2Bindings(
      AddGLLibraryCallback add_gl_library,
      SetGLGetProcAddressProcCallback set_gl_get_proc_address) OVERRIDE;

  virtual bool SchedulePageFlip(gfx::AcceleratedWidget w);

  virtual SkCanvas* GetCanvasForWidget(gfx::AcceleratedWidget w);

  virtual scoped_ptr<gfx::VSyncProvider> CreateVSyncProvider(
      gfx::AcceleratedWidget w);

  void SetHardwareCursor(gfx::AcceleratedWidget window,
                         const SkBitmap& image,
                         const gfx::Point& location);

  void MoveHardwareCursor(gfx::AcceleratedWidget window,
                          const gfx::Point& location);

  void UnsetHardwareCursor(gfx::AcceleratedWidget window);

 private:
  virtual DriSurface* CreateSurface(const gfx::Size& size);

  virtual DriWrapper* CreateWrapper();

  virtual bool InitializeControllerForPrimaryDisplay(
    DriWrapper* drm,
    HardwareDisplayController* controller);

  // Blocks until a DRM event is read.
  // TODO(dnicoara) Remove once we can safely move DRM event processing in the
  // message loop while correctly signaling when we're done displaying the
  // pending frame.
  virtual void WaitForPageFlipEvent(int fd);

  // Draw the last set cursor & update the cursor plane.
  void ResetCursor();

  scoped_ptr<DriWrapper> drm_;

  HardwareState state_;

  // Active output.
  scoped_ptr<HardwareDisplayController> controller_;

  scoped_ptr<DriSurface> cursor_surface_;

  SkBitmap cursor_bitmap_;
  gfx::Point cursor_location_;

  DISALLOW_COPY_AND_ASSIGN(DriSurfaceFactory);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRI_DRI_SURFACE_FACTORY_H_
