// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/fake_video_decoder.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/message_loop/message_loop_proxy.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/test_helpers.h"

namespace media {

FakeVideoDecoder::FakeVideoDecoder(int decoding_delay,
                                   bool supports_get_decode_output)
    : task_runner_(base::MessageLoopProxy::current()),
      decoding_delay_(decoding_delay),
      supports_get_decode_output_(supports_get_decode_output),
      state_(UNINITIALIZED),
      total_bytes_decoded_(0),
      weak_factory_(this) {
  DCHECK_GE(decoding_delay, 0);
}

FakeVideoDecoder::~FakeVideoDecoder() {
  DCHECK_EQ(state_, UNINITIALIZED);
}

void FakeVideoDecoder::Initialize(const VideoDecoderConfig& config,
                                  bool low_delay,
                                  const PipelineStatusCB& status_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(config.IsValidConfig());
  DCHECK(decode_cb_.IsNull()) << "No reinitialization during pending decode.";
  DCHECK(reset_cb_.IsNull()) << "No reinitialization during pending reset.";

  current_config_ = config;
  init_cb_.SetCallback(BindToCurrentLoop(status_cb));

  if (!decoded_frames_.empty()) {
    DVLOG(1) << "Decoded frames dropped during reinitialization.";
    decoded_frames_.clear();
  }

  state_ = NORMAL;
  init_cb_.RunOrHold(PIPELINE_OK);
}

void FakeVideoDecoder::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                              const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(decode_cb_.IsNull()) << "Overlapping decodes are not supported.";
  DCHECK(reset_cb_.IsNull());
  DCHECK_LE(decoded_frames_.size(), static_cast<size_t>(decoding_delay_));

  int buffer_size = buffer->end_of_stream() ? 0 : buffer->data_size();
  decode_cb_.SetCallback(
      BindToCurrentLoop(base::Bind(&FakeVideoDecoder::OnFrameDecoded,
                                   weak_factory_.GetWeakPtr(),
                                   buffer_size,
                                   decode_cb)));

  if (buffer->end_of_stream() && decoded_frames_.empty()) {
    decode_cb_.RunOrHold(kOk, VideoFrame::CreateEOSFrame());
    return;
  }

  if (!buffer->end_of_stream()) {
    DCHECK(VerifyFakeVideoBufferForTest(buffer, current_config_));
    scoped_refptr<VideoFrame> video_frame = VideoFrame::CreateColorFrame(
        current_config_.coded_size(), 0, 0, 0, buffer->timestamp());
    decoded_frames_.push_back(video_frame);

    if (decoded_frames_.size() <= static_cast<size_t>(decoding_delay_)) {
      decode_cb_.RunOrHold(kNotEnoughData, scoped_refptr<VideoFrame>());
      return;
    }
  }

  scoped_refptr<VideoFrame> frame = decoded_frames_.front();
  decoded_frames_.pop_front();
  decode_cb_.RunOrHold(kOk, frame);
}

void FakeVideoDecoder::Reset(const base::Closure& closure) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(reset_cb_.IsNull());
  reset_cb_.SetCallback(BindToCurrentLoop(closure));

  // Defer the reset if a decode is pending.
  if (!decode_cb_.IsNull())
    return;

  DoReset();
}

void FakeVideoDecoder::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!init_cb_.IsNull())
    SatisfyInit();
  if (!decode_cb_.IsNull())
    SatisfyDecode();
  if (!reset_cb_.IsNull())
    SatisfyReset();

  decoded_frames_.clear();
  state_ = UNINITIALIZED;
}

scoped_refptr<VideoFrame> FakeVideoDecoder::GetDecodeOutput() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (!supports_get_decode_output_ || decoded_frames_.empty())
    return NULL;
  scoped_refptr<VideoFrame> out = decoded_frames_.front();
  decoded_frames_.pop_front();
  return out;
}

void FakeVideoDecoder::HoldNextInit() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  init_cb_.HoldCallback();
}

void FakeVideoDecoder::HoldNextDecode() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  decode_cb_.HoldCallback();
}

void FakeVideoDecoder::HoldNextReset() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  reset_cb_.HoldCallback();
}

void FakeVideoDecoder::SatisfyInit() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(decode_cb_.IsNull());
  DCHECK(reset_cb_.IsNull());

  init_cb_.RunHeldCallback();
}

void FakeVideoDecoder::SatisfyDecode() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  decode_cb_.RunHeldCallback();

  if (!reset_cb_.IsNull())
    DoReset();
}

void FakeVideoDecoder::SatisfyReset() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(decode_cb_.IsNull());
  reset_cb_.RunHeldCallback();
}

void FakeVideoDecoder::DoReset() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(decode_cb_.IsNull());
  DCHECK(!reset_cb_.IsNull());

  decoded_frames_.clear();
  reset_cb_.RunOrHold();
}

void FakeVideoDecoder::OnFrameDecoded(
    int buffer_size,
    const DecodeCB& decode_cb,
    Status status,
    const scoped_refptr<VideoFrame>& video_frame) {
  if (status == kOk || status == kNotEnoughData)
    total_bytes_decoded_ += buffer_size;
  decode_cb.Run(status, video_frame);
}

}  // namespace media
