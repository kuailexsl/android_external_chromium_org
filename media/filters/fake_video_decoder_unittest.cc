// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/mock_filters.h"
#include "media/base/test_helpers.h"
#include "media/base/video_frame.h"
#include "media/filters/fake_video_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

static const int kDecodingDelay = 9;
static const int kTotalBuffers = 12;
static const int kDurationMs = 30;

class FakeVideoDecoderTest : public testing::Test,
                             public testing::WithParamInterface<int> {
 public:
  FakeVideoDecoderTest()
      : decoder_(new FakeVideoDecoder(kDecodingDelay, false, GetParam())),
        num_input_buffers_(0),
        num_decoded_frames_(0),
        last_decode_status_(VideoDecoder::kNotEnoughData),
        pending_decode_requests_(0),
        is_reset_pending_(false) {}

  virtual ~FakeVideoDecoderTest() {
    Stop();
  }

  void InitializeWithConfig(const VideoDecoderConfig& config) {
    decoder_->Initialize(config, false, NewExpectedStatusCB(PIPELINE_OK));
    message_loop_.RunUntilIdle();
    current_config_ = config;
  }

  void Initialize() {
    InitializeWithConfig(TestVideoConfig::Normal());
  }

  void EnterPendingInitState() {
    decoder_->HoldNextInit();
    Initialize();
  }

  void SatisfyInit() {
    decoder_->SatisfyInit();
    message_loop_.RunUntilIdle();
  }

  // Callback for VideoDecoder::Read().
  void FrameReady(VideoDecoder::Status status,
                  const scoped_refptr<VideoFrame>& frame) {
    DCHECK_GT(pending_decode_requests_, 0);

    --pending_decode_requests_;
    last_decode_status_ = status;
    last_decoded_frame_ = frame;

    if (frame && !frame->end_of_stream())
      num_decoded_frames_++;
  }

  enum CallbackResult {
    PENDING,
    OK,
    NOT_ENOUGH_DATA,
    ABORTED,
    EOS
  };

  void ExpectReadResult(CallbackResult result) {
    switch (result) {
      case PENDING:
        EXPECT_GT(pending_decode_requests_, 0);
        break;
      case OK:
        EXPECT_EQ(0, pending_decode_requests_);
        ASSERT_EQ(VideoDecoder::kOk, last_decode_status_);
        ASSERT_TRUE(last_decoded_frame_);
        EXPECT_FALSE(last_decoded_frame_->end_of_stream());
        break;
      case NOT_ENOUGH_DATA:
        EXPECT_EQ(0, pending_decode_requests_);
        ASSERT_EQ(VideoDecoder::kNotEnoughData, last_decode_status_);
        ASSERT_FALSE(last_decoded_frame_);
        break;
      case ABORTED:
        EXPECT_EQ(0, pending_decode_requests_);
        ASSERT_EQ(VideoDecoder::kAborted, last_decode_status_);
        EXPECT_FALSE(last_decoded_frame_);
        break;
      case EOS:
        EXPECT_EQ(0, pending_decode_requests_);
        ASSERT_EQ(VideoDecoder::kOk, last_decode_status_);
        ASSERT_TRUE(last_decoded_frame_);
        EXPECT_TRUE(last_decoded_frame_->end_of_stream());
        break;
    }
  }

  void Decode() {
    scoped_refptr<DecoderBuffer> buffer;

    if (num_input_buffers_ < kTotalBuffers) {
      buffer = CreateFakeVideoBufferForTest(
          current_config_,
          base::TimeDelta::FromMilliseconds(kDurationMs * num_input_buffers_),
          base::TimeDelta::FromMilliseconds(kDurationMs));
      num_input_buffers_++;
    } else {
      buffer = DecoderBuffer::CreateEOSBuffer();
    }

    ++pending_decode_requests_;

    decoder_->Decode(
        buffer,
        base::Bind(&FakeVideoDecoderTest::FrameReady, base::Unretained(this)));
    message_loop_.RunUntilIdle();
  }

  void ReadOneFrame() {
    do {
      Decode();
    } while (last_decode_status_ == VideoDecoder::kNotEnoughData &&
             pending_decode_requests_ == 0);
  }

  void ReadUntilEOS() {
    do {
      ReadOneFrame();
    } while (last_decoded_frame_ && !last_decoded_frame_->end_of_stream());
  }

  void EnterPendingReadState() {
    // Pass the initial NOT_ENOUGH_DATA stage.
    ReadOneFrame();
    decoder_->HoldDecode();
    ReadOneFrame();
    ExpectReadResult(PENDING);
  }

  void SatisfyReadAndExpect(CallbackResult result) {
    decoder_->SatisfyDecode();
    message_loop_.RunUntilIdle();
    ExpectReadResult(result);
  }

  void SatisfyRead() {
    SatisfyReadAndExpect(OK);
  }

  // Callback for VideoDecoder::Reset().
  void OnDecoderReset() {
    DCHECK(is_reset_pending_);
    is_reset_pending_ = false;
  }

  void ExpectResetResult(CallbackResult result) {
    switch (result) {
      case PENDING:
        EXPECT_TRUE(is_reset_pending_);
        break;
      case OK:
        EXPECT_FALSE(is_reset_pending_);
        break;
      default:
        NOTREACHED();
    }
  }

  void ResetAndExpect(CallbackResult result) {
    is_reset_pending_ = true;
    decoder_->Reset(base::Bind(&FakeVideoDecoderTest::OnDecoderReset,
                               base::Unretained(this)));
    message_loop_.RunUntilIdle();
    ExpectResetResult(result);
  }

  void EnterPendingResetState() {
    decoder_->HoldNextReset();
    ResetAndExpect(PENDING);
  }

  void SatisfyReset() {
    decoder_->SatisfyReset();
    message_loop_.RunUntilIdle();
    ExpectResetResult(OK);
  }

  void Stop() {
    decoder_->Stop();
    message_loop_.RunUntilIdle();

    // All pending callbacks must have been fired.
    DCHECK_EQ(pending_decode_requests_, 0);
    DCHECK(!is_reset_pending_);
  }

  base::MessageLoop message_loop_;
  VideoDecoderConfig current_config_;

  scoped_ptr<FakeVideoDecoder> decoder_;

  int num_input_buffers_;
  int num_decoded_frames_;

  // Callback result/status.
  VideoDecoder::Status last_decode_status_;
  scoped_refptr<VideoFrame> last_decoded_frame_;
  int pending_decode_requests_;
  bool is_reset_pending_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeVideoDecoderTest);
};

