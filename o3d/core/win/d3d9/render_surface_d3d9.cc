/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "core/cross/precompile.h"
#include "core/win/d3d9/render_surface_d3d9.h"
#include "core/win/d3d9/utils_d3d9.h"
#include "core/win/d3d9/renderer_d3d9.h"
#include "core/cross/error.h"

namespace o3d {

RenderSurfaceD3D9::RenderSurfaceD3D9(ServiceLocator *service_locator,
                                     int width,
                                     int height,
                                     Texture *texture,
                                     SurfaceConstructor *surface_constructor)
    : RenderSurface(service_locator, width, height, texture),
      direct3d_surface_(NULL),
      surface_constructor_(surface_constructor) {
  DCHECK(surface_constructor);
  DCHECK(texture);
  // TODO: Won't this crash if it can not create the surface?
  HR(surface_constructor_->ConstructSurface(&direct3d_surface_));
  Clear();
}

bool RenderSurfaceD3D9::OnLostDevice() {
  direct3d_surface_ = NULL;
  return true;
}

bool RenderSurfaceD3D9::OnResetDevice() {
  // Reconstruct the surface from the construction object provided by the
  // owning texture.
  HR(surface_constructor_->ConstructSurface(&direct3d_surface_));
  Clear();
  return direct3d_surface_ != NULL;
}

void RenderSurfaceD3D9::Clear() {
  RendererD3D9* renderer =
      down_cast<RendererD3D9*>(service_locator()->GetService<Renderer>());
  if (direct3d_surface_) {
    renderer->d3d_device()->ColorFill(
        direct3d_surface_, NULL, D3DCOLOR_RGBA(0, 0, 0, 0));
  }
}

bool RenderSurfaceD3D9::PlatformSpecificGetIntoBitmap(
    Bitmap::Ref bitmap) const {
  DCHECK(bitmap->width() == static_cast<unsigned int>(clip_width()) &&
         bitmap->height() == static_cast<unsigned int>(clip_height()) &&
         bitmap->num_mipmaps() == 1 &&
         bitmap->format() == Texture::ARGB8);

  if (!direct3d_surface_) {
    return false;
  }

  RendererD3D9* renderer =
      down_cast<RendererD3D9*>(service_locator()->GetService<Renderer>());
  LPDIRECT3DDEVICE9 device = renderer->d3d_device();
  CComPtr<IDirect3DSurface9> system_surface;

  D3DSURFACE_DESC surface_description;
  if (!HR(direct3d_surface_->GetDesc(&surface_description))) {
    return false;
  }

  if (!HR(device->CreateOffscreenPlainSurface(surface_description.Width,
                                              surface_description.Height,
                                              surface_description.Format,
                                              D3DPOOL_SYSTEMMEM,
                                              &system_surface,
                                              NULL))) {
    return false;
  }

  if (!HR(device->GetRenderTargetData(direct3d_surface_, system_surface)))
    return false;

  RECT rect = { 0, 0, clip_width(), clip_height() };
  D3DLOCKED_RECT out_rect = {0};
  if (!HR(system_surface->LockRect(&out_rect, &rect, D3DLOCK_READONLY))) {
    O3D_ERROR(service_locator()) << "Failed to Lock Surface (D3D9)";
    return false;
  }

  bitmap->SetRect(0, 0, 0,
                  clip_width(),
                  clip_height(),
                  out_rect.pBits,
                  out_rect.Pitch);

  system_surface->UnlockRect();

  return true;
}

RenderDepthStencilSurfaceD3D9::RenderDepthStencilSurfaceD3D9(
    ServiceLocator *service_locator,
    int width,
    int height,
    SurfaceConstructor *surface_constructor)
    : RenderDepthStencilSurface(service_locator, width, height),
      direct3d_surface_(NULL),
      surface_constructor_(surface_constructor) {
  DCHECK(surface_constructor);
  HR(surface_constructor_->ConstructSurface(&direct3d_surface_));
}

bool RenderDepthStencilSurfaceD3D9::OnLostDevice() {
  direct3d_surface_ = NULL;
  return true;
}

bool RenderDepthStencilSurfaceD3D9::OnResetDevice() {
  // Reconstruct the surface from the construction object provided by the
  // owning texture.
  HR(surface_constructor_->ConstructSurface(&direct3d_surface_));
  return direct3d_surface_ != NULL;
}

}  // namespace o3d
