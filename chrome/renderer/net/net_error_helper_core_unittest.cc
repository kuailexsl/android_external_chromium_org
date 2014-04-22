// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/net/net_error_helper_core.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/timer/mock_timer.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "chrome/common/net/net_error_info.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebURLError.h"

using blink::WebURLError;
using chrome_common_net::DnsProbeStatus;
using chrome_common_net::DnsProbeStatusToString;

const char kFailedUrl[] = "http://failed/";
const char kFailedHttpsUrl[] = "https://failed/";
const char kNavigationCorrectionUrl[] = "http://navigation.corrections/";
const char kSearchUrl[] = "http://www.google.com/search";
const char kSuggestedSearchTerms[] = "Happy Goats";

struct NavigationCorrection {
  const char* correction_type;
  const char* url_correction;
  bool is_porn;
  bool is_soft_porn;

  base::Value* ToValue() const {
    base::DictionaryValue* dict = new base::DictionaryValue();
    dict->SetString("correctionType", correction_type);
    dict->SetString("urlCorrection", url_correction);
    dict->SetBoolean("isPorn", is_porn);
    dict->SetBoolean("isSoftPorn", is_soft_porn);
    return dict;
  }
};

const NavigationCorrection kDefaultCorrections[] = {
  {"reloadPage", kFailedUrl, false, false},
  {"urlCorrection", "http://somewhere_else/", false, false},
  {"contentOverlap", "http://somewhere_else_entirely/", false, false},

  // Porn should be ignored.
  {"emphasizedUrlCorrection", "http://porn/", true, false},
  {"sitemap", "http://more_porn/", false, true},

  {"webSearchQuery", kSuggestedSearchTerms, false, false},
};

std::string SuggestionsToResponse(const NavigationCorrection* corrections,
                                  int num_corrections) {
  base::ListValue* url_corrections = new base::ListValue();
  for (int i = 0; i < num_corrections; ++i)
    url_corrections->Append(corrections[i].ToValue());

  scoped_ptr<base::DictionaryValue> response(new base::DictionaryValue());
  response->Set("result.UrlCorrections", url_corrections);

  std::string json;
  base::JSONWriter::Write(response.get(), &json);
  return json;
}

// Creates a string from an error that is used as a mock locally generated
// error page for that error.
std::string ErrorToString(const WebURLError& error, bool is_failed_post) {
  return base::StringPrintf("(%s, %s, %i, %s)",
                            error.unreachableURL.string().utf8().c_str(),
                            error.domain.utf8().c_str(), error.reason,
                            is_failed_post ? "POST" : "NOT POST");
}

WebURLError ProbeError(DnsProbeStatus status) {
  WebURLError error;
  error.unreachableURL = GURL(kFailedUrl);
  error.domain = blink::WebString::fromUTF8(
      chrome_common_net::kDnsProbeErrorDomain);
  error.reason = status;
  return error;
}

WebURLError NetError(net::Error net_error) {
  WebURLError error;
  error.unreachableURL = GURL(kFailedUrl);
  error.domain = blink::WebString::fromUTF8(net::kErrorDomain);
  error.reason = net_error;
  return error;
}

WebURLError HttpError(int status_code) {
  WebURLError error;
  error.unreachableURL = GURL(kFailedUrl);
  error.domain = blink::WebString::fromUTF8("http");
  error.reason = status_code;
  return error;
}

// Convenience functions that create an error string for a non-POST request.

std::string ProbeErrorString(DnsProbeStatus status) {
  return ErrorToString(ProbeError(status), false);
}

std::string NetErrorString(net::Error net_error) {
  return ErrorToString(NetError(net_error), false);
}