INSTANTIATE_TEST_CASE_P(NoParallelDecode,
                        FakeVideoDecoderTest,
                        ::testing::Values(1));
INSTANTIATE_TEST_CASE_P(ParallelDecode,
                        FakeVideoDecoderTest,
                        ::testing::Values(3));

TEST_P(FakeVideoDecoderTest, Initialize) {
  Initialize();
}

TEST_P(FakeVideoDecoderTest, Read_AllFrames) {
  Initialize();
  ReadUntilEOS();
  EXPECT_EQ(kTotalBuffers, num_decoded_frames_);
}

TEST_P(FakeVideoDecoderTest, Read_DecodingDelay) {
  Initialize();

  while (num_input_buffers_ < kTotalBuffers) {
    ReadOneFrame();
    EXPECT_EQ(num_input_buffers_, num_decoded_frames_ + kDecodingDelay);
  }
}

TEST_P(FakeVideoDecoderTest, Read_ZeroDelay) {
  decoder_.reset(new FakeVideoDecoder(0, false, 1));
  Initialize();

  while (num_input_buffers_ < kTotalBuffers) {
    ReadOneFrame();
    EXPECT_EQ(num_input_buffers_, num_decoded_frames_);
  }
}

TEST_P(FakeVideoDecoderTest, Read_Pending_NotEnoughData) {
  Initialize();
  decoder_->HoldDecode();
  ReadOneFrame();
  ExpectReadResult(PENDING);
  SatisfyReadAndExpect(NOT_ENOUGH_DATA);
}

