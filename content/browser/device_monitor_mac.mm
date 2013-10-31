// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/device_monitor_mac.h"

#import <QTKit/QTKit.h>

#include "base/logging.h"
#import "media/video/capture/mac/avfoundation_glue.h"

namespace {

// This class is used to keep track of system devices names and their types.
class DeviceInfo {
 public:
  enum DeviceType {
    kAudio,
    kVideo,
    kMuxed,
    kUnknown
  };

  DeviceInfo(std::string unique_id, DeviceType type)
      : unique_id_(unique_id), type_(type) {}

  // Operator== is needed here to use this class in a std::find. A given
  // |unique_id_| always has the same |type_| so for comparison purposes the
  // latter can be safely ignored.
  bool operator==(const DeviceInfo& device) const {
    return unique_id_ == device.unique_id_;
  }

  const std::string& unique_id() const { return unique_id_; }
  DeviceType type() const { return type_; }

 private:
  std::string unique_id_;
  DeviceType type_;
  // Allow generated copy constructor and assignment.
};

// Base abstract class used by DeviceMonitorMac to interact with either a QTKit
// or an AVFoundation implementation of events and notifications.
class DeviceMonitorMacImpl {
 public:
  explicit DeviceMonitorMacImpl(content::DeviceMonitorMac* monitor)
      : monitor_(monitor),
        cached_devices_(),
        device_arrival_(nil),
        device_removal_(nil) {
    DCHECK(monitor);
  }
  virtual ~DeviceMonitorMacImpl() {}

  virtual void OnDeviceChanged() = 0;

  // Method called by the default notification center when a device is removed
  // or added to the system. It will compare the |cached_devices_| with the
  // current situation, update it, and, if there's an update, signal to
  // |monitor_| with the appropriate device type.
  void ConsolidateDevicesListAndNotify(
      const std::vector<DeviceInfo>& snapshot_devices);

 protected:
  content::DeviceMonitorMac* monitor_;
  std::vector<DeviceInfo> cached_devices_;

  // Handles to NSNotificationCenter block observers.
  id device_arrival_;
  id device_removal_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceMonitorMacImpl);
};

void DeviceMonitorMacImpl::ConsolidateDevicesListAndNotify(
    const std::vector<DeviceInfo>& snapshot_devices) {
  bool video_device_added = false;
  bool audio_device_added = false;
  bool video_device_removed = false;
  bool audio_device_removed = false;

  // Compare the current system devices snapshot with the ones cached to detect
  // additions, present in the former but not in the latter. If we find a device
  // in snapshot_devices entry also present in cached_devices, we remove it from
  // the latter vector.
  std::vector<DeviceInfo>::const_iterator it;
  for (it = snapshot_devices.begin(); it != snapshot_devices.end(); ++it) {
    std::vector<DeviceInfo>::iterator cached_devices_iterator =
        std::find(cached_devices_.begin(), cached_devices_.end(), *it);
    if (cached_devices_iterator == cached_devices_.end()) {
      video_device_added |= ((it->type() == DeviceInfo::kVideo) ||
                             (it->type() == DeviceInfo::kMuxed));
      audio_device_added |= ((it->type() == DeviceInfo::kAudio) ||
                             (it->type() == DeviceInfo::kMuxed));
      DVLOG(1) << "Device has been added, id: " << it->unique_id();
    } else {
      cached_devices_.erase(cached_devices_iterator);
    }
  }
  // All the remaining entries in cached_devices are removed devices.
  for (it = cached_devices_.begin(); it != cached_devices_.end(); ++it) {
    video_device_removed |= ((it->type() == DeviceInfo::kVideo) ||
                             (it->type() == DeviceInfo::kMuxed));
    audio_device_removed |= ((it->type() == DeviceInfo::kAudio) ||
                             (it->type() == DeviceInfo::kMuxed));
    DVLOG(1) << "Device has been removed, id: " << it->unique_id();
  }
  // Update the cached devices with the current system snapshot.
  cached_devices_ = snapshot_devices;

  if (video_device_added || video_device_removed)
    monitor_->NotifyDeviceChanged(base::SystemMonitor::DEVTYPE_VIDEO_CAPTURE);
  if (audio_device_added || video_device_removed)
    monitor_->NotifyDeviceChanged(base::SystemMonitor::DEVTYPE_AUDIO_CAPTURE);
}

class QTKitMonitorImpl : public DeviceMonitorMacImpl {
 public:
  explicit QTKitMonitorImpl(content::DeviceMonitorMac* monitor);
  virtual ~QTKitMonitorImpl();

  virtual void OnDeviceChanged() OVERRIDE;
};

