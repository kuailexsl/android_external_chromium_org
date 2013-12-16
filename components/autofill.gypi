# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      # Private target only used in components/autofill.
      'target_name': 'autofill_regexes',
      'type': 'none',
      'actions': [{
        'action_name': 'autofill_regexes',
        'inputs': [
          '<(DEPTH)/build/escape_unicode.py',
          'autofill/core/browser/autofill_regex_constants.cc.utf8',
        ],
        'outputs': [
          '<(SHARED_INTERMEDIATE_DIR)/autofill_regex_constants.cc',
        ],
        'action': ['python', '<(DEPTH)/build/escape_unicode.py',
                   '-o', '<(SHARED_INTERMEDIATE_DIR)',
                   'autofill/core/browser/autofill_regex_constants.cc.utf8'],
      }],
    },
    
    {
      'target_name': 'autofill_core_common',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../ui/gfx/gfx.gyp:gfx',
        '../ui/ui.gyp:ui',
        '../url/url.gyp:url_lib',
      ],
      'conditions': [
        ['OS == "android"', {
          'dependencies': [
            'autofill_jni_headers',
          ],
        }],
      ],
      'include_dirs': [
        '..',
        '<(SHARED_INTERMEDIATE_DIR)/autofill'
      ],
      'sources': [
        'autofill/core/browser/android/auxiliary_profile_loader_android.cc',
        'autofill/core/browser/android/auxiliary_profile_loader_android.h',
        'autofill/core/browser/android/auxiliary_profiles_android.cc',
        'autofill/core/browser/android/auxiliary_profiles_android.h',
        'autofill/core/browser/android/component_jni_registrar.cc',
        'autofill/core/browser/android/component_jni_registrar.h',
        'autofill/core/browser/android/personal_data_manager_android.cc',
        'autofill/core/common/autofill_constants.cc',
        'autofill/core/common/autofill_constants.h',
        'autofill/core/common/autofill_pref_names.cc',
        'autofill/core/common/autofill_pref_names.h',
        'autofill/core/common/autofill_switches.cc',
        'autofill/core/common/autofill_switches.h',
        'autofill/core/common/form_data.cc',
        'autofill/core/common/form_data.h',
        'autofill/core/common/form_data_predictions.cc',
        'autofill/core/common/form_data_predictions.h',
        'autofill/core/common/form_field_data.cc',
        'autofill/core/common/form_field_data.h',
        'autofill/core/common/form_field_data_predictions.cc',
        'autofill/core/common/form_field_data_predictions.h',
        'autofill/core/common/password_autofill_util.cc',
        'autofill/core/common/password_autofill_util.h',
        'autofill/core/common/password_form.cc',
        'autofill/core/common/password_form.h',
        'autofill/core/common/password_form_fill_data.cc',
        'autofill/core/common/password_form_fill_data.h',
        'autofill/core/common/password_generation_util.cc',
        'autofill/core/common/password_generation_util.h',
        'autofill/core/common/web_element_descriptor.cc',
        'autofill/core/common/web_element_descriptor.h',
      ],
    },

    {
      'target_name': 'autofill_core_browser',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        'autofill_core_common',
        'autofill_regexes',
        'component_strings.gyp:component_strings',
        'encryptor',
        'user_prefs',
        'webdata_common',
        '../base/base.gyp:base',
        '../base/base.gyp:base_i18n',
        '../base/base.gyp:base_prefs',
        '../google_apis/google_apis.gyp:google_apis',
        '../skia/skia.gyp:skia',
        '../sql/sql.gyp:sql',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/libjingle/libjingle.gyp:libjingle',
        '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber',
        '../ui/gfx/gfx.gyp:gfx',
        '../ui/ui.gyp:ui',
        '../url/url.gyp:url_lib',
      ],
      # TODO(blundell): Eliminate the need for this conditional dependence.
      # crbug.com/328150
      'conditions': [
        ['OS != "ios"', {
          'dependencies': [
            '../webkit/webkit_resources.gyp:webkit_resources',
          ],
        }],
      ],
      'sources': [
        'autofill/core/browser/address.cc',
        'autofill/core/browser/address.h',
        'autofill/core/browser/address_field.cc',
        'autofill/core/browser/address_field.h',
        'autofill/core/browser/autocomplete_history_manager.cc',
        'autofill/core/browser/autocomplete_history_manager.h',
        'autofill/core/browser/autofill-inl.h',
        'autofill/core/browser/autofill_country.cc',
        'autofill/core/browser/autofill_country.h',
        'autofill/core/browser/autofill_data_model.cc',
        'autofill/core/browser/autofill_data_model.h',
        'autofill/core/browser/autofill_download.cc',
        'autofill/core/browser/autofill_download.h',
        'autofill/core/browser/autofill_download_url.cc',
        'autofill/core/browser/autofill_download_url.h',
        'autofill/core/browser/autofill_driver.h',
        'autofill/core/browser/autofill_external_delegate.cc',
        'autofill/core/browser/autofill_external_delegate.h',
        'autofill/core/browser/autofill_field.cc',
        'autofill/core/browser/autofill_field.h',
        'autofill/core/browser/autofill_ie_toolbar_import_win.cc',
        'autofill/core/browser/autofill_ie_toolbar_import_win.h',
        'autofill/core/browser/autofill_manager.cc',
        'autofill/core/browser/autofill_manager.h',
        'autofill/core/browser/autofill_manager_delegate.h',
        'autofill/core/browser/autofill_manager_test_delegate.h',
        'autofill/core/browser/autofill_metrics.cc',
        'autofill/core/browser/autofill_metrics.h',
        'autofill/core/browser/autofill_popup_delegate.h',
        'autofill/core/browser/autofill_profile.cc',
        'autofill/core/browser/autofill_profile.h',
        'autofill/core/browser/autofill_regex_constants.cc.utf8',
        'autofill/core/browser/autofill_regex_constants.h',
        'autofill/core/browser/autofill_regexes.cc',
        'autofill/core/browser/autofill_regexes.h',
        'autofill/core/browser/autofill_scanner.cc',
        'autofill/core/browser/autofill_scanner.h',
        'autofill/core/browser/autofill_server_field_info.h',
        'autofill/core/browser/autofill_type.cc',
        'autofill/core/browser/autofill_type.h',
        'autofill/core/browser/autofill_xml_parser.cc',
        'autofill/core/browser/autofill_xml_parser.h',
        'autofill/core/browser/contact_info.cc',
        'autofill/core/browser/contact_info.h',
        'autofill/core/browser/credit_card.cc',
        'autofill/core/browser/credit_card.h',
        'autofill/core/browser/credit_card_field.cc',
        'autofill/core/browser/credit_card_field.h',
        'autofill/core/browser/email_field.cc',
        'autofill/core/browser/email_field.h',
        'autofill/core/browser/field_types.h',
        'autofill/core/browser/form_field.cc',
        'autofill/core/browser/form_field.h',
        'autofill/core/browser/form_group.cc',
        'autofill/core/browser/form_group.h',
        'autofill/core/browser/form_structure.cc',
        'autofill/core/browser/form_structure.h',
        'autofill/core/browser/name_field.cc',
        'autofill/core/browser/name_field.h',
        'autofill/core/browser/password_autofill_manager.cc',
        'autofill/core/browser/password_autofill_manager.h',
        'autofill/core/browser/password_generator.cc',
        'autofill/core/browser/password_generator.h',
        'autofill/core/browser/personal_data_manager.cc',
        'autofill/core/browser/personal_data_manager.h',
        'autofill/core/browser/personal_data_manager_mac.mm',
        'autofill/core/browser/personal_data_manager_observer.h',
        'autofill/core/browser/phone_field.cc',
        'autofill/core/browser/phone_field.h',
        'autofill/core/browser/phone_number.cc',
        'autofill/core/browser/phone_number.h',
        'autofill/core/browser/phone_number_i18n.cc',
        'autofill/core/browser/phone_number_i18n.h',
        'autofill/core/browser/state_names.cc',
        'autofill/core/browser/state_names.h',
        'autofill/core/browser/validation.cc',
        'autofill/core/browser/validation.h',
        'autofill/core/browser/webdata/autofill_change.cc',
        'autofill/core/browser/webdata/autofill_change.h',
        'autofill/core/browser/webdata/autofill_entry.cc',
        'autofill/core/browser/webdata/autofill_entry.h',
        'autofill/core/browser/webdata/autofill_table.cc',
        'autofill/core/browser/webdata/autofill_table.h',
        'autofill/core/browser/webdata/autofill_webdata.h',
        'autofill/core/browser/webdata/autofill_webdata_backend.h',
        'autofill/core/browser/webdata/autofill_webdata_backend_impl.cc',
        'autofill/core/browser/webdata/autofill_webdata_backend_impl.h',
        'autofill/core/browser/webdata/autofill_webdata_service.cc',
        'autofill/core/browser/webdata/autofill_webdata_service.h',
        'autofill/core/browser/webdata/autofill_webdata_service_observer.h',

        # This file is generated by the autofill_regexes action.
        '<(SHARED_INTERMEDIATE_DIR)/autofill_regex_constants.cc',
      ],

      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [4267, ],
    },

    {
      'target_name': 'autofill_core_test_support',
      'type': 'static_library',
      'dependencies': [
        'autofill_core_common',
        'autofill_core_browser',
        '../skia/skia.gyp:skia',
        '../testing/gtest.gyp:gtest',
      ],
      'sources': [
        'autofill/core/browser/android/test_auxiliary_profile_loader_android.cc',
        'autofill/core/browser/android/test_auxiliary_profile_loader_android.h',
        'autofill/core/browser/autofill_test_utils.cc',
        'autofill/core/browser/autofill_test_utils.h',
        'autofill/core/browser/data_driven_test.cc',
        'autofill/core/browser/data_driven_test.h',
        'autofill/core/browser/test_autofill_driver.cc',
        'autofill/core/browser/test_autofill_driver.h',
        'autofill/core/browser/test_autofill_external_delegate.cc',
        'autofill/core/browser/test_autofill_external_delegate.h',
        'autofill/core/browser/test_autofill_manager_delegate.cc',
        'autofill/core/browser/test_autofill_manager_delegate.h',
        'autofill/core/browser/test_personal_data_manager.cc',
        'autofill/core/browser/test_personal_data_manager.h',
      ],
    },
  ],
  'conditions': [
    ['OS != "ios"', {
      'targets': [
        {
          'target_name': 'autofill_content_common',
          'type': 'static_library',
          'dependencies': [
            'autofill_core_common',
            '../base/base.gyp:base',
            '../content/content.gyp:content_common',
            '../ipc/ipc.gyp:ipc',
            '../third_party/WebKit/public/blink.gyp:blink_minimal',
            '../ui/gfx/gfx.gyp:gfx',
          ],
          'include_dirs': [
            '..',
          ],
          'sources': [
            'autofill/content/common/autofill_messages.h',
            'autofill/content/common/autofill_message_generator.cc',
            'autofill/content/common/autofill_message_generator.h',
            'autofill/content/common/autofill_param_traits_macros.h',
          ],
        },

        {
          # Protobuf compiler / generate rule for Autofill's risk integration.
          'target_name': 'autofill_content_risk_proto',
          'type': 'static_library',
          'sources': [
            'autofill/content/browser/risk/proto/fingerprint.proto',
          ],
          'variables': {
            'proto_in_dir': 'autofill/content/browser/risk/proto',
            'proto_out_dir': 'components/autofill/content/browser/risk/proto',
          },
          'includes': [ '../build/protoc.gypi' ]
        },
       {
         'target_name': 'autofill_content_test_support',
         'type': 'static_library',
         'dependencies': [
            '../testing/gmock.gyp:gmock',
         ],
         'sources': [
           'autofill/content/browser/wallet/mock_wallet_client.cc',
           'autofill/content/browser/wallet/mock_wallet_client.h',
           'autofill/content/browser/wallet/wallet_test_util.cc',
           'autofill/content/browser/wallet/wallet_test_util.h',
         ],
         'include_dirs': [ '..' ],
       },
       {
          'target_name': 'autofill_content_browser',
          'type': 'static_library',
          'include_dirs': [
            '..',
          ],
          'dependencies': [
            'autofill_content_common',
            'autofill_content_risk_proto',
            'autofill_core_browser',
            'autofill_core_common',
            'autofill_regexes',
            'encryptor',
            'user_prefs',
            'webdata_common',
            '../base/base.gyp:base',
            '../base/base.gyp:base_i18n',
            '../base/base.gyp:base_prefs',
            '../content/content.gyp:content_browser',
            '../content/content.gyp:content_common',
            '../google_apis/google_apis.gyp:google_apis',
            '../ipc/ipc.gyp:ipc',
            '../skia/skia.gyp:skia',
            '../sql/sql.gyp:sql',
            '../third_party/icu/icu.gyp:icui18n',
            '../third_party/icu/icu.gyp:icuuc',
            '../third_party/libjingle/libjingle.gyp:libjingle',
            '../third_party/libphonenumber/libphonenumber.gyp:libphonenumber',
            '../ui/gfx/gfx.gyp:gfx',
            '../ui/ui.gyp:ui',
            '../url/url.gyp:url_lib',
            '../webkit/webkit_resources.gyp:webkit_resources',

            'component_strings.gyp:component_strings',
          ],
          'sources': [
            'autofill/content/browser/autofill_driver_impl.cc',
            'autofill/content/browser/autofill_driver_impl.h',
            'autofill/content/browser/request_autocomplete_manager.cc',
            'autofill/content/browser/request_autocomplete_manager.h',
            'autofill/content/browser/risk/fingerprint.cc',
            'autofill/content/browser/risk/fingerprint.h',
            'autofill/content/browser/wallet/form_field_error.cc',
            'autofill/content/browser/wallet/form_field_error.h',
            'autofill/content/browser/wallet/full_wallet.cc',
            'autofill/content/browser/wallet/full_wallet.h',
            'autofill/content/browser/wallet/gaia_account.cc',
            'autofill/content/browser/wallet/gaia_account.h',
            'autofill/content/browser/wallet/instrument.cc',
            'autofill/content/browser/wallet/instrument.h',
            'autofill/content/browser/wallet/required_action.cc',
            'autofill/content/browser/wallet/required_action.h',
            'autofill/content/browser/wallet/wallet_address.cc',
            'autofill/content/browser/wallet/wallet_address.h',
            'autofill/content/browser/wallet/wallet_client.cc',
            'autofill/content/browser/wallet/wallet_client.h',
            'autofill/content/browser/wallet/wallet_client_delegate.h',
            'autofill/content/browser/wallet/wallet_items.cc',
            'autofill/content/browser/wallet/wallet_items.h',
            'autofill/content/browser/wallet/wallet_service_url.cc',
            'autofill/content/browser/wallet/wallet_service_url.h',
            'autofill/content/browser/wallet/wallet_signin_helper.cc',
            'autofill/content/browser/wallet/wallet_signin_helper.h',
          ],

          # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
          'msvs_disabled_warnings': [4267, ],
        },

        {
          'target_name': 'autofill_content_renderer',
          'type': 'static_library',
          'include_dirs': [
            '..',
          ],
          'dependencies': [
            'autofill_content_common',
            'autofill_core_common',
            '../base/base.gyp:base',
            '../content/content.gyp:content_renderer',
            '../content/content.gyp:content_common',
            '../ipc/ipc.gyp:ipc',
            '../skia/skia.gyp:skia',

            'component_strings.gyp:component_strings',
          ],
          'sources': [
            'autofill/content/renderer/autofill_agent.cc',
            'autofill/content/renderer/autofill_agent.h',
            'autofill/content/renderer/form_autofill_util.cc',
            'autofill/content/renderer/form_autofill_util.h',
            'autofill/content/renderer/form_cache.cc',
            'autofill/content/renderer/form_cache.h',
            'autofill/content/renderer/page_click_listener.h',
            'autofill/content/renderer/page_click_tracker.cc',
            'autofill/content/renderer/page_click_tracker.h',
            'autofill/content/renderer/password_autofill_agent.cc',
            'autofill/content/renderer/password_autofill_agent.h',
            'autofill/content/renderer/password_form_conversion_utils.cc',
            'autofill/content/renderer/password_form_conversion_utils.h',
            'autofill/content/renderer/password_generation_agent.cc',
            'autofill/content/renderer/password_generation_agent.h',
          ],
          # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
          'msvs_disabled_warnings': [4267, ],
        },
      ],
    }],
    ['OS == "android"', {
      'targets': [
        {
          'target_name': 'autofill_java',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base',
            '../content/content.gyp:content_java',
          ],
          'variables': {
            'java_in_dir': 'autofill/core/browser/android/java',
          },
          'includes': [ '../build/java.gypi' ],
        },
        {
          'target_name': 'autofill_jni_headers',
          'type': 'none',
          'sources': [
            'autofill/core/browser/android/java/src/org/chromium/components/browser/autofill/PersonalAutofillPopulator.java',
          ],
          'variables': {
            'jni_gen_package': 'autofill',
          },
          'includes': [ '../build/jni_generator.gypi' ],
        },
      ],
    }],
  ],
}
