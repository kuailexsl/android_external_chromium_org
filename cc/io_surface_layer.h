// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef IOSurfaceLayerChromium_h
#define IOSurfaceLayerChromium_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerChromium.h"

namespace cc {

class IOSurfaceLayerChromium : public LayerChromium {
public:
    static scoped_refptr<IOSurfaceLayerChromium> create();

    void setIOSurfaceProperties(uint32_t ioSurfaceId, const IntSize&);

    virtual scoped_ptr<CCLayerImpl> createCCLayerImpl() OVERRIDE;
    virtual bool drawsContent() const OVERRIDE;
    virtual void pushPropertiesTo(CCLayerImpl*) OVERRIDE;

protected:
    IOSurfaceLayerChromium();

private:
    virtual ~IOSurfaceLayerChromium();

    uint32_t m_ioSurfaceId;
    IntSize m_ioSurfaceSize;
};

}
#endif // USE(ACCELERATED_COMPOSITING)

#endif
