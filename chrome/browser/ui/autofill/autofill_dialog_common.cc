// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/autofill_dialog_common.h"

#include "chrome/browser/browser_process.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "grit/chromium_strings.h"
#include "grit/component_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "grit/webkit_resources.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/autofill/autofill_dialog_i18n_input.h"
#endif  // !defined(OS_ANDROID)

namespace autofill {
namespace common {

// Returns true if |input| should be shown when |field_type| has been requested.
bool InputTypeMatchesFieldType(const DetailInput& input,
                               const AutofillType& field_type) {
  // If any credit card expiration info is asked for, show both month and year
  // inputs.
  ServerFieldType server_type = field_type.GetStorableType();
  if (server_type == CREDIT_CARD_EXP_4_DIGIT_YEAR ||
      server_type == CREDIT_CARD_EXP_2_DIGIT_YEAR ||
      server_type == CREDIT_CARD_EXP_DATE_4_DIGIT_YEAR ||
      server_type == CREDIT_CARD_EXP_DATE_2_DIGIT_YEAR ||
      server_type == CREDIT_CARD_EXP_MONTH) {
    return input.type == CREDIT_CARD_EXP_4_DIGIT_YEAR ||
           input.type == CREDIT_CARD_EXP_MONTH;
  }

  if (server_type == CREDIT_CARD_TYPE)
    return input.type == CREDIT_CARD_NUMBER;

  // Check the groups to distinguish billing types from shipping ones.
  AutofillType input_type = AutofillType(input.type);
  return input_type.GetStorableType() == server_type &&
         input_type.group() == field_type.group();
}

// Returns true if |input| in the given |section| should be used for a
// site-requested |field|.
bool DetailInputMatchesField(DialogSection section,
                             const DetailInput& input,
                             const AutofillField& field) {
  AutofillType field_type = field.Type();

  // The credit card name is filled from the billing section's data.
  if (field_type.GetStorableType() == CREDIT_CARD_NAME &&
      (section == SECTION_BILLING || section == SECTION_CC_BILLING)) {
    return input.type == NAME_BILLING_FULL;
  }

  return InputTypeMatchesFieldType(input, field_type);
}

bool IsCreditCardType(ServerFieldType type) {
  return AutofillType(type).group() == CREDIT_CARD;
}

// Constructs |inputs| from template data.
void BuildInputs(const DetailInput* input_template,
                 size_t template_size,
                 DetailInputs* inputs) {
  for (size_t i = 0; i < template_size; ++i) {
    const DetailInput* input = &input_template[i];
    inputs->push_back(*input);
  }
}

bool IsI18nInputEnabled() {
#if defined(OS_ANDROID)
  return false;
#else
  return i18ninput::Enabled();
#endif
}

void BuildI18nAddressInputs(AddressType address_type,
                            const std::string& country_code,
                            DetailInputs* inputs) {
#if defined(OS_ANDROID)
  NOTREACHED();
#else
  i18ninput::BuildAddressInputs(address_type, country_code, inputs);
#endif
}

// Constructs |inputs| from template data for a given |dialog_section|.
void BuildInputsForSection(DialogSection dialog_section,
                           const std::string& country_code,
                           DetailInputs* inputs) {
  using l10n_util::GetStringUTF16;

  const DetailInput kCCInputs[] = {
    { DetailInput::LONG,
      CREDIT_CARD_NUMBER,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_CARD_NUMBER) },
    { DetailInput::SHORT,
      CREDIT_CARD_EXP_MONTH,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_EXPIRY_MONTH) },
    { DetailInput::SHORT,
      CREDIT_CARD_EXP_4_DIGIT_YEAR,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_EXPIRY_YEAR) },
    { DetailInput::SHORT_EOL,
      CREDIT_CARD_VERIFICATION_CODE,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_CVC),
      1.5 },
  };

  const DetailInput kBillingInputs[] = {
    { DetailInput::LONG,
      NAME_BILLING_FULL,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_CARDHOLDER_NAME) },
    { DetailInput::LONG,
      ADDRESS_BILLING_LINE1,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_ADDRESS_LINE_1) },
    { DetailInput::LONG,
      ADDRESS_BILLING_LINE2,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_ADDRESS_LINE_2) },
    { DetailInput::LONG,
      ADDRESS_BILLING_CITY,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_LOCALITY) },
    { DetailInput::SHORT,
      ADDRESS_BILLING_STATE,
      GetStringUTF16(IDS_AUTOFILL_FIELD_LABEL_STATE) },
    { DetailInput::SHORT_EOL,
      ADDRESS_BILLING_ZIP,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_POSTAL_CODE) },
    // We don't allow the user to change the country: http://crbug.com/247518
    { DetailInput::NONE, ADDRESS_BILLING_COUNTRY },
  };

  const DetailInput kBillingPhoneInputs[] = {
    { DetailInput::LONG,
      PHONE_BILLING_WHOLE_NUMBER,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_PHONE_NUMBER) },
  };

  const DetailInput kEmailInputs[] = {
    { DetailInput::LONG,
      EMAIL_ADDRESS,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_EMAIL) },
  };

  const DetailInput kShippingInputs[] = {
    { DetailInput::LONG,
      NAME_FULL,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_ADDRESSEE_NAME) },
    { DetailInput::LONG,
      ADDRESS_HOME_LINE1,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_ADDRESS_LINE_1) },
    { DetailInput::LONG,
      ADDRESS_HOME_LINE2,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_ADDRESS_LINE_2) },
    { DetailInput::LONG,
      ADDRESS_HOME_CITY,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_LOCALITY) },
    { DetailInput::SHORT,
      ADDRESS_HOME_STATE,
      GetStringUTF16(IDS_AUTOFILL_FIELD_LABEL_STATE) },
    { DetailInput::SHORT_EOL,
      ADDRESS_HOME_ZIP,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_POSTAL_CODE) },
    { DetailInput::NONE, ADDRESS_HOME_COUNTRY },
  };

  const DetailInput kShippingPhoneInputs[] = {
    { DetailInput::LONG,
      PHONE_HOME_WHOLE_NUMBER,
      GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_PHONE_NUMBER) },
  };

  switch (dialog_section) {
    case SECTION_CC:
      BuildInputs(kCCInputs, arraysize(kCCInputs), inputs);
      break;

    case SECTION_BILLING:
      if (IsI18nInputEnabled())
        BuildI18nAddressInputs(ADDRESS_TYPE_BILLING, country_code, inputs);
      else
        BuildInputs(kBillingInputs, arraysize(kBillingInputs), inputs);

      BuildInputs(kBillingPhoneInputs, arraysize(kBillingPhoneInputs), inputs);
      BuildInputs(kEmailInputs, arraysize(kEmailInputs), inputs);
      break;

    case SECTION_CC_BILLING:
      BuildInputs(kCCInputs, arraysize(kCCInputs), inputs);

      if (IsI18nInputEnabled())
        BuildI18nAddressInputs(ADDRESS_TYPE_BILLING, country_code, inputs);
      else
        BuildInputs(kBillingInputs, arraysize(kBillingInputs), inputs);

      BuildInputs(kBillingPhoneInputs, arraysize(kBillingPhoneInputs), inputs);
      break;

    case SECTION_SHIPPING:
      if (IsI18nInputEnabled())
        BuildI18nAddressInputs(ADDRESS_TYPE_SHIPPING, country_code, inputs);
      else
        BuildInputs(kShippingInputs, arraysize(kShippingInputs), inputs);

      BuildInputs(
          kShippingPhoneInputs, arraysize(kShippingPhoneInputs), inputs);
      break;
  }
}