QTKitMonitorImpl::QTKitMonitorImpl(content::DeviceMonitorMac* monitor)
    : DeviceMonitorMacImpl(monitor) {
  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
  device_arrival_ =
      [nc addObserverForName:QTCaptureDeviceWasConnectedNotification
                      object:nil
                       queue:nil
                  usingBlock:^(NSNotification* notification) {
                      OnDeviceChanged();
                  }];

  device_removal_ =
      [nc addObserverForName:QTCaptureDeviceWasDisconnectedNotification
                      object:nil
                       queue:nil
                  usingBlock:^(NSNotification* notification) {
                      OnDeviceChanged();
                  }];
}

QTKitMonitorImpl::~QTKitMonitorImpl() {
  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:device_arrival_];
  [nc removeObserver:device_removal_];
}

void QTKitMonitorImpl::OnDeviceChanged() {
  std::vector<DeviceInfo> snapshot_devices;

  NSArray* devices = [QTCaptureDevice inputDevices];
  for (QTCaptureDevice* device in devices) {
    DeviceInfo::DeviceType device_type = DeviceInfo::kUnknown;
    if ([device hasMediaType:QTMediaTypeVideo])
      device_type = DeviceInfo::kVideo;
    else if ([device hasMediaType:QTMediaTypeMuxed])
      device_type = DeviceInfo::kMuxed;
    else if ([device hasMediaType:QTMediaTypeSound])
      device_type = DeviceInfo::kAudio;

    snapshot_devices.push_back(
        DeviceInfo([[device uniqueID] UTF8String], device_type));
  }

  ConsolidateDevicesListAndNotify(snapshot_devices);
}

class AVFoundationMonitorImpl : public DeviceMonitorMacImpl {
 public:
  explicit AVFoundationMonitorImpl(content::DeviceMonitorMac* monitor);
  virtual ~AVFoundationMonitorImpl();

  virtual void OnDeviceChanged() OVERRIDE;
};

AVFoundationMonitorImpl::AVFoundationMonitorImpl(
    content::DeviceMonitorMac* monitor)
    : DeviceMonitorMacImpl(monitor) {
  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

  device_arrival_ =
      [nc addObserverForName:AVFoundationGlue::
          AVCaptureDeviceWasConnectedNotification()
                      object:nil
                       queue:nil
                  usingBlock:^(NSNotification* notification) {
                      OnDeviceChanged();
                  }];
  device_removal_ =
      [nc addObserverForName:AVFoundationGlue::
          AVCaptureDeviceWasDisconnectedNotification()
                      object:nil
                       queue:nil
                  usingBlock:^(NSNotification* notification) {
                      OnDeviceChanged();
                  }];
}

AVFoundationMonitorImpl::~AVFoundationMonitorImpl() {
  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
  [nc removeObserver:device_arrival_];
  [nc removeObserver:device_removal_];
}

void AVFoundationMonitorImpl::OnDeviceChanged() {
  std::vector<DeviceInfo> snapshot_devices;

  NSArray* devices = [AVCaptureDeviceGlue devices];
  for (CrAVCaptureDevice* device in devices) {
    DeviceInfo::DeviceType device_type = DeviceInfo::kUnknown;
    if ([AVCaptureDeviceGlue hasMediaType:AVFoundationGlue::AVMediaTypeVideo()
                         forCaptureDevice:device]) {
      device_type = DeviceInfo::kVideo;
    } else if ([AVCaptureDeviceGlue
                   hasMediaType:AVFoundationGlue::AVMediaTypeMuxed()
               forCaptureDevice:device]) {
      device_type = DeviceInfo::kMuxed;
    } else if ([AVCaptureDeviceGlue
                   hasMediaType:AVFoundationGlue::AVMediaTypeAudio()
               forCaptureDevice:device]) {
      device_type = DeviceInfo::kAudio;
    }
    snapshot_devices.push_back(DeviceInfo(
        [[AVCaptureDeviceGlue uniqueID:device] UTF8String], device_type));
  }

  ConsolidateDevicesListAndNotify(snapshot_devices);
}

}  // namespace

namespace content {

DeviceMonitorMac::DeviceMonitorMac() {
  if (AVFoundationGlue::IsAVFoundationSupported()) {
    DVLOG(1) << "Monitoring via AVFoundation";
    device_monitor_impl_.reset(new AVFoundationMonitorImpl(this));
  } else {
    DVLOG(1) << "Monitoring via QTKit";
    device_monitor_impl_.reset(new QTKitMonitorImpl(this));
  }
  // Force device enumeration to correctly list those already in the system.
  device_monitor_impl_->OnDeviceChanged();
}

DeviceMonitorMac::~DeviceMonitorMac() {}

void DeviceMonitorMac::NotifyDeviceChanged(
    base::SystemMonitor::DeviceType type) {
  // TODO(xians): Remove the global variable for SystemMonitor.
  base::SystemMonitor::Get()->ProcessDevicesChanged(type);
}

}  // namespace content
