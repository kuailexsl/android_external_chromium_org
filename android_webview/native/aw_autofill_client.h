// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_NATIVE_AW_AUTOFILL_CLIENT_H_
#define ANDROID_WEBVIEW_NATIVE_AW_AUTOFILL_CLIENT_H_

#include <jni.h>
#include <vector>

#include "base/android/jni_weak_ref.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service_factory.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "content/public/browser/web_contents_user_data.h"
#include "base/android/scoped_java_ref.h"

using base::android::ScopedJavaLocalRef;

namespace autofill {
class AutofillMetrics;
class AutofillPopupDelegate;
class CreditCard;
class FormStructure;
class PasswordGenerator;
class PersonalDataManager;
struct FormData;
}

namespace content {
class WebContents;
}

namespace gfx {
class RectF;
}

class PersonalDataManager;
class PrefService;

namespace android_webview {

// Manager delegate for the autofill functionality. Android webview
// supports enabling autocomplete feature for each webview instance
// (different than the browser which supports enabling/disabling for
// a profile). Since there is only one pref service for a given browser
// context, we cannot enable this feature via UserPrefs. Rather, we always
// keep the feature enabled at the pref service, and control it via
// the delegates.
class AwAutofillClient : public autofill::AutofillClient,
                         public content::WebContentsUserData<AwAutofillClient> {
 public:
  virtual ~AwAutofillClient();

  void SetSaveFormData(bool enabled);
  bool GetSaveFormData();

  // AutofillClient:
  virtual autofill::PersonalDataManager* GetPersonalDataManager() OVERRIDE;
  virtual scoped_refptr<autofill::AutofillWebDataService> GetDatabase()
      OVERRIDE;
  virtual PrefService* GetPrefs() OVERRIDE;
  virtual void HideRequestAutocompleteDialog() OVERRIDE;
  virtual void ShowAutofillSettings() OVERRIDE;
  virtual void ConfirmSaveCreditCard(
      const autofill::AutofillMetrics& metric_logger,
      const base::Closure& save_card_callback) OVERRIDE;
  virtual void ShowRequestAutocompleteDialog(
      const autofill::FormData& form,
      const GURL& source_url,
      const ResultCallback& callback) OVERRIDE;
  virtual void ShowAutofillPopup(
      const gfx::RectF& element_bounds,
      base::i18n::TextDirection text_direction,
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels,
      const std::vector<base::string16>& icons,
      const std::vector<int>& identifiers,
      base::WeakPtr<autofill::AutofillPopupDelegate> delegate) OVERRIDE;
  virtual void UpdateAutofillPopupDataListValues(
      const std::vector<base::string16>& values,
      const std::vector<base::string16>& labels) OVERRIDE;
  virtual void HideAutofillPopup() OVERRIDE;
  virtual bool IsAutocompleteEnabled() OVERRIDE;
  virtual void DetectAccountCreationForms(
      const std::vector<autofill::FormStructure*>& forms) OVERRIDE;
  virtual void DidFillOrPreviewField(
      const base::string16& autofilled_value,
      const base::string16& profile_full_name) OVERRIDE;

  void SuggestionSelected(JNIEnv* env, jobject obj, jint position);
  std::string AddOrUpdateProfile(jstring guid, jstring name_full, jstring email, jstring company,
        jstring address1, jstring address2, jstring city,
        jstring state, jstring zipcode, jstring country,
        jstring phone);
  void RemoveProfileByGUID(jstring guid);
  ScopedJavaLocalRef<jobject>  GetProfileByGUID(jstring guid);
  void RemoveAllAutoFillProfiles();
  ScopedJavaLocalRef<jobjectArray> GetAllAutoFillProfiles();

 private:
  AwAutofillClient(content::WebContents* web_contents);
  friend class content::WebContentsUserData<AwAutofillClient>;

  void ShowAutofillPopupImpl(const gfx::RectF& element_bounds,
                             bool is_rtl,
                             const std::vector<base::string16>& values,
                             const std::vector<base::string16>& labels,
                             const std::vector<int>& identifiers);

  // The web_contents associated with this delegate.
  content::WebContents* web_contents_;
  bool save_form_data_;
  JavaObjectWeakGlobalRef java_ref_;

  // The current Autofill query values.
  std::vector<base::string16> values_;
  std::vector<int> identifiers_;
  base::WeakPtr<autofill::AutofillPopupDelegate> delegate_;
  scoped_ptr<autofill::PersonalDataManager> personal_data_;
  scoped_ptr<autofill::PersonalDataManager> personal_data_incog;

  DISALLOW_COPY_AND_ASSIGN(AwAutofillClient);
};

bool RegisterAwAutofillClient(JNIEnv* env);

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_NATIVE_AW_AUTOFILL_CLIENT_H_