class NetErrorHelperCoreTest : public testing::Test,
                               public NetErrorHelperCore::Delegate {
 public:
  NetErrorHelperCoreTest() : timer_(new base::MockTimer(false, false)),
                             core_(this),
                             update_count_(0),
                             error_html_update_count_(0),
                             reload_count_(0),
                             load_stale_count_(0),
                             enable_page_helper_functions_count_(0) {
    core_.set_auto_reload_enabled(false);
    core_.set_timer_for_testing(scoped_ptr<base::Timer>(timer_));
  }

  virtual ~NetErrorHelperCoreTest() {
    // No test finishes while an error page is being fetched.
    EXPECT_FALSE(is_url_being_fetched());
  }

  NetErrorHelperCore& core() { return core_; }

  const GURL& url_being_fetched() const { return url_being_fetched_; }
  bool is_url_being_fetched() const { return !url_being_fetched_.is_empty(); }

  int reload_count() const {
    return reload_count_;
  }

  int load_stale_count() const {
    return load_stale_count_;
  }

  const GURL& load_stale_url() const {
    return load_stale_url_;
  }

  int enable_page_helper_functions_count() const {
    return enable_page_helper_functions_count_;
  }

  const std::string& last_update_string() const { return last_update_string_; }
  int update_count() const { return update_count_;  }

  const std::string& last_error_html() const { return last_error_html_; }
  int error_html_update_count() const { return error_html_update_count_; }

  const LocalizedError::ErrorPageParams* last_error_page_params() const {
    return last_error_page_params_.get();
  }

  base::MockTimer* timer() { return timer_; }

  void NavigationCorrectionsLoadSuccess(
      const NavigationCorrection* corrections, int num_corrections) {
    NavigationCorrectionsLoadFinished(
        SuggestionsToResponse(corrections, num_corrections));
  }

  void NavigationCorrectionsLoadFailure() {
    NavigationCorrectionsLoadFinished("");
  }

  void NavigationCorrectionsLoadFinished(const std::string& result) {
    url_being_fetched_ = GURL();
    core().OnNavigationCorrectionsFetched(result, "en", false);
  }

  void DoErrorLoad(net::Error error) {
    core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                       NetErrorHelperCore::NON_ERROR_PAGE);
    std::string html;
    core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                        NetError(error), false, &html);
    EXPECT_FALSE(html.empty());
    EXPECT_EQ(NetErrorString(error), html);

    core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                       NetErrorHelperCore::ERROR_PAGE);
    core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
    core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  }

  void DoSuccessLoad() {
    core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                       NetErrorHelperCore::NON_ERROR_PAGE);
    core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
    core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  }

  void DoDnsProbe(chrome_common_net::DnsProbeStatus final_status) {
    core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
    core().OnNetErrorInfo(final_status);
  }

  void EnableNavigationCorrections() {
    SetNavigationCorrectionURL(GURL(kNavigationCorrectionUrl));
  }

  void DisableNavigationCorrections() {
    SetNavigationCorrectionURL(GURL());
  }

  void ExpectDefaultNavigationCorrections() const {
    // Checks that the last error page params correspond to kDefaultSuggestions.
    ASSERT_TRUE(last_error_page_params());
    EXPECT_TRUE(last_error_page_params()->suggest_reload);
    EXPECT_EQ(2u, last_error_page_params()->override_suggestions->GetSize());
    EXPECT_EQ(GURL(kSearchUrl), last_error_page_params()->search_url);
    EXPECT_EQ(kSuggestedSearchTerms, last_error_page_params()->search_terms);
  }

 private:
  void SetNavigationCorrectionURL(const GURL& navigation_correction_url) {
    core().OnSetNavigationCorrectionInfo(navigation_correction_url,
                                         "en", "us", "api_key",
                                         GURL(kSearchUrl));
  }

  // NetErrorHelperCore::Delegate implementation:
  virtual void GenerateLocalizedErrorPage(
      const WebURLError& error,
      bool is_failed_post,
      scoped_ptr<LocalizedError::ErrorPageParams> params,
      bool* reload_button_shown,
      bool* load_stale_button_shown,
      std::string* html) const OVERRIDE {
    last_error_page_params_.reset(params.release());
    *reload_button_shown = false;
    *load_stale_button_shown = false;
    *html = ErrorToString(error, is_failed_post);
  }

  virtual void LoadErrorPageInMainFrame(const std::string& html,
                                        const GURL& failed_url) OVERRIDE {
    error_html_update_count_++;
    last_error_html_ = html;
  }

  virtual void EnablePageHelperFunctions() OVERRIDE {
    enable_page_helper_functions_count_++;
  }

  virtual void UpdateErrorPage(const WebURLError& error,
                               bool is_failed_post) OVERRIDE {
    update_count_++;
    last_error_page_params_.reset(NULL);
    last_error_html_ = ErrorToString(error, is_failed_post);
  }

  virtual void FetchNavigationCorrections(
      const GURL& navigation_correction_url,
      const std::string& navigation_correction_request_body) OVERRIDE {
    EXPECT_TRUE(url_being_fetched_.is_empty());
    EXPECT_TRUE(request_body_.empty());
    EXPECT_EQ(GURL(kNavigationCorrectionUrl), navigation_correction_url);

    url_being_fetched_ = navigation_correction_url;
    request_body_ = navigation_correction_request_body;
  }

  virtual void CancelFetchNavigationCorrections() OVERRIDE {
    url_being_fetched_ = GURL();
    request_body_.clear();
  }

  virtual void ReloadPage() OVERRIDE {
    reload_count_++;
  }

  virtual void LoadPageFromCache(const GURL& error_url) OVERRIDE {
    load_stale_count_++;
    load_stale_url_ = error_url;
  }

  base::MockTimer* timer_;

  NetErrorHelperCore core_;

  GURL url_being_fetched_;
  std::string request_body_;

  // Contains the information passed to the last call to UpdateErrorPage, as a
  // string.
  std::string last_update_string_;
  // Number of times |last_update_string_| has been changed.
  int update_count_;

  // Contains the HTML set by the last call to LoadErrorPageInMainFrame.
  std::string last_error_html_;
  // Number of times |last_error_html_| has been changed.
  int error_html_update_count_;

  // Mutable because GenerateLocalizedErrorPage is const.
  mutable scoped_ptr<LocalizedError::ErrorPageParams> last_error_page_params_;

  int reload_count_;
  int load_stale_count_;
  GURL load_stale_url_;

  int enable_page_helper_functions_count_;
};

