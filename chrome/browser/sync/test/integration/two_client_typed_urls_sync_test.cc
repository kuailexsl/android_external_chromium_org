// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/number_formatting.h"
#include "base/memory/scoped_vector.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/sync/test/integration/typed_urls_helper.h"

using typed_urls_helper::AddUrlToHistory;
using typed_urls_helper::AddUrlToHistoryWithTransition;
using typed_urls_helper::AssertAllProfilesHaveSameURLsAsVerifier;
using typed_urls_helper::DeleteUrlFromHistory;
using typed_urls_helper::GetTypedUrlsFromClient;

class TwoClientTypedUrlsSyncTest : public SyncTest {
 public:
  TwoClientTypedUrlsSyncTest() : SyncTest(TWO_CLIENT) {}
  virtual ~TwoClientTypedUrlsSyncTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientTypedUrlsSyncTest);
};

// TCM: 3728323
IN_PROC_BROWSER_TEST_F(TwoClientTypedUrlsSyncTest, Add) {
  const string16 kHistoryUrl(
      ASCIIToUTF16("http://www.add-one-history.google.com/"));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Populate one client with a URL, should sync to the other.
  GURL new_url(kHistoryUrl);
  AddUrlToHistory(0, new_url);
  std::vector<history::URLRow> urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(new_url, urls[0].url());

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL.
  AssertAllProfilesHaveSameURLsAsVerifier();
}

// TCM: 3705291
IN_PROC_BROWSER_TEST_F(TwoClientTypedUrlsSyncTest, AddThenDelete) {
  const string16 kHistoryUrl(
      ASCIIToUTF16("http://www.add-one-history.google.com/"));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Populate one client with a URL, should sync to the other.
  GURL new_url(kHistoryUrl);
  AddUrlToHistory(0, new_url);
  std::vector<history::URLRow> urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(new_url, urls[0].url());

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL.
  AssertAllProfilesHaveSameURLsAsVerifier();

  // Delete from first client, should delete from second.
  DeleteUrlFromHistory(0, new_url);

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Neither client should have this URL.
  AssertAllProfilesHaveSameURLsAsVerifier();
}

// TCM: 3643277
IN_PROC_BROWSER_TEST_F(TwoClientTypedUrlsSyncTest, DisableEnableSync) {
  const string16 kUrl1(ASCIIToUTF16("http://history1.google.com/"));
  const string16 kUrl2(ASCIIToUTF16("http://history2.google.com/"));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Disable typed url sync for one client, leave it active for the other.
  GetClient(0)->DisableSyncForDatatype(syncable::TYPED_URLS);

  // Add one URL to non-syncing client, add a different URL to the other,
  // wait for sync cycle to complete. No data should be exchanged.
  GURL url1(kUrl1);
  GURL url2(kUrl2);
  AddUrlToHistory(0, url1);
  AddUrlToHistory(1, url2);
  ASSERT_TRUE(AwaitQuiescence());

  // Make sure that no data was exchanged.
  std::vector<history::URLRow> post_sync_urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, post_sync_urls.size());
  ASSERT_EQ(url1, post_sync_urls[0].url());
  post_sync_urls = GetTypedUrlsFromClient(1);
  ASSERT_EQ(1U, post_sync_urls.size());
  ASSERT_EQ(url2, post_sync_urls[0].url());

  // Enable typed url sync, make both URLs are synced to each client.
  GetClient(0)->EnableSyncForDatatype(syncable::TYPED_URLS);
  ASSERT_TRUE(AwaitQuiescence());

  AssertAllProfilesHaveSameURLsAsVerifier();
}

IN_PROC_BROWSER_TEST_F(TwoClientTypedUrlsSyncTest, AddOneDeleteOther) {
  const string16 kHistoryUrl(
      ASCIIToUTF16("http://www.add-one-delete-history.google.com/"));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Populate one client with a URL, should sync to the other.
  GURL new_url(kHistoryUrl);
  AddUrlToHistory(0, new_url);
  std::vector<history::URLRow> urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(new_url, urls[0].url());

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL.
  AssertAllProfilesHaveSameURLsAsVerifier();

  // Now, delete the URL from the second client.
  DeleteUrlFromHistory(1, new_url);
  urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, urls.size());

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL removed.
  AssertAllProfilesHaveSameURLsAsVerifier();
}

IN_PROC_BROWSER_TEST_F(TwoClientTypedUrlsSyncTest,
                       AddOneDeleteOtherAddAgain) {
  const string16 kHistoryUrl(
      ASCIIToUTF16("http://www.add-delete-add-history.google.com/"));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Populate one client with a URL, should sync to the other.
  GURL new_url(kHistoryUrl);
  AddUrlToHistory(0, new_url);
  std::vector<history::URLRow> urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(new_url, urls[0].url());

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL.
  AssertAllProfilesHaveSameURLsAsVerifier();

  // Now, delete the URL from the second client.
  DeleteUrlFromHistory(1, new_url);
  urls = GetTypedUrlsFromClient(0);
  ASSERT_EQ(1U, urls.size());

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL removed.
  AssertAllProfilesHaveSameURLsAsVerifier();

  // Add it to the first client again, should succeed (tests that the deletion
  // properly disassociates that URL).
  AddUrlToHistory(0, new_url);

  // Let sync finish.
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // Both clients should have this URL added again.
  AssertAllProfilesHaveSameURLsAsVerifier();
}

IN_PROC_BROWSER_TEST_F(TwoClientTypedUrlsSyncTest,
                       SkipImportedVisits) {

  GURL imported_url("http://imported_url.com");
  GURL browsed_url("http://browsed_url.com");
  GURL browsed_and_imported_url("http://browsed_and_imported_url.com");
  ASSERT_TRUE(SetupClients());

  // Create 3 items in our first client - 1 imported, one browsed, one with
  // both imported and browsed entries.
  AddUrlToHistoryWithTransition(0, imported_url,
                                content::PAGE_TRANSITION_TYPED,
                                history::SOURCE_FIREFOX_IMPORTED);
  AddUrlToHistoryWithTransition(0, browsed_url,
                                content::PAGE_TRANSITION_TYPED,
                                history::SOURCE_BROWSED);
  AddUrlToHistoryWithTransition(0, browsed_and_imported_url,
                                content::PAGE_TRANSITION_TYPED,
                                history::SOURCE_FIREFOX_IMPORTED);

  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  std::vector<history::URLRow> urls = GetTypedUrlsFromClient(1);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(browsed_url, urls[0].url());

  // Now browse to 3rd URL - this should cause it to be synced, even though it
  // was initially imported.
  AddUrlToHistoryWithTransition(0, browsed_and_imported_url,
                                content::PAGE_TRANSITION_TYPED,
                                history::SOURCE_BROWSED);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  urls = GetTypedUrlsFromClient(1);
  ASSERT_EQ(2U, urls.size());

  // Make sure the imported URL didn't make it over.
  for (size_t i = 0; i < urls.size(); ++i) {
    ASSERT_NE(imported_url, urls[i].url());
  }
}