TEST_P(FakeVideoDecoderTest, Read_Pending_OK) {
  Initialize();
  ReadOneFrame();
  EnterPendingReadState();
  SatisfyReadAndExpect(OK);
}

TEST_P(FakeVideoDecoderTest, Read_Parallel) {
  int max_decode_requests = GetParam();
  if (max_decode_requests < 2)
    return;

  Initialize();
  ReadOneFrame();
  decoder_->HoldDecode();
  for (int i = 0; i < max_decode_requests; ++i) {
    ReadOneFrame();
    ExpectReadResult(PENDING);
  }
  EXPECT_EQ(max_decode_requests, pending_decode_requests_);
  SatisfyReadAndExpect(OK);
}

TEST_P(FakeVideoDecoderTest, ReadWithHold_DecodingDelay) {
  Initialize();

  // Hold all decodes and satisfy one decode at a time.
  decoder_->HoldDecode();
  int num_decodes_satisfied = 0;
  while (num_decoded_frames_ == 0) {
    while (pending_decode_requests_ < decoder_->GetMaxDecodeRequests())
      Decode();
    decoder_->SatisfySingleDecode();
    ++num_decodes_satisfied;
    message_loop_.RunUntilIdle();
  }

  DCHECK_EQ(num_decoded_frames_, 1);
  DCHECK_EQ(num_decodes_satisfied, kDecodingDelay + 1);
}

TEST_P(FakeVideoDecoderTest, Reinitialize) {
  Initialize();
  ReadOneFrame();
  InitializeWithConfig(TestVideoConfig::Large());
  ReadOneFrame();
}

// Reinitializing the decoder during the middle of the decoding process can
// cause dropped frames.
TEST_P(FakeVideoDecoderTest, Reinitialize_FrameDropped) {
  Initialize();
  ReadOneFrame();
  Initialize();
  ReadUntilEOS();
  EXPECT_LT(num_decoded_frames_, kTotalBuffers);
}

TEST_P(FakeVideoDecoderTest, Reset) {
  Initialize();
  ReadOneFrame();
  ResetAndExpect(OK);
}

TEST_P(FakeVideoDecoderTest, Reset_DuringPendingRead) {
  Initialize();
  EnterPendingReadState();
  ResetAndExpect(PENDING);
  SatisfyReadAndExpect(ABORTED);
}

TEST_P(FakeVideoDecoderTest, Reset_Pending) {
  Initialize();
  EnterPendingResetState();
  SatisfyReset();
}

TEST_P(FakeVideoDecoderTest, Reset_PendingDuringPendingRead) {
  Initialize();
  EnterPendingReadState();
  EnterPendingResetState();
  SatisfyReadAndExpect(ABORTED);
  SatisfyReset();
}

TEST_P(FakeVideoDecoderTest, Stop) {
  Initialize();
  ReadOneFrame();
  ExpectReadResult(OK);
  Stop();
}

TEST_P(FakeVideoDecoderTest, Stop_DuringPendingInitialization) {
  EnterPendingInitState();
  Stop();
}

TEST_P(FakeVideoDecoderTest, Stop_DuringPendingRead) {
  Initialize();
  EnterPendingReadState();
  Stop();
}

TEST_P(FakeVideoDecoderTest, Stop_DuringPendingReset) {
  Initialize();
  EnterPendingResetState();
  Stop();
}

TEST_P(FakeVideoDecoderTest, Stop_DuringPendingReadAndPendingReset) {
  Initialize();
  EnterPendingReadState();
  EnterPendingResetState();
  Stop();
}

TEST_P(FakeVideoDecoderTest, GetDecodeOutput) {
  decoder_.reset(new FakeVideoDecoder(kDecodingDelay, true, 1));
  Initialize();

  while (num_input_buffers_ < kTotalBuffers) {
    ReadOneFrame();
    while (decoder_->GetDecodeOutput())
      ++num_decoded_frames_;
    EXPECT_EQ(num_input_buffers_, num_decoded_frames_);
  }
}

}  // namespace media
