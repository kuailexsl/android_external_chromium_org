// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/profile_sync_test_util.h"

#include "base/bind.h"
#include "base/task.h"
#include "base/threading/thread.h"

using content::BrowserThread;

ProfileSyncServiceObserverMock::ProfileSyncServiceObserverMock() {}

ProfileSyncServiceObserverMock::~ProfileSyncServiceObserverMock() {}

ThreadNotificationService::ThreadNotificationService(
    base::Thread* notification_thread)
    : done_event_(false, false),
      notification_thread_(notification_thread) {}

void ThreadNotificationService::Init() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  notification_thread_->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&ThreadNotificationService::InitTask, this));
  done_event_.Wait();
}

void ThreadNotificationService::TearDown() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  notification_thread_->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&ThreadNotificationService::TearDownTask, this));
  done_event_.Wait();
}

ThreadNotificationService::~ThreadNotificationService() {}

void ThreadNotificationService::InitTask() {
  service_.reset(content::NotificationService::Create());
  done_event_.Signal();
}

void ThreadNotificationService::TearDownTask() {
  service_.reset(NULL);
  done_event_.Signal();
}

ThreadNotifier::ThreadNotifier(base::Thread* notify_thread)
    : done_event_(false, false),
      notify_thread_(notify_thread) {}

void ThreadNotifier::Notify(int type,
                            const content::NotificationDetails& details) {
  Notify(type, content::NotificationService::AllSources(), details);
}

void ThreadNotifier::Notify(int type,
                            const content::NotificationSource& source,
                            const content::NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  notify_thread_->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&ThreadNotifier::NotifyTask, this, type, source, details));
  done_event_.Wait();
}

ThreadNotifier::~ThreadNotifier() {}

void ThreadNotifier::NotifyTask(int type,
                                const content::NotificationSource& source,
                                const content::NotificationDetails& details) {
  content::NotificationService::current()->Notify(type, source, details);
  done_event_.Signal();
}