//------------------------------------------------------------------------------
// Basic tests that don't update the error page for probes or load navigation
// corrections.
//------------------------------------------------------------------------------

TEST_F(NetErrorHelperCoreTest, Null) {
}

TEST_F(NetErrorHelperCoreTest, SuccessfulPageLoad) {
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

TEST_F(NetErrorHelperCoreTest, SuccessfulPageLoadWithNavigationCorrections) {
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

TEST_F(NetErrorHelperCoreTest, MainFrameNonDnsError) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  // Should have returned a local error page.
  EXPECT_FALSE(html.empty());
  EXPECT_EQ(NetErrorString(net::ERR_CONNECTION_RESET), html);

  // Error page loads.
  EXPECT_EQ(0, enable_page_helper_functions_count());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
  EXPECT_EQ(1, enable_page_helper_functions_count());
}

TEST_F(NetErrorHelperCoreTest, MainFrameNonDnsErrorWithCorrections) {
  EnableNavigationCorrections();

  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  // Should have returned a local error page.
  EXPECT_FALSE(html.empty());
  EXPECT_EQ(NetErrorString(net::ERR_CONNECTION_RESET), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Much like above tests, but with a bunch of spurious DNS status messages that
// should have no effect.
TEST_F(NetErrorHelperCoreTest, MainFrameNonDnsErrorSpuriousStatus) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET),
                      false, &html);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  // Should have returned a local error page.
  EXPECT_FALSE(html.empty());
  EXPECT_EQ(NetErrorString(net::ERR_CONNECTION_RESET),  html);

  // Error page loads.

  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

TEST_F(NetErrorHelperCoreTest, SubFrameDnsError) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::SUB_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::SUB_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page.
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::SUB_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::SUB_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::SUB_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

TEST_F(NetErrorHelperCoreTest, SubFrameDnsErrorWithCorrections) {
  EnableNavigationCorrections();

  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::SUB_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::SUB_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page.
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::SUB_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::SUB_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::SUB_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Much like above tests, but with a bunch of spurious DNS status messages that
// should have no effect.
TEST_F(NetErrorHelperCoreTest, SubFrameDnsErrorSpuriousStatus) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::SUB_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::SUB_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  // Should have returned a local error page.
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), html);

  // Error page loads.

  core().OnStartLoad(NetErrorHelperCore::SUB_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  core().OnCommitLoad(NetErrorHelperCore::SUB_FRAME);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  core().OnFinishLoad(NetErrorHelperCore::SUB_FRAME);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

//------------------------------------------------------------------------------
// Tests for updating the error page in response to DNS probe results.  None
// of these have navigation corrections enabled.
//------------------------------------------------------------------------------

// Test case where the error page finishes loading before receiving any DNS
// probe messages.
TEST_F(NetErrorHelperCoreTest, FinishedBeforeProbe) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Same as above, but the probe is not run.
TEST_F(NetErrorHelperCoreTest, FinishedBeforeProbeNotRun) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());

  // When the not run status arrives, the page should revert to the normal dns
  // error page.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_NOT_RUN);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Same as above, but the probe result is inconclusive.