AutofillMetrics::DialogUiEvent DialogSectionToUiItemAddedEvent(
    DialogSection section) {
  switch (section) {
    case SECTION_BILLING:
      return AutofillMetrics::DIALOG_UI_BILLING_ITEM_ADDED;

    case SECTION_CC_BILLING:
      return AutofillMetrics::DIALOG_UI_CC_BILLING_ITEM_ADDED;

    case SECTION_SHIPPING:
      return AutofillMetrics::DIALOG_UI_SHIPPING_ITEM_ADDED;

    case SECTION_CC:
      return AutofillMetrics::DIALOG_UI_CC_ITEM_ADDED;
  }

  NOTREACHED();
  return AutofillMetrics::NUM_DIALOG_UI_EVENTS;
}

AutofillMetrics::DialogUiEvent DialogSectionToUiSelectionChangedEvent(
    DialogSection section) {
  switch (section) {
    case SECTION_BILLING:
      return AutofillMetrics::DIALOG_UI_BILLING_SELECTED_SUGGESTION_CHANGED;

    case SECTION_CC_BILLING:
      return AutofillMetrics::DIALOG_UI_CC_BILLING_SELECTED_SUGGESTION_CHANGED;

    case SECTION_SHIPPING:
      return AutofillMetrics::DIALOG_UI_SHIPPING_SELECTED_SUGGESTION_CHANGED;

    case SECTION_CC:
      return AutofillMetrics::DIALOG_UI_CC_SELECTED_SUGGESTION_CHANGED;
  }

  NOTREACHED();
  return AutofillMetrics::NUM_DIALOG_UI_EVENTS;
}

base::string16 GetHardcodedValueForType(ServerFieldType type) {
  if (AutofillType(type).GetStorableType() == ADDRESS_HOME_COUNTRY) {
    AutofillCountry country("US", g_browser_process->GetApplicationLocale());
    return country.name();
  }

  return base::string16();
}

}  // namespace common
}  // namespace autofill
