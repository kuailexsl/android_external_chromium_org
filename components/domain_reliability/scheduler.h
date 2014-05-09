// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOMAIN_RELIABILITY_SCHEDULER_H_
#define COMPONENTS_DOMAIN_RELIABILITY_SCHEDULER_H_

#include <vector>

#include "base/callback.h"
#include "base/time/time.h"
#include "components/domain_reliability/domain_reliability_export.h"

namespace domain_reliability {

class DomainReliabilityConfig;
class MockableTime;

// Determines when an upload should be scheduled. A domain's config will
// specify minimum and maximum upload delays; the minimum upload delay ensures
// that Chrome will not send too many upload requests to a site by waiting at
// least that long after the first beacon, while the maximum upload delay makes
// sure the server receives the reports while they are still fresh.
//
// When everything is working fine, the scheduler will return precisely that
// interval. If all uploaders have failed, then the beginning or ending points
// of the interval may be pushed later to accomodate the retry with exponential
// backoff.
//
// See dispatcher.h for an explanation of what happens with the scheduled
// interval.
class DOMAIN_RELIABILITY_EXPORT DomainReliabilityScheduler {
 public:
  typedef base::Callback<void(base::TimeDelta, base::TimeDelta)>
      ScheduleUploadCallback;

  struct Params {
   public:
    base::TimeDelta minimum_upload_delay;
    base::TimeDelta maximum_upload_delay;
    base::TimeDelta upload_retry_interval;

    static Params GetFromFieldTrialsOrDefaults();
  };

  DomainReliabilityScheduler(MockableTime* time,
                             size_t num_collectors,
                             const Params& params,
                             const ScheduleUploadCallback& callback);
  ~DomainReliabilityScheduler();

  // If there is no upload pending, schedules an upload based on the provided
  // parameters (some time between the minimum and maximum delay from now).
  // May call the ScheduleUploadCallback.
  void OnBeaconAdded();

  // Returns which collector to use for an upload that is about to start. Must
  // be called exactly once during or after the ScheduleUploadCallback but
  // before OnUploadComplete is called. (Also records the upload start time for
  // future retries, if the upload ends up failing.)
  size_t OnUploadStart();

  // Updates the scheduler state based on the result of an upload. Must be
  // called exactly once after |OnUploadStart|. |success| should be true if the
  // upload was successful, and false otherwise.
  void OnUploadComplete(bool success);

 private:
  struct CollectorState {
    CollectorState();

    // The number of consecutive failures to upload to this collector, or 0 if
    // the most recent upload succeeded.
    unsigned failures;
    base::TimeTicks next_upload;
  };

  void MaybeScheduleUpload();

  void GetNextUploadTimeAndCollector(base::TimeTicks now,
                                     base::TimeTicks* upload_time_out,
                                     size_t* collector_index_out);

  base::TimeDelta GetUploadRetryInterval(unsigned failures);

  MockableTime* time_;
  std::vector<CollectorState> collectors_;
  Params params_;
  ScheduleUploadCallback callback_;

  // Whether there are beacons that have not yet been uploaded. Set when a
  // beacon arrives or an upload fails, and cleared when an upload starts.
  bool upload_pending_;

  // Whether the scheduler has called the ScheduleUploadCallback to schedule
  // the next upload. Set when an upload is scheduled and cleared when the
  // upload starts.
  bool upload_scheduled_;

  // Whether the last scheduled upload is in progress. Set when the upload
  // starts and cleared when the upload completes (successfully or not).
  bool upload_running_;

  // Index of the collector selected for the next upload.  (Set in
  // |OnUploadStart| and cleared in |OnUploadComplete|.)
  size_t collector_index_;

  // Time of the first beacon that was not included in the last successful
  // upload.
  base::TimeTicks first_beacon_time_;

  // first_beacon_time_ saved during uploads.  Restored if upload fails.
  base::TimeTicks old_first_beacon_time_;
};

}  // namespace domain_reliability

#endif  // COMPONENTS_DOMAIN_RELIABILITY_SCHEDULER_H_