TEST_F(NetErrorHelperCoreTest, FinishedBeforeProbeInconclusive) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  // When the inconclusive status arrives, the page should revert to the normal
  // dns error page.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_INCONCLUSIVE);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_INCONCLUSIVE);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Same as above, but the probe result is no internet.
TEST_F(NetErrorHelperCoreTest, FinishedBeforeProbeNoInternet) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  // When the inconclusive status arrives, the page should revert to the normal
  // dns error page.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET),
            last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Same as above, but the probe result is bad config.
TEST_F(NetErrorHelperCoreTest, FinishedBeforeProbeBadConfig) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  // When the inconclusive status arrives, the page should revert to the normal
  // dns error page.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_BAD_CONFIG);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_BAD_CONFIG),
            last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_BAD_CONFIG);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Test case where the error page finishes loading after receiving the start
// DNS probe message.
TEST_F(NetErrorHelperCoreTest, FinishedAfterStartProbe) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);

  // Nothing should be done when a probe status comes in before loading
  // finishes.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(0, update_count());

  // When loading finishes, however, the buffered probe status should be sent
  // to the page.
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  // Should update the page again when the probe result comes in.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_NOT_RUN);
  EXPECT_EQ(2, update_count());
}

// Test case where the error page finishes loading before receiving any DNS
// probe messages and the request is a POST.
TEST_F(NetErrorHelperCoreTest, FinishedBeforeProbePost) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      true, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ErrorToString(
                ProbeError(chrome_common_net::DNS_PROBE_POSSIBLE),
                           true),
            html);

  // Error page loads.
  EXPECT_EQ(0, enable_page_helper_functions_count());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(1, enable_page_helper_functions_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ErrorToString(
                ProbeError(chrome_common_net::DNS_PROBE_STARTED), true),
            last_error_html());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ErrorToString(
                ProbeError(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
                           true),
            last_error_html());
  EXPECT_EQ(0, error_html_update_count());
}

// Test case where the probe finishes before the page is committed.
TEST_F(NetErrorHelperCoreTest, ProbeFinishesEarly) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);

  // Nothing should be done when the probe statuses come in before loading
  // finishes.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());

  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(0, update_count());

  // When loading finishes, however, the buffered probe status should be sent
  // to the page.
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // Any other probe updates should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(1, update_count());
}

// Test case where one error page loads completely before a new navigation
// results in another error page.  Probes are run for both pages.
TEST_F(NetErrorHelperCoreTest, TwoErrorsWithProbes) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Probe results come in.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // The process starts again.

  // Normal page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(2, update_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(3, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  // The probe returns a different result this time.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET);
  EXPECT_EQ(4, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET),
            last_error_html());
  EXPECT_EQ(0, error_html_update_count());
}

// Test case where one error page loads completely before a new navigation
// results in another error page.  Probe results for the first probe are only
// received after the second load starts, but before it commits.
TEST_F(NetErrorHelperCoreTest, TwoErrorsWithProbesAfterSecondStarts) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // The process starts again.

  // Normal page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page starts to load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);

  // Probe results come in, and the first page is updated.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // Second page finishes loading, and is updated using the same probe result.
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(3, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // Other probe results should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET);
  EXPECT_EQ(3, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// Same as above, but a new page is loaded before the error page commits.
TEST_F(NetErrorHelperCoreTest, ErrorPageLoadInterrupted) {
  // Original page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and an error page is requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  // Probe statuses come in, but should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());

  // A new navigation begins while the error page is loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // And fails.
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  // Should have returned a local error page indicating a probe may run.
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE), html);

  // Error page finishes loading.
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Probe results come in.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NO_INTERNET),
            last_error_html());
  EXPECT_EQ(0, error_html_update_count());
}

//------------------------------------------------------------------------------
// Navigation correction tests.
//------------------------------------------------------------------------------

// Check that corrections are not used for HTTPS URLs.
TEST_F(NetErrorHelperCoreTest, NoCorrectionsForHttps) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // The HTTPS page fails to load.
  std::string html;
  blink::WebURLError error = NetError(net::ERR_NAME_NOT_RESOLVED);
  error.unreachableURL = GURL(kFailedHttpsUrl);
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME, error, false, &html);

  blink::WebURLError probe_error =
      ProbeError(chrome_common_net::DNS_PROBE_POSSIBLE);
  probe_error.unreachableURL = GURL(kFailedHttpsUrl);
  EXPECT_EQ(ErrorToString(probe_error, false), html);
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // The blank page loads, no error page is loaded.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Page is updated in response to DNS probes as normal.
  EXPECT_EQ(0, update_count());
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_FALSE(last_error_page_params());
  blink::WebURLError final_probe_error =
      ProbeError(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  final_probe_error.unreachableURL = GURL(kFailedHttpsUrl);
  EXPECT_EQ(ErrorToString(final_probe_error, false), last_error_html());
}

