// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/test/simple_test_tick_clock.h"
#include "media/cast/audio_receiver/audio_receiver.h"
#include "media/cast/cast_defines.h"
#include "media/cast/cast_environment.h"
#include "media/cast/pacing/mock_paced_packet_sender.h"
#include "media/cast/test/fake_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
namespace cast {

static const int kPacketSize = 1500;

class TestAudioEncoderCallback :
    public base::RefCountedThreadSafe<TestAudioEncoderCallback>  {
 public:
  TestAudioEncoderCallback()
      : num_called_(0) {}

  void SetExpectedResult(uint8 expected_frame_id,
                         const base::TimeTicks& expected_playout_time) {
    expected_frame_id_ = expected_frame_id;
    expected_playout_time_ = expected_playout_time;
  }

  void DeliverEncodedAudioFrame(scoped_ptr<EncodedAudioFrame> audio_frame,
                                const base::TimeTicks& playout_time) {
    EXPECT_EQ(0, audio_frame->frame_id);
    EXPECT_EQ(kPcm16, audio_frame->codec);
    EXPECT_EQ(expected_playout_time_, playout_time);
    num_called_++;
  }

  int number_times_called() { return num_called_;}

 private:
  int num_called_;
  uint8 expected_frame_id_;
  base::TimeTicks expected_playout_time_;
};

class PeerAudioReceiver : public AudioReceiver {
 public:
  PeerAudioReceiver(scoped_refptr<CastEnvironment> cast_environment,
                    const AudioReceiverConfig& audio_config,
                    PacedPacketSender* const packet_sender)
      : AudioReceiver(cast_environment, audio_config, packet_sender) {}

  using AudioReceiver::IncomingParsedRtpPacket;
};

class AudioReceiverTest : public ::testing::Test {
 protected:
  AudioReceiverTest() {
    // Configure the audio receiver to use PCM16.
    audio_config_.rtp_payload_type = 127;
    audio_config_.frequency = 16000;
    audio_config_.channels = 1;
    audio_config_.codec = kPcm16;
    audio_config_.use_external_decoder = false;
    task_runner_ = new test::FakeTaskRunner(&testing_clock_);
    cast_environment_ = new CastEnvironment(task_runner_, task_runner_,
        task_runner_, task_runner_, task_runner_);
    test_audio_encoder_callback_ = new TestAudioEncoderCallback();
  }

  void Configure(bool use_external_decoder) {
    audio_config_.use_external_decoder = use_external_decoder;
    receiver_.reset(new PeerAudioReceiver(cast_environment_, audio_config_,
                                          &mock_transport_));
  }

  virtual ~AudioReceiverTest() {}

  virtual void SetUp() {
    payload_.assign(kPacketSize, 0);
    rtp_header_.is_key_frame = true;
    rtp_header_.frame_id = 0;
    rtp_header_.packet_id = 0;
    rtp_header_.max_packet_id = 0;
    rtp_header_.is_reference = false;
    rtp_header_.reference_frame_id = 0;
    rtp_header_.webrtc.header.timestamp = 0;
  }

  AudioReceiverConfig audio_config_;
  std::vector<uint8> payload_;
  RtpCastHeader rtp_header_;
  base::SimpleTestTickClock testing_clock_;
  MockPacedPacketSender mock_transport_;
  scoped_refptr<test::FakeTaskRunner> task_runner_;
  scoped_ptr<PeerAudioReceiver> receiver_;
  scoped_refptr<CastEnvironment> cast_environment_;
  scoped_refptr<TestAudioEncoderCallback> test_audio_encoder_callback_;
};

TEST_F(AudioReceiverTest, GetOnePacketEncodedframe) {
  Configure(true);
  receiver_->IncomingParsedRtpPacket(
      payload_.data(), payload_.size(), rtp_header_);
  EncodedAudioFrame audio_frame;
  base::TimeTicks playout_time;
  test_audio_encoder_callback_->SetExpectedResult(0, testing_clock_.NowTicks());

  AudioFrameEncodedCallback frame_encoded_callback =
      base::Bind(&TestAudioEncoderCallback::DeliverEncodedAudioFrame,
                 test_audio_encoder_callback_.get());

  receiver_->GetEncodedAudioFrame(frame_encoded_callback);
  task_runner_->RunTasks();
  EXPECT_EQ(1, test_audio_encoder_callback_->number_times_called());
}

// TODO(mikhal): Add encoded frames.
TEST_F(AudioReceiverTest, GetRawFrame) {
}

}  // namespace cast
}  // namespace media
