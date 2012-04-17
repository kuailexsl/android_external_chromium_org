# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'browser_extensions',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'dependencies': [
        'app/policy/cloud_policy_codegen.gyp:policy',
        '../sync/protocol/sync_proto.gyp:sync_proto',
        'chrome_resources.gyp:chrome_extra_resources',
        'chrome_resources.gyp:chrome_resources',
        'chrome_resources.gyp:chrome_strings',
        'chrome_resources.gyp:platform_locale_settings',
        'chrome_resources.gyp:theme_resources',
        'common',
        'common/extensions/api/api.gyp:api',
        'common_net',
        'debugger',
        'in_memory_url_index_cache_proto',
        'installer_util',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../content/content.gyp:content_browser',
        '../crypto/crypto.gyp:crypto',
        '../net/net.gyp:net',
        '../skia/skia.gyp:skia',
        '../third_party/bzip2/bzip2.gyp:bzip2',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/leveldatabase/leveldatabase.gyp:leveldatabase',
        '../ui/base/strings/ui_strings.gyp:ui_strings',
        '../ui/ui.gyp:ui',
        '../ui/ui.gyp:ui_resources',
        '../ui/ui.gyp:ui_resources_2x',
        '../ui/ui.gyp:ui_resources_standard',
        '../webkit/support/webkit_support.gyp:appcache',
        '../webkit/support/webkit_support.gyp:blob',
        '../webkit/support/webkit_support.gyp:database',
        '../webkit/support/webkit_support.gyp:fileapi',
        '../webkit/support/webkit_support.gyp:glue',
        '../webkit/support/webkit_support.gyp:quota',
        '../webkit/support/webkit_support.gyp:webkit_resources',
        '../webkit/support/webkit_support.gyp:webkit_user_agent',
      ],
      'include_dirs': [
        '..',
        '<(INTERMEDIATE_DIR)',
      ],
      'sources': [
        # All .cc, .h, .m, and .mm files under browser/extensions except for
        # tests and mocks.
        'browser/extensions/api/api_function.cc',
        'browser/extensions/api/api_function.h',
        'browser/extensions/api/api_resource.cc',
        'browser/extensions/api/api_resource.h',
        'browser/extensions/api/api_resource_controller.cc',
        'browser/extensions/api/api_resource_controller.h',
        'browser/extensions/api/api_resource_event_notifier.cc',
        'browser/extensions/api/api_resource_event_notifier.h',
        'browser/extensions/api/alarms/alarm_manager.cc',
        'browser/extensions/api/alarms/alarm_manager.h',
        'browser/extensions/api/alarms/alarms_api.cc',
        'browser/extensions/api/alarms/alarms_api.h',
        'browser/extensions/api/app/app_api.cc',
        'browser/extensions/api/app/app_api.h',
        'browser/extensions/api/bluetooth/bluetooth_api.cc',
        'browser/extensions/api/bluetooth/bluetooth_api.h',
        'browser/extensions/api/browsing_data/browsing_data_api.cc',
        'browser/extensions/api/browsing_data/browsing_data_api.h',
        'browser/extensions/api/declarative/declarative_api.cc',
        'browser/extensions/api/declarative/declarative_api.h',
        'browser/extensions/api/declarative/initializing_rules_registry.cc',
        'browser/extensions/api/declarative/initializing_rules_registry.h',
        'browser/extensions/api/declarative/rules_registry.cc',
        'browser/extensions/api/declarative/rules_registry.h',
        'browser/extensions/api/declarative/rules_registry_service.cc',
        'browser/extensions/api/declarative/rules_registry_service.h',
        'browser/extensions/api/declarative/rules_registry_with_cache.cc',
        'browser/extensions/api/declarative/rules_registry_with_cache.h',
        'browser/extensions/api/declarative/substring_set_matcher.cc',
        'browser/extensions/api/declarative/substring_set_matcher.h',
        'browser/extensions/api/declarative/test_rules_registry.cc',
        'browser/extensions/api/declarative/test_rules_registry.h',
        'browser/extensions/api/declarative/url_matcher.cc',
        'browser/extensions/api/declarative/url_matcher.h',
        'browser/extensions/api/declarative_webrequest/request_stages.h',
        'browser/extensions/api/declarative_webrequest/webrequest_action.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_action.h',
        'browser/extensions/api/declarative_webrequest/webrequest_condition.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_condition.h',
        'browser/extensions/api/declarative_webrequest/webrequest_condition_attribute.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_condition_attribute.h',
        'browser/extensions/api/declarative_webrequest/webrequest_constants.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_constants.h',
        'browser/extensions/api/declarative_webrequest/webrequest_helpers.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_helpers.h',
        'browser/extensions/api/declarative_webrequest/webrequest_rule.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_rule.h',
        'browser/extensions/api/declarative_webrequest/webrequest_rules_registry.cc',
        'browser/extensions/api/declarative_webrequest/webrequest_rules_registry.h',
        'browser/extensions/api/dns/dns_api.cc',
        'browser/extensions/api/dns/dns_api.h',
        'browser/extensions/api/extension_action/extension_actions_api.cc',
        'browser/extensions/api/extension_action/extension_actions_api.h',
        'browser/extensions/api/extension_action/extension_browser_actions_api.cc',
        'browser/extensions/api/extension_action/extension_browser_actions_api.h',
        'browser/extensions/api/extension_action/extension_page_actions_api.cc',
        'browser/extensions/api/extension_action/extension_page_actions_api.h',
        'browser/extensions/api/extension_action/extension_page_actions_api_constants.cc',
        'browser/extensions/api/extension_action/extension_page_actions_api_constants.h',
        'browser/extensions/api/identity/identity_api.cc',
        'browser/extensions/api/identity/identity_api.h',
        'browser/extensions/api/offscreen_tabs/offscreen_tabs_api.cc',
        'browser/extensions/api/offscreen_tabs/offscreen_tabs_api.h',
        'browser/extensions/api/offscreen_tabs/offscreen_tabs_constants.cc',
        'browser/extensions/api/offscreen_tabs/offscreen_tabs_constants.h',
        'browser/extensions/api/permissions/permissions_api.cc',
        'browser/extensions/api/permissions/permissions_api.h',
        'browser/extensions/api/permissions/permissions_api_helpers.cc',
        'browser/extensions/api/permissions/permissions_api_helpers.h',
        'browser/extensions/api/proxy/proxy_api.cc',
        'browser/extensions/api/proxy/proxy_api.h',
        'browser/extensions/api/proxy/proxy_api_constants.cc',
        'browser/extensions/api/proxy/proxy_api_constants.h',
        'browser/extensions/api/proxy/proxy_api_helpers.cc',
        'browser/extensions/api/proxy/proxy_api_helpers.h',
        'browser/extensions/api/runtime/runtime_api.cc',
        'browser/extensions/api/runtime/runtime_api.h',
        'browser/extensions/api/serial/serial_api.cc',
        'browser/extensions/api/serial/serial_api.h',
        'browser/extensions/api/serial/serial_connection.cc',
        'browser/extensions/api/serial/serial_connection.h',
        'browser/extensions/api/serial/serial_connection_posix.cc',
        'browser/extensions/api/serial/serial_connection_win.cc',
        'browser/extensions/api/socket/socket.cc',
        'browser/extensions/api/socket/socket.h',
        'browser/extensions/api/socket/socket_api.cc',
        'browser/extensions/api/socket/socket_api.h',
        'browser/extensions/api/socket/tcp_socket.cc',
        'browser/extensions/api/socket/tcp_socket.h',
        'browser/extensions/api/socket/udp_socket.cc',
        'browser/extensions/api/socket/udp_socket.h',
        'browser/extensions/api/terminal/terminal_extension_helper.cc',
        'browser/extensions/api/terminal/terminal_extension_helper.h',
        'browser/extensions/api/terminal/terminal_private_api.cc',
        'browser/extensions/api/terminal/terminal_private_api.h',
        'browser/extensions/api/web_navigation/web_navigation_api.cc',
        'browser/extensions/api/web_navigation/web_navigation_api.h',
        'browser/extensions/api/web_navigation/web_navigation_api_constants.cc',
        'browser/extensions/api/web_navigation/web_navigation_api_constants.h',
        'browser/extensions/api/web_request/web_request_api.cc',
        'browser/extensions/api/web_request/web_request_api.h',
        'browser/extensions/api/web_request/web_request_api_constants.cc',
        'browser/extensions/api/web_request/web_request_api_constants.h',
        'browser/extensions/api/web_request/web_request_api_helpers.cc',
        'browser/extensions/api/web_request/web_request_api_helpers.h',
        'browser/extensions/api/web_request/web_request_time_tracker.cc',
        'browser/extensions/api/web_request/web_request_time_tracker.h',
        'browser/extensions/app_notification.cc',
        'browser/extensions/app_notification.h',
        'browser/extensions/app_notification_manager.cc',
        'browser/extensions/app_notification_manager.h',
        'browser/extensions/app_notification_storage.cc',
        'browser/extensions/app_notification_storage.h',
        'browser/extensions/app_notify_channel_setup.cc',
        'browser/extensions/app_notify_channel_setup.h',
        'browser/extensions/app_notify_channel_ui.cc',
        'browser/extensions/app_notify_channel_ui.h',
        'browser/extensions/app_shortcut_manager.cc',
        'browser/extensions/app_shortcut_manager.h',
        'browser/extensions/apps_promo.cc',
        'browser/extensions/apps_promo.h',
        'browser/extensions/app_sync_bundle.cc',
        'browser/extensions/app_sync_bundle.h',
        'browser/extensions/app_sync_data.cc',
        'browser/extensions/app_sync_data.h',
        'browser/extensions/browser_action_test_util.h',
        'browser/extensions/browser_action_test_util_gtk.cc',
        'browser/extensions/browser_action_test_util_mac.mm',
        'browser/extensions/browser_action_test_util_views.cc',
        'browser/extensions/browser_extension_window_controller.cc',
        'browser/extensions/browser_extension_window_controller.h',
        'browser/extensions/bundle_installer.cc',
        'browser/extensions/bundle_installer.h',
        'browser/extensions/component_loader.cc',
        'browser/extensions/component_loader.h',
        'browser/extensions/convert_user_script.cc',
        'browser/extensions/convert_user_script.h',
        'browser/extensions/convert_web_app.cc',
        'browser/extensions/convert_web_app.h',
        'browser/extensions/crx_installer.cc',
        'browser/extensions/crx_installer.h',
        'browser/extensions/default_apps.cc',
        'browser/extensions/default_apps.h',
        'browser/extensions/default_apps_trial.cc',
        'browser/extensions/default_apps_trial.h',
        'browser/extensions/extension_activity_log.cc',
        'browser/extensions/extension_activity_log.h',
        'browser/extensions/extension_browser_event_router.cc',
        'browser/extensions/extension_browser_event_router.h',
        'browser/extensions/extension_content_settings_api_constants.cc',
        'browser/extensions/extension_content_settings_api_constants.h',
        'browser/extensions/extension_content_settings_helpers.cc',
        'browser/extensions/extension_content_settings_helpers.h',
        'browser/extensions/extension_content_settings_store.cc',
        'browser/extensions/extension_content_settings_store.h',
        'browser/extensions/extension_context_menu_model.cc',
        'browser/extensions/extension_context_menu_model.h',
        'browser/extensions/extension_cookies_api_constants.cc',
        'browser/extensions/extension_cookies_api_constants.h',
        'browser/extensions/extension_cookies_helpers.cc',
        'browser/extensions/extension_cookies_helpers.h',
        'browser/extensions/extension_creator.cc',
        'browser/extensions/extension_creator.h',
        'browser/extensions/extension_creator_filter.cc',
        'browser/extensions/extension_creator_filter.h',
        'browser/extensions/extension_data_deleter.cc',
        'browser/extensions/extension_data_deleter.h',
        'browser/extensions/extension_debugger_api_constants.cc',
        'browser/extensions/extension_debugger_api_constants.h',
        'browser/extensions/extension_devtools_bridge.cc',
        'browser/extensions/extension_devtools_bridge.h',
        'browser/extensions/extension_devtools_events.cc',
        'browser/extensions/extension_devtools_events.h',
        'browser/extensions/extension_devtools_manager.cc',
        'browser/extensions/extension_devtools_manager.h',
        'browser/extensions/extension_disabled_ui.cc',
        'browser/extensions/extension_disabled_ui.h',
        'browser/extensions/extension_error_reporter.cc',
        'browser/extensions/extension_error_reporter.h',
        'browser/extensions/extension_event_names.cc',
        'browser/extensions/extension_event_names.h',
        'browser/extensions/extension_event_router.cc',
        'browser/extensions/extension_event_router.h',
        'browser/extensions/extension_event_router_forwarder.cc',
        'browser/extensions/extension_event_router_forwarder.h',
        'browser/extensions/extension_function.cc',
        'browser/extensions/extension_function.h',
        'browser/extensions/extension_function_dispatcher.cc',
        'browser/extensions/extension_function_dispatcher.h',
        'browser/extensions/extension_function_registry.cc',
        'browser/extensions/extension_function_registry.h',
        'browser/extensions/extension_global_error.cc',
        'browser/extensions/extension_global_error.h',
        'browser/extensions/extension_global_error_badge.cc',
        'browser/extensions/extension_global_error_badge.h',
        'browser/extensions/extension_host.cc',
        'browser/extensions/extension_host.h',
        'browser/extensions/extension_host_mac.h',
        'browser/extensions/extension_host_mac.mm',
        'browser/extensions/extension_icon_manager.cc',
        'browser/extensions/extension_icon_manager.h',
        'browser/extensions/extension_idle_api_constants.cc',
        'browser/extensions/extension_idle_api_constants.h',
        'browser/extensions/extension_info_map.cc',
        'browser/extensions/extension_info_map.h',
        'browser/extensions/extension_info_private_api_chromeos.cc',
        'browser/extensions/extension_info_private_api_chromeos.h',
        'browser/extensions/extension_infobar_delegate.cc',
        'browser/extensions/extension_infobar_delegate.h',
        'browser/extensions/extension_input_module_constants.cc',
        'browser/extensions/extension_input_module_constants.h',
        'browser/extensions/extension_install_dialog.cc',
        'browser/extensions/extension_install_dialog.h',
        'browser/extensions/extension_install_ui.cc',
        'browser/extensions/extension_install_ui.h',
        'browser/extensions/extension_keybinding_registry.cc',
        'browser/extensions/extension_keybinding_registry.h',
        'browser/extensions/extension_management_api_constants.cc',
        'browser/extensions/extension_management_api_constants.h',
        'browser/extensions/extension_menu_manager.cc',
        'browser/extensions/extension_menu_manager.h',
        'browser/extensions/extension_message_handler.cc',
        'browser/extensions/extension_message_handler.h',
        'browser/extensions/extension_message_service.cc',
        'browser/extensions/extension_message_service.h',
        'browser/extensions/extension_module.cc',
        'browser/extensions/extension_module.h',
        'browser/extensions/extension_navigation_observer.cc',
        'browser/extensions/extension_navigation_observer.h',
        'browser/extensions/extension_omnibox_api.cc',
        'browser/extensions/extension_omnibox_api.h',
        'browser/extensions/extension_pref_store.cc',
        'browser/extensions/extension_pref_store.h',
        'browser/extensions/extension_pref_value_map.cc',
        'browser/extensions/extension_pref_value_map.h',
        'browser/extensions/extension_pref_value_map_factory.cc',
        'browser/extensions/extension_pref_value_map_factory.h',
        'browser/extensions/extension_preference_api_constants.cc',
        'browser/extensions/extension_preference_api_constants.h',
        'browser/extensions/extension_preference_helpers.cc',
        'browser/extensions/extension_preference_helpers.h',
        'browser/extensions/extension_prefs.cc',
        'browser/extensions/extension_prefs.h',
        'browser/extensions/extension_prefs_scope.h',
        'browser/extensions/extension_process_manager.cc',
        'browser/extensions/extension_process_manager.h',
        'browser/extensions/extension_processes_api.cc',
        'browser/extensions/extension_processes_api.h',
        'browser/extensions/extension_processes_api_constants.cc',
        'browser/extensions/extension_processes_api_constants.h',
        'browser/extensions/extension_protocols.cc',
        'browser/extensions/extension_protocols.h',
        'browser/extensions/extension_scoped_prefs.h',
        'browser/extensions/extension_service.cc',
        'browser/extensions/extension_service.h',
        'browser/extensions/extension_sorting.cc',
        'browser/extensions/extension_sorting.h',
        'browser/extensions/extension_special_storage_policy.cc',
        'browser/extensions/extension_special_storage_policy.h',
        'browser/extensions/extension_system.cc',
        'browser/extensions/extension_system.h',
        'browser/extensions/extension_sync_bundle.cc',
        'browser/extensions/extension_sync_bundle.h',
        'browser/extensions/extension_system_factory.cc',
        'browser/extensions/extension_system_factory.h',
        'browser/extensions/extension_sync_data.cc',
        'browser/extensions/extension_sync_data.h',
        'browser/extensions/extension_tab_helper.cc',
        'browser/extensions/extension_tab_helper.h',
        'browser/extensions/extension_tab_helper_delegate.cc',
        'browser/extensions/extension_tab_helper_delegate.h',
        'browser/extensions/extension_tab_id_map.cc',
        'browser/extensions/extension_tab_id_map.h',
        'browser/extensions/extension_tab_util.cc',
        'browser/extensions/extension_tab_util.h',
        'browser/extensions/extension_tabs_module_constants.cc',
        'browser/extensions/extension_tabs_module_constants.h',
        'browser/extensions/extension_toolbar_model.cc',
        'browser/extensions/extension_toolbar_model.h',
        'browser/extensions/extension_uninstall_dialog.cc',
        'browser/extensions/extension_uninstall_dialog.h',
        'browser/extensions/extension_warning_set.cc',
        'browser/extensions/extension_warning_set.h',
        'browser/extensions/extension_window_controller.cc',
        'browser/extensions/extension_window_controller.h',
        'browser/extensions/extension_window_list.cc',
        'browser/extensions/extension_window_list.h',
        'browser/extensions/extension_web_ui.cc',
        'browser/extensions/extension_web_ui.h',
        'browser/extensions/extension_webkit_preferences.cc',
        'browser/extensions/extension_webkit_preferences.h',
        'browser/extensions/extensions_quota_service.cc',
        'browser/extensions/extensions_quota_service.h',
        'browser/extensions/extensions_startup.cc',
        'browser/extensions/extensions_startup.h',
        'browser/extensions/external_extension_loader.cc',
        'browser/extensions/external_extension_loader.h',
        'browser/extensions/external_extension_provider_impl.cc',
        'browser/extensions/external_extension_provider_impl.h',
        'browser/extensions/external_extension_provider_interface.h',
        'browser/extensions/external_policy_extension_loader.cc',
        'browser/extensions/external_policy_extension_loader.h',
        'browser/extensions/external_pref_extension_loader.cc',
        'browser/extensions/external_pref_extension_loader.h',
        'browser/extensions/external_registry_extension_loader_win.cc',
        'browser/extensions/external_registry_extension_loader_win.h',
        'browser/extensions/file_reader.cc',
        'browser/extensions/file_reader.h',
        'browser/extensions/image_loading_tracker.cc',
        'browser/extensions/image_loading_tracker.h',
        'browser/extensions/installed_loader.cc',
        'browser/extensions/installed_loader.h',
        'browser/extensions/key_identifier_conversion_views.cc',
        'browser/extensions/key_identifier_conversion_views.h',
        'browser/extensions/lazy_background_task_queue.cc',
        'browser/extensions/lazy_background_task_queue.h',
        'browser/extensions/pack_extension_job.cc',
        'browser/extensions/pack_extension_job.h',
        'browser/extensions/pending_extension_info.cc',
        'browser/extensions/pending_extension_info.h',
        'browser/extensions/pending_extension_manager.cc',
        'browser/extensions/pending_extension_manager.h',
        'browser/extensions/permissions_updater.cc',
        'browser/extensions/permissions_updater.h',
        'browser/extensions/process_map.cc',
        'browser/extensions/process_map.h',
        'browser/extensions/sandboxed_extension_unpacker.cc',
        'browser/extensions/sandboxed_extension_unpacker.h',
        'browser/extensions/settings/failing_settings_storage.cc',
        'browser/extensions/settings/failing_settings_storage.h',
        'browser/extensions/settings/setting_change.cc',
        'browser/extensions/settings/setting_change.h',
        'browser/extensions/settings/setting_sync_data.cc',
        'browser/extensions/settings/setting_sync_data.h',
        'browser/extensions/settings/settings_backend.cc',
        'browser/extensions/settings/settings_backend.h',
        'browser/extensions/settings/settings_frontend.cc',
        'browser/extensions/settings/settings_frontend.h',
        'browser/extensions/settings/settings_leveldb_storage.cc',
        'browser/extensions/settings/settings_leveldb_storage.h',
        'browser/extensions/settings/settings_namespace.cc',
        'browser/extensions/settings/settings_namespace.h',
        'browser/extensions/settings/settings_observer.h',
        'browser/extensions/settings/settings_storage.cc',
        'browser/extensions/settings/settings_storage.h',
        'browser/extensions/settings/settings_storage_factory.h',
        'browser/extensions/settings/settings_storage_quota_enforcer.cc',
        'browser/extensions/settings/settings_storage_quota_enforcer.h',
        'browser/extensions/settings/settings_sync_processor.cc',
        'browser/extensions/settings/settings_sync_processor.h',
        'browser/extensions/settings/settings_sync_util.cc',
        'browser/extensions/settings/settings_sync_util.h',
        'browser/extensions/settings/syncable_settings_storage.cc',
        'browser/extensions/settings/syncable_settings_storage.h',
        'browser/extensions/settings/testing_settings_storage.cc',
        'browser/extensions/settings/testing_settings_storage.h',
        'browser/extensions/settings/weak_unlimited_settings_storage.cc',
        'browser/extensions/settings/weak_unlimited_settings_storage.h',
        'browser/extensions/theme_installed_infobar_delegate.cc',
        'browser/extensions/theme_installed_infobar_delegate.h',
        'browser/extensions/unpacked_installer.cc',
        'browser/extensions/unpacked_installer.h',
        'browser/extensions/updater/extension_downloader.cc',
        'browser/extensions/updater/extension_downloader.h',
        'browser/extensions/updater/extension_downloader_delegate.cc',
        'browser/extensions/updater/extension_downloader_delegate.h',
        'browser/extensions/updater/extension_updater.cc',
        'browser/extensions/updater/extension_updater.h',
        'browser/extensions/updater/manifest_fetch_data.cc',
        'browser/extensions/updater/manifest_fetch_data.h',
        'browser/extensions/updater/safe_manifest_parser.cc',
        'browser/extensions/updater/safe_manifest_parser.h',
        'browser/extensions/user_script_listener.cc',
        'browser/extensions/user_script_listener.h',
        'browser/extensions/user_script_master.cc',
        'browser/extensions/user_script_master.h',
        'browser/extensions/webstore_inline_installer.cc',
        'browser/extensions/webstore_inline_installer.h',
        'browser/extensions/webstore_install_helper.cc',
        'browser/extensions/webstore_install_helper.h',
        'browser/extensions/webstore_installer.cc',
        'browser/extensions/webstore_installer.h',
      ],
      'conditions': [
        ['chromeos==0', {
          'sources/': [
            ['exclude', 'browser/extensions/api/terminal/terminal_extension_helper.cc'],
            ['exclude', 'browser/extensions/api/terminal/terminal_extension_helper.h'],
            ['exclude', 'browser/extensions/api/terminal/terminal_private_api.cc'],
            ['exclude', 'browser/extensions/api/terminal/terminal_private_api.h'],
            ['exclude', 'browser/extensions/extension_input_ime_api.cc'],
            ['exclude', 'browser/extensions/extension_input_ime_api.h'],
            ['exclude', 'browser/extensions/extension_input_method_api.cc'],
            ['exclude', 'browser/extensions/extension_input_method_api.h'],
          ],
        }, {  # chromeos==1
          'dependencies': [
            '../build/linux/system.gyp:dbus-glib',
            '../third_party/libevent/libevent.gyp:libevent',
            '../third_party/mozc/chrome/chromeos/renderer/chromeos_renderer.gyp:mozc_candidates_proto',
            'browser/chromeos/input_method/input_method.gyp:gencode',
          ],
          'sources!': [
            'browser/extensions/default_apps.cc',
            'browser/extensions/default_apps.h',
          ],
        }],
        ['enable_extensions==1', {
          'sources': [
            'browser/extensions/execute_code_in_tab_function.cc',
            'browser/extensions/execute_code_in_tab_function.h',
            'browser/extensions/extension_chrome_auth_private_api.cc',
            'browser/extensions/extension_chrome_auth_private_api.h',
            'browser/extensions/extension_content_settings_api.cc',
            'browser/extensions/extension_content_settings_api.h',
            'browser/extensions/extension_context_menu_api.cc',
            'browser/extensions/extension_context_menu_api.h',
            'browser/extensions/extension_cookies_api.cc',
            'browser/extensions/extension_cookies_api.h',
            'browser/extensions/extension_debugger_api.cc',
            'browser/extensions/extension_debugger_api.h',
            'browser/extensions/extension_font_settings_api.cc',
            'browser/extensions/extension_font_settings_api.h',
            'browser/extensions/extension_i18n_api.cc',
            'browser/extensions/extension_i18n_api.h',
            'browser/extensions/extension_idle_api.cc',
            'browser/extensions/extension_idle_api.h',
            'browser/extensions/extension_input_api.cc',
            'browser/extensions/extension_input_api.h',
            'browser/extensions/extension_input_ime_api.cc',
            'browser/extensions/extension_input_ime_api.h',
            'browser/extensions/extension_input_method_api.cc',
            'browser/extensions/extension_input_method_api.h',
            'browser/extensions/extension_input_ui_api.cc',
            'browser/extensions/extension_input_ui_api.h',
            'browser/extensions/extension_managed_mode_api.cc',
            'browser/extensions/extension_managed_mode_api.h',
            'browser/extensions/extension_management_api.cc',
            'browser/extensions/extension_management_api.h',
            'browser/extensions/extension_metrics_module.cc',
            'browser/extensions/extension_metrics_module.h',
            'browser/extensions/extension_page_capture_api.cc',
            'browser/extensions/extension_page_capture_api.h',
            'browser/extensions/extension_preference_api.cc',
            'browser/extensions/extension_preference_api.h',
            'browser/extensions/extension_tabs_module.cc',
            'browser/extensions/extension_tabs_module.h',
            'browser/extensions/extension_test_api.cc',
            'browser/extensions/extension_test_api.h',
            'browser/extensions/extension_web_socket_proxy_private_api.cc',
            'browser/extensions/extension_web_socket_proxy_private_api.h',
            'browser/extensions/extension_webstore_private_api.cc',
            'browser/extensions/extension_webstore_private_api.h',
            'browser/extensions/settings/settings_api.cc',
            'browser/extensions/settings/settings_api.h',
            'browser/extensions/system/system_api.cc',
            'browser/extensions/system/system_api.h',
          ],
        }, {  # enable_extensions==0
          'sources/': [
            # Handle files in browser/extensions/api. Exclude everything by default, white list
            # files if they are needed for linking.
            # TODO: The re-includes should go away or shrink as extensions are refactored to be
            # less intertwined in the main codebase.
            ['exclude', '^browser/extensions/api/'],
            ['include', '^browser/extensions/api/declarative/initializing_rules_registry.cc'],
            ['include', '^browser/extensions/api/declarative/rules_registry.cc'],
            ['include', '^browser/extensions/api/declarative/rules_registry_service.cc'],
            ['include', '^browser/extensions/api/declarative/rules_registry_with_cache.cc'],
            ['include', '^browser/extensions/api/declarative/substring_set_matcher.cc'],
            ['include', '^browser/extensions/api/declarative/url_matcher.cc'],
            ['include', '^browser/extensions/api/declarative_webrequest/webrequest_action.cc'],
            ['include', '^browser/extensions/api/declarative_webrequest/webrequest_condition.cc'],
            ['include', '^browser/extensions/api/declarative_webrequest/webrequest_condition_attribute.cc'],
            ['include', '^browser/extensions/api/declarative_webrequest/webrequest_constants.cc'],
            ['include', '^browser/extensions/api/declarative_webrequest/webrequest_rule.cc'],
            ['include', '^browser/extensions/api/declarative_webrequest/webrequest_rules_registry.cc'],
            ['include', '^browser/extensions/api/permissions/permissions_api.cc'],
            ['include', '^browser/extensions/api/permissions/permissions_api_helpers.cc'],
            ['include', '^browser/extensions/api/proxy/proxy_api.cc'],
            ['include', '^browser/extensions/api/proxy/proxy_api_constants.cc'],
            ['include', '^browser/extensions/api/runtime/runtime_api.cc'],
            ['include', '^browser/extensions/api/web_navigation/web_navigation_api.cc'],
            ['include', '^browser/extensions/api/web_navigation/web_navigation_api_constants.cc'],
            ['include', '^browser/extensions/api/web_request/web_request_api.cc'],
            ['include', '^browser/extensions/api/web_request/web_request_api_constants.cc'],
            ['include', '^browser/extensions/api/web_request/web_request_api_helpers.cc'],
            ['include', '^browser/extensions/api/web_request/web_request_time_tracker.cc'],
          ],
        }],
        ['use_virtual_keyboard==0', {
          'sources/': [
            ['exclude', '^browser/extensions/extension_input_ui_api.*'],
          ],
        }],
        ['OS=="linux" and use_aura==1', {
          'dependencies': [
            '../build/linux/system.gyp:dbus',
            '../build/linux/system.gyp:fontconfig',
            '../build/linux/system.gyp:x11',
            '../dbus/dbus.gyp:dbus',
          ],
        }],
        ['toolkit_uses_gtk == 1', {
          'dependencies': [
            '../build/linux/system.gyp:dbus',
            '../build/linux/system.gyp:gconf',
            '../build/linux/system.gyp:gtk',
            '../build/linux/system.gyp:gtkprint',
            '../build/linux/system.gyp:ssl',
            '../build/linux/system.gyp:x11',
            '../dbus/dbus.gyp:dbus',
            '../third_party/undoview/undoview.gyp:undoview',
          ],
        }],
        ['OS=="win"', {
          'dependencies': [
            '../third_party/iaccessible2/iaccessible2.gyp:iaccessible2',
            '../third_party/isimpledom/isimpledom.gyp:isimpledom',
          ],
          'conditions': [
            ['win_use_allocator_shim==1', {
              'dependencies': [
                '<(allocator_target)',
              ],
            }],
          ],
        }, {  # 'OS!="win"
          'conditions': [
            ['OS=="linux" and toolkit_views==1',{
              'include_dirs': [
                '<(INTERMEDIATE_DIR)',
                '<(INTERMEDIATE_DIR)/chrome',
              ],
              'sources/': [
                ['include', '^browser/extensions/'],

                # Other excluded stuff.
                ['exclude', '^browser/extensions/browser_action_test_util_gtk.cc'],
                ['exclude', '^browser/extensions/extension_host_mac.h'],
                ['exclude', '^browser/extensions/extension_host_mac.mm'],
                ['exclude', '^browser/extensions/external_registry_extension_loader_win.cc'],
                ['exclude', '^browser/extensions/external_registry_extension_loader_win.h'],
              ],
            }],
            # Exclude these toolkit_views specific files again.
            # (Required because of the '^browser/extensions/' include above)
            ['toolkit_views==0', {
              'sources/': [
                ['exclude', '^browser/extensions/extension_input_api.cc'],
                ['exclude', '^browser/extensions/extension_input_api.h'],
                ['exclude', '^browser/extensions/key_identifier_conversion_views.cc'],
                ['exclude', '^browser/extensions/key_identifier_conversion_views.h'],
              ],
            }],
            # Exclude extension_input_ui_api that depends on chromeos again
            # (Required because of the '^browser/extensions/' include above)
            ['chromeos == 0 or use_virtual_keyboard == 0', {
              'sources/': [
                ['exclude', '^browser/extensions/extension_input_ui_api.cc'],
                ['exclude', '^browser/extensions/extension_input_ui_api.h'],
              ],
            }],
            ['chromeos==1',{
              'sources/': [
                ['exclude', '^browser/extensions/extension_tts_api_linux.cc'],
              ],
              'dependencies': [
                '../dbus/dbus.gyp:dbus',
                '../third_party/protobuf/protobuf.gyp:protobuf_lite',
                '../third_party/protobuf/protobuf.gyp:protoc#host',
              ],
              'conditions': [
                ['system_libcros==0', {
                  'dependencies': [
                    '../third_party/cros/cros_api.gyp:cros_api',
                  ],
                  'include_dirs': [
                    '../third_party/'
                  ],
                }],
              ],
            }],
          ],
        }],
      ],
    },
  ],
}