// The blank page loads, then the navigation corrections request succeeds and is
// loaded.  Then the probe results come in.
TEST_F(NetErrorHelperCoreTest, CorrectionsReceivedBeforeProbe) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                      NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);

  // Corrections retrieval starts when the error page finishes loading.
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Corrections are retrieved.
  NavigationCorrectionsLoadSuccess(kDefaultCorrections,
                                   arraysize(kDefaultCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  ExpectDefaultNavigationCorrections();
  EXPECT_FALSE(is_url_being_fetched());

  // Corrections load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                      NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Any probe statuses should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);

  EXPECT_EQ(0, update_count());
  EXPECT_EQ(1, error_html_update_count());
}

// The blank page finishes loading, then probe results come in, and then
// the navigation corrections request succeeds.
TEST_F(NetErrorHelperCoreTest, CorrectionsRetrievedAfterProbes) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Probe statuses should be ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
  EXPECT_FALSE(last_error_page_params());

  // Corrections are retrieved.
  EXPECT_TRUE(is_url_being_fetched());
  NavigationCorrectionsLoadSuccess(kDefaultCorrections,
                                   arraysize(kDefaultCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  ExpectDefaultNavigationCorrections();
  EXPECT_FALSE(is_url_being_fetched());

  // Corrections load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(0, update_count());
}

// The corrections request fails and then the error page loads for an error that
// does not trigger DNS probes.
TEST_F(NetErrorHelperCoreTest, CorrectionsFailLoadNoProbes) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_FAILED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Corrections request fails, final error page is shown.
  EXPECT_TRUE(is_url_being_fetched());
  NavigationCorrectionsLoadFailure();
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(last_error_html(), NetErrorString(net::ERR_CONNECTION_FAILED));
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());
  EXPECT_FALSE(last_error_page_params());

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // If probe statuses come in last from another page load, they should be
  // ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(1, error_html_update_count());
}

// The corrections request fails and then the error page loads before probe
// results are received.
TEST_F(NetErrorHelperCoreTest, CorrectionsFailLoadBeforeProbe) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Corrections request fails, probe pending page shown.
  EXPECT_TRUE(is_url_being_fetched());
  NavigationCorrectionsLoadFailure();
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(last_error_html(),
            ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE));
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());

  // Probe page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Probe statuses comes in, and page is updated.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());

  // The commit results in sending a second probe status, which is ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(1, error_html_update_count());
}

// The corrections request fails after receiving probe results.
TEST_F(NetErrorHelperCoreTest, CorrectionsFailAfterProbe) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Results come in, but end up being ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());

  // Corrections request fails, probe pending page shown.
  EXPECT_TRUE(is_url_being_fetched());
  NavigationCorrectionsLoadFailure();
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(last_error_html(),
            ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE));
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());

  // Probe page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Probe statuses comes in, and page is updated.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(1, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());
  EXPECT_EQ(1, error_html_update_count());
}

// An error page load that would normally load correction is interrupted
// by a new navigation before the blank page commits.
TEST_F(NetErrorHelperCoreTest, CorrectionsInterruptedBeforeCommit) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page starts loading.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);

  // A new page load starts.
  EXPECT_FALSE(is_url_being_fetched());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // A new page load interrupts the original load.
  EXPECT_FALSE(is_url_being_fetched());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  EXPECT_FALSE(is_url_being_fetched());
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(is_url_being_fetched());
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// An error page load that would normally load corrections is interrupted
// by a new navigation before the blank page finishes loading.
TEST_F(NetErrorHelperCoreTest, CorrectionsInterruptedBeforeLoad) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page starts loading and is committed.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);

  // A new page load interrupts the original load.
  EXPECT_FALSE(is_url_being_fetched());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  EXPECT_FALSE(is_url_being_fetched());
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(is_url_being_fetched());
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());
  EXPECT_EQ(0, error_html_update_count());
}

