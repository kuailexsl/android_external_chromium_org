// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_buffer.h"

#include <cstring>

#include "base/logging.h"
#include "net/base/io_buffer.h"
#include "net/spdy/spdy_protocol.h"

namespace net {

namespace {

// Makes a SpdyFrame with |size| bytes of data copied from
// |data|. |data| must be non-NULL and |size| must be positive.
scoped_ptr<SpdyFrame> MakeSpdyFrame(const char* data, size_t size) {
  DCHECK(data);
  DCHECK_GT(size, 0u);
  scoped_ptr<char[]> frame_data(new char[size]);
  std::memcpy(frame_data.get(), data, size);
  scoped_ptr<SpdyFrame> frame(
      new SpdyFrame(frame_data.release(), size, true /* owns_buffer */));
  return frame.Pass();
}

}  // namespace

SpdyBuffer::SpdyBuffer(scoped_ptr<SpdyFrame> frame)
    : frame_(frame.Pass()),
      offset_(0) {}

// The given data may not be strictly a SPDY frame; we (ab)use
// |frame_| just as a container.
SpdyBuffer::SpdyBuffer(const char* data, size_t size) :
    frame_(MakeSpdyFrame(data, size)),
    offset_(0) {}

SpdyBuffer::~SpdyBuffer() {}

const char* SpdyBuffer::GetRemainingData() const {
  return frame_->data() + offset_;
}

size_t SpdyBuffer::GetRemainingSize() const {
  return frame_->size() - offset_;
}

void SpdyBuffer::Consume(size_t consume_size) {
  DCHECK_GE(consume_size, 1u);
  DCHECK_LE(consume_size, GetRemainingSize());
  offset_ += consume_size;
};

IOBuffer* SpdyBuffer::GetIOBufferForRemainingData() {
  return new WrappedIOBuffer(GetRemainingData());
}

}  // namespace net
