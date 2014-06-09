// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Audio test utilities.

// GetStats reports audio output energy in the [0, 32768] range.
var MAX_AUDIO_OUTPUT_ENERGY = 32768;

// Gathers |numSamples| samples at |frequency| number of times per second and
// calls back |callback| with an array with numbers in the [0, 32768] range.
function gatherAudioLevelSamples(peerConnection, numSamples, frequency,
                                 callback) {
  console.log('Gathering ' + numSamples + ' audio samples...');
  var audioLevelSamples = []
  var gatherSamples = setInterval(function() {
    peerConnection.getStats(function(response) {
      audioOutputLevels = getAudioLevelFromStats_(response);
      if (audioOutputLevels.length == 0) {
        // The call probably isn't up yet.
        return;
      }

      // If more than one audio level is reported we get confused.
      assertEquals(1, audioOutputLevels.length);
      audioLevelSamples.push(audioOutputLevels[0]);

      if (audioLevelSamples.length == numSamples) {
        console.log('Gathered all samples.');
        clearInterval(gatherSamples);
        callback(audioLevelSamples);
      }
    });
  }, 1000 / frequency);
}

// Tries to identify the beep-every-half-second signal generated by the fake
// audio device in media/video/capture/fake_video_capture_device.cc. Fails the
// test if we can't see a signal. The samples should have been gathered over at
// least two seconds since we expect to see at least three "peaks" in there
// (we should see either 3 or 4 depending on how things line up).
//
// If |beLenient| is specified, we assume we're running on a slow device or
// or under TSAN, and relax the checks quite a bit.
function verifyAudioIsPlaying(samples, beLenient) {
  var numPeaks = 0;
  var threshold = MAX_AUDIO_OUTPUT_ENERGY * 0.7;
  if (beLenient)
    threshold = MAX_AUDIO_OUTPUT_ENERGY * 0.6;
  var currentlyOverThreshold = false;

  // Detect when we have been been over the threshold and is going back again
  // (i.e. count peaks). We should see about one peak per second.
  for (var i = 0; i < samples.length; ++i) {
    if (currentlyOverThreshold && samples[i] < threshold)
      numPeaks++;
    currentlyOverThreshold = samples[i] >= threshold;
  }

  console.log('Number of peaks identified: ' + numPeaks);

  var expectedPeaks = 2;
  if (beLenient)
    expectedPeaks = 1;

  if (numPeaks < expectedPeaks)
    failTest('Expected to see at least ' + expectedPeaks + ' peak(s) in ' +
        'audio signal, got ' + numPeaks + '. Dumping samples for analysis: "' +
        samples + '"');
}

// If silent (like when muted), we should get very near zero audio level.
function verifyIsSilent(samples) {
  var average = 0;
  for (var i = 0; i < samples.length; ++i)
    average += samples[i] / samples.length;

  console.log('Average audio level: ' + average);
  if (average > 500)
    failTest('Expected silence, but avg audio level was ' + average);
}

/**
 * @private
 */
function getAudioLevelFromStats_(response) {
  var reports = response.result();
  var audioOutputLevels = [];
  for (var i = 0; i < reports.length; ++i) {
    var report = reports[i];
    if (report.names().indexOf('audioOutputLevel') != -1) {
      audioOutputLevels.push(report.stat('audioOutputLevel'));
    }
  }
  return audioOutputLevels;
}