// The corrections request is cancelled due to a new navigation.  The new
// navigation fails and then loads corrections successfully.
TEST_F(NetErrorHelperCoreTest, CorrectionsInterrupted) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());

  // Results come in, but end up being ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());

  // A new load appears!
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  EXPECT_FALSE(is_url_being_fetched());

  // It fails, and corrections are requested again once a blank page is loaded.
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(is_url_being_fetched());
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());

  // Corrections request succeeds.
  NavigationCorrectionsLoadSuccess(kDefaultCorrections,
                                   arraysize(kDefaultCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  ExpectDefaultNavigationCorrections();
  EXPECT_FALSE(is_url_being_fetched());

  // Probe statuses come in, and are ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());
}

// The corrections request is cancelled due to call to Stop().  The cross
// process navigation is cancelled, and then a new load fails and tries to load
// corrections again, unsuccessfully.
TEST_F(NetErrorHelperCoreTest, CorrectionsStopped) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  EXPECT_TRUE(is_url_being_fetched());
  core().OnStop();
  EXPECT_FALSE(is_url_being_fetched());

  // Results come in, but end up being ignored.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(0, update_count());

  // Cross process navigation must have been cancelled, and a new load appears!
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested again.
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads again.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());

  // Corrections request fails, probe pending page shown.
  NavigationCorrectionsLoadFailure();
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(last_error_html(),
            ProbeErrorString(chrome_common_net::DNS_PROBE_POSSIBLE));
  EXPECT_FALSE(is_url_being_fetched());

  // Probe page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Probe statuses comes in, and page is updated.
  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_STARTED);
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_STARTED),
            last_error_html());
  EXPECT_EQ(1, update_count());

  core().OnNetErrorInfo(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  EXPECT_EQ(2, update_count());
  EXPECT_EQ(ProbeErrorString(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN),
            last_error_html());
  EXPECT_EQ(1, error_html_update_count());
}

// Check the case corrections are disabled while the blank page (Loaded
// before the corrections page) is being loaded.
TEST_F(NetErrorHelperCoreTest, CorrectionsDisabledBeforeFetch) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  // Corrections is disabled.
  DisableNavigationCorrections();
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Corrections are retrieved.
  NavigationCorrectionsLoadSuccess(kDefaultCorrections,
                                   arraysize(kDefaultCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  EXPECT_FALSE(is_url_being_fetched());
  ExpectDefaultNavigationCorrections();

  // Corrections load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(0, update_count());
}

// Check the case corrections is disabled while fetching the corrections for
// a failed page load.
TEST_F(NetErrorHelperCoreTest, CorrectionsDisabledDuringFetch) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Corrections are disabled.
  DisableNavigationCorrections();

  // Corrections are retrieved.
  NavigationCorrectionsLoadSuccess(kDefaultCorrections,
                                   arraysize(kDefaultCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  EXPECT_FALSE(is_url_being_fetched());
  ExpectDefaultNavigationCorrections();

  // Corrections load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(0, update_count());
}

// Checks corrections are is used when there are no search suggestions.
TEST_F(NetErrorHelperCoreTest, CorrectionsWithoutSearch) {
  const NavigationCorrection kCorrections[] = {
    {"urlCorrection", "http://somewhere_else/", false, false},
  };

  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Corrections are retrieved.
  NavigationCorrectionsLoadSuccess(kCorrections, arraysize(kCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  EXPECT_FALSE(is_url_being_fetched());

  // Check params.
  ASSERT_TRUE(last_error_page_params());
  EXPECT_FALSE(last_error_page_params()->suggest_reload);
  EXPECT_EQ(1u, last_error_page_params()->override_suggestions->GetSize());
  EXPECT_FALSE(last_error_page_params()->search_url.is_valid());
  EXPECT_EQ("", last_error_page_params()->search_terms);

  // Corrections load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(0, update_count());
}

// Checks corrections are used when there are only search suggestions.
TEST_F(NetErrorHelperCoreTest, CorrectionsOnlySearchSuggestion) {
  const NavigationCorrection kCorrections[] = {
    {"webSearchQuery", kSuggestedSearchTerms, false, false},
  };

  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_NAME_NOT_RESOLVED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_TRUE(is_url_being_fetched());
  EXPECT_FALSE(last_error_page_params());

  // Corrections are retrieved.
  NavigationCorrectionsLoadSuccess(kCorrections, arraysize(kCorrections));
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(NetErrorString(net::ERR_NAME_NOT_RESOLVED), last_error_html());
  EXPECT_FALSE(is_url_being_fetched());

  // Check params.
  ASSERT_TRUE(last_error_page_params());
  EXPECT_FALSE(last_error_page_params()->suggest_reload);
  EXPECT_EQ(0u, last_error_page_params()->override_suggestions->GetSize());
  EXPECT_EQ(GURL(kSearchUrl), last_error_page_params()->search_url);
  EXPECT_EQ(kSuggestedSearchTerms, last_error_page_params()->search_terms);

  // Corrections load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(0, update_count());
}

// The correction service returns a non-JSON result.
TEST_F(NetErrorHelperCoreTest, CorrectionServiceReturnsNonJsonResult) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_FAILED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Corrections request fails, final error page is shown.
  EXPECT_TRUE(is_url_being_fetched());
  NavigationCorrectionsLoadFinished("Weird Response");
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(last_error_html(), NetErrorString(net::ERR_CONNECTION_FAILED));
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());
  EXPECT_FALSE(last_error_page_params());

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
}

// The correction service returns a JSON result that isn't a valid list of
// corrections.
TEST_F(NetErrorHelperCoreTest, CorrectionServiceReturnsInvalidJsonResult) {
  // Original page starts loading.
  EnableNavigationCorrections();
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);

  // It fails, and corrections are requested.
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_FAILED),
                      false, &html);
  EXPECT_TRUE(html.empty());

  // The blank page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);

  // Corrections request fails, final error page is shown.
  EXPECT_TRUE(is_url_being_fetched());
  NavigationCorrectionsLoadFinished("{\"result\": 42}");
  EXPECT_EQ(1, error_html_update_count());
  EXPECT_EQ(last_error_html(), NetErrorString(net::ERR_CONNECTION_FAILED));
  EXPECT_FALSE(is_url_being_fetched());
  EXPECT_EQ(0, update_count());
  EXPECT_FALSE(last_error_page_params());

  // Error page loads.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
}

TEST_F(NetErrorHelperCoreTest, AutoReloadDisabled) {
  DoErrorLoad(net::ERR_CONNECTION_RESET);

  EXPECT_FALSE(timer()->IsRunning());
  EXPECT_EQ(0, reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadSucceeds) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);

  EXPECT_TRUE(timer()->IsRunning());
  EXPECT_EQ(0, reload_count());

  timer()->Fire();
  EXPECT_FALSE(timer()->IsRunning());
  EXPECT_EQ(1, reload_count());

  DoSuccessLoad();

  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadRetries) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);

  EXPECT_TRUE(timer()->IsRunning());
  base::TimeDelta first_delay = timer()->GetCurrentDelay();
  EXPECT_EQ(0, reload_count());

  timer()->Fire();
  EXPECT_FALSE(timer()->IsRunning());
  EXPECT_EQ(1, reload_count());

  DoErrorLoad(net::ERR_CONNECTION_RESET);

  EXPECT_TRUE(timer()->IsRunning());
  EXPECT_GT(timer()->GetCurrentDelay(), first_delay);
}

TEST_F(NetErrorHelperCoreTest, AutoReloadStopsTimerOnStop) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_TRUE(timer()->IsRunning());
  core().OnStop();
  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadStopsLoadingOnStop) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_EQ(0, core().auto_reload_count());
  timer()->Fire();
  EXPECT_EQ(1, core().auto_reload_count());
  EXPECT_EQ(1, reload_count());
  core().OnStop();
  EXPECT_FALSE(timer()->IsRunning());
  EXPECT_EQ(0, core().auto_reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadStopsOnOtherLoadStart) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_TRUE(timer()->IsRunning());
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  EXPECT_FALSE(timer()->IsRunning());
  EXPECT_EQ(0, core().auto_reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadResetsCountOnSuccess) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  base::TimeDelta delay = timer()->GetCurrentDelay();
  EXPECT_EQ(0, core().auto_reload_count());
  timer()->Fire();
  EXPECT_EQ(1, core().auto_reload_count());
  EXPECT_EQ(1, reload_count());
  DoSuccessLoad();
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_EQ(0, core().auto_reload_count());
  EXPECT_EQ(timer()->GetCurrentDelay(), delay);
  timer()->Fire();
  EXPECT_EQ(1, core().auto_reload_count());
  EXPECT_EQ(2, reload_count());
  DoSuccessLoad();
  EXPECT_EQ(0, core().auto_reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadRestartsOnOnline) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  base::TimeDelta delay = timer()->GetCurrentDelay();
  timer()->Fire();
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_TRUE(timer()->IsRunning());
  EXPECT_NE(delay, timer()->GetCurrentDelay());
  core().NetworkStateChanged(true);
  EXPECT_TRUE(timer()->IsRunning());
  EXPECT_EQ(delay, timer()->GetCurrentDelay());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadDoesNotStartOnOnline) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  timer()->Fire();
  DoSuccessLoad();
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadStopsOnOffline) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_TRUE(timer()->IsRunning());
  core().NetworkStateChanged(false);
  EXPECT_FALSE(timer()->IsRunning());
  EXPECT_EQ(0, core().auto_reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadStopsOnOfflineThenRestartsOnOnline) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_TRUE(timer()->IsRunning());
  core().NetworkStateChanged(false);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_TRUE(timer()->IsRunning());
  EXPECT_EQ(0, core().auto_reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadDoesNotRestartOnOnlineAfterStop) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  timer()->Fire();
  core().OnStop();
  core().NetworkStateChanged(true);
  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadWithDnsProbes) {
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  DoDnsProbe(chrome_common_net::DNS_PROBE_FINISHED_NXDOMAIN);
  timer()->Fire();
  EXPECT_EQ(1, reload_count());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadExponentialBackoffLevelsOff) {
  core().set_auto_reload_enabled(true);
  base::TimeDelta previous = base::TimeDelta::FromMilliseconds(0);
  const int kMaxTries = 50;
  int tries = 0;
  for (tries = 0; tries < kMaxTries; tries++) {
    DoErrorLoad(net::ERR_CONNECTION_RESET);
    EXPECT_TRUE(timer()->IsRunning());
    if (previous == timer()->GetCurrentDelay())
      break;
    previous = timer()->GetCurrentDelay();
    timer()->Fire();
  }

  EXPECT_LT(tries, kMaxTries);
}

TEST_F(NetErrorHelperCoreTest, AutoReloadSlowError) {
  core().set_auto_reload_enabled(true);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(timer()->IsRunning());
  // Start a new non-error page load.
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  EXPECT_FALSE(timer()->IsRunning());
  // Finish the error page load.
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(timer()->IsRunning());
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadOnlineSlowError) {
  core().set_auto_reload_enabled(true);
  core().NetworkStateChanged(false);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(false);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_TRUE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadOnlinePendingError) {
  core().set_auto_reload_enabled(true);
  core().NetworkStateChanged(false);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(false);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_TRUE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, AutoReloadOnlinePartialErrorReplacement) {
  core().set_auto_reload_enabled(true);
  core().NetworkStateChanged(false);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  std::string html;
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  core().OnCommitLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnFinishLoad(NetErrorHelperCore::MAIN_FRAME);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::NON_ERROR_PAGE);
  core().GetErrorHTML(NetErrorHelperCore::MAIN_FRAME,
                      NetError(net::ERR_CONNECTION_RESET), false, &html);
  core().OnStartLoad(NetErrorHelperCore::MAIN_FRAME,
                     NetErrorHelperCore::ERROR_PAGE);
  EXPECT_FALSE(timer()->IsRunning());
  core().NetworkStateChanged(true);
  EXPECT_FALSE(timer()->IsRunning());
}

TEST_F(NetErrorHelperCoreTest, ShouldSuppressErrorPage) {
  // Set up the environment to test ShouldSuppressErrorPage: auto-reload is
  // enabled, an error page is loaded, and the auto-reload callback is running.
  core().set_auto_reload_enabled(true);
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  timer()->Fire();

  EXPECT_FALSE(core().ShouldSuppressErrorPage(NetErrorHelperCore::SUB_FRAME,
                                              GURL(kFailedUrl)));
  EXPECT_FALSE(core().ShouldSuppressErrorPage(NetErrorHelperCore::MAIN_FRAME,
                                              GURL("http://some.other.url")));
  EXPECT_TRUE(core().ShouldSuppressErrorPage(NetErrorHelperCore::MAIN_FRAME,
                                             GURL(kFailedUrl)));
}

TEST_F(NetErrorHelperCoreTest, ExplicitReloadSucceeds) {
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_EQ(0, reload_count());
  core().ExecuteButtonPress(NetErrorHelperCore::RELOAD_BUTTON);
  EXPECT_EQ(1, reload_count());
}

TEST_F(NetErrorHelperCoreTest, ExplicitLoadStaleSucceeds) {
  DoErrorLoad(net::ERR_CONNECTION_RESET);
  EXPECT_EQ(0, load_stale_count());
  core().ExecuteButtonPress(NetErrorHelperCore::LOAD_STALE_BUTTON);
  EXPECT_EQ(1, load_stale_count());
  EXPECT_EQ(GURL(kFailedUrl), load_stale_url());
}


