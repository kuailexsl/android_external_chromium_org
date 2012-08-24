# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'ppapi_proxy_target': 0,
    },
    'target_conditions': [
      # This part is shared between the targets defined below.
      ['ppapi_proxy_target==1', {
        'sources': [
          # Take some standalone files from the C++ wrapper allowing us to more
          # easily make async callbacks in the proxy. We can't depend on the
          # full C++ wrappers at this layer since the C++ wrappers expect
          # symbols defining the globals for "being a plugin" which we are not.
          # These callback files are standalone.
          'cpp/completion_callback.h',
          'utility/completion_callback_factory.h',

          'proxy/broker_dispatcher.cc',
          'proxy/broker_dispatcher.h',
          'proxy/connection.h',
          'proxy/dispatcher.cc',
          'proxy/dispatcher.h',
          'proxy/enter_proxy.h',
          'proxy/file_chooser_resource.cc',
          'proxy/file_chooser_resource.h',
          'proxy/gamepad_resource.cc',
          'proxy/gamepad_resource.h',
          'proxy/host_dispatcher.cc',
          'proxy/host_dispatcher.h',
          'proxy/host_var_serialization_rules.cc',
          'proxy/host_var_serialization_rules.h',
          'proxy/interface_list.cc',
          'proxy/interface_list.h',
          'proxy/interface_proxy.cc',
          'proxy/interface_proxy.h',
          'proxy/pepper_file_messages.cc',
          'proxy/pepper_file_messages.h',
          'proxy/plugin_array_buffer_var.cc',
          'proxy/plugin_array_buffer_var.h',
          'proxy/plugin_dispatcher.cc',
          'proxy/plugin_dispatcher.h',
          'proxy/plugin_globals.cc',
          'proxy/plugin_globals.h',
          'proxy/plugin_main_nacl.cc',
          'proxy/plugin_message_filter.cc',
          'proxy/plugin_message_filter.h',
          'proxy/plugin_resource.cc',
          'proxy/plugin_resource.h',
          'proxy/plugin_resource_tracker.cc',
          'proxy/plugin_resource_tracker.h',
          'proxy/plugin_var_serialization_rules.cc',
          'proxy/plugin_var_serialization_rules.h',
          'proxy/plugin_var_tracker.cc',
          'proxy/plugin_var_tracker.h',
          'proxy/ppapi_messages.cc',
          'proxy/ppapi_messages.h',
          'proxy/ppapi_command_buffer_proxy.h',
          'proxy/ppapi_command_buffer_proxy.cc',
          'proxy/ppapi_param_traits.cc',
          'proxy/ppapi_param_traits.h',
          'proxy/ppb_audio_input_proxy.cc',
          'proxy/ppb_audio_input_proxy.h',
          'proxy/ppb_audio_proxy.cc',
          'proxy/ppb_audio_proxy.h',
          'proxy/ppb_broker_proxy.cc',
          'proxy/ppb_broker_proxy.h',
          'proxy/ppb_buffer_proxy.cc',
          'proxy/ppb_buffer_proxy.h',
          'proxy/ppb_core_proxy.cc',
          'proxy/ppb_core_proxy.h',
          'proxy/ppb_file_io_proxy.cc',
          'proxy/ppb_file_io_proxy.h',
          'proxy/ppb_file_ref_proxy.cc',
          'proxy/ppb_file_ref_proxy.h',
          'proxy/ppb_file_system_proxy.cc',
          'proxy/ppb_file_system_proxy.h',
          'proxy/ppb_flash_device_id_proxy.cc',
          'proxy/ppb_flash_device_id_proxy.h',
          'proxy/ppb_flash_proxy.cc',
          'proxy/ppb_flash_proxy.h',
          'proxy/ppb_flash_menu_proxy.cc',
          'proxy/ppb_flash_menu_proxy.h',
          'proxy/ppb_flash_message_loop_proxy.cc',
          'proxy/ppb_flash_message_loop_proxy.h',
          'proxy/ppb_graphics_2d_proxy.cc',
          'proxy/ppb_graphics_2d_proxy.h',
          'proxy/ppb_graphics_3d_proxy.cc',
          'proxy/ppb_graphics_3d_proxy.h',
          'proxy/ppb_host_resolver_private_proxy.cc',
          'proxy/ppb_host_resolver_private_proxy.h',
          'proxy/ppb_image_data_proxy.cc',
          'proxy/ppb_image_data_proxy.h',
          'proxy/ppb_instance_proxy.cc',
          'proxy/ppb_instance_proxy.h',
          'proxy/ppb_message_loop_proxy.cc',
          'proxy/ppb_message_loop_proxy.h',
          'proxy/ppb_network_monitor_private_proxy.cc',
          'proxy/ppb_network_monitor_private_proxy.h',
          'proxy/ppb_pdf_proxy.cc',
          'proxy/ppb_pdf_proxy.h',
          'proxy/ppb_talk_private_proxy.cc',
          'proxy/ppb_talk_private_proxy.h',
          'proxy/ppb_tcp_server_socket_private_proxy.cc',
          'proxy/ppb_tcp_server_socket_private_proxy.h',
          'proxy/ppb_tcp_socket_private_proxy.cc',
          'proxy/ppb_tcp_socket_private_proxy.h',
          'proxy/ppb_testing_proxy.cc',
          'proxy/ppb_testing_proxy.h',
          'proxy/ppb_udp_socket_private_proxy.cc',
          'proxy/ppb_udp_socket_private_proxy.h',
          'proxy/ppb_url_loader_proxy.cc',
          'proxy/ppb_url_loader_proxy.h',
          'proxy/ppb_url_response_info_proxy.cc',
          'proxy/ppb_url_response_info_proxy.h',
          'proxy/ppb_var_deprecated_proxy.cc',
          'proxy/ppb_var_deprecated_proxy.h',
          'proxy/ppb_video_capture_proxy.cc',
          'proxy/ppb_video_capture_proxy.h',
          'proxy/ppb_video_decoder_proxy.cc',
          'proxy/ppb_video_decoder_proxy.h',
          'proxy/ppb_x509_certificate_private_proxy.cc',
          'proxy/ppb_x509_certificate_private_proxy.h',
          'proxy/ppp_class_proxy.cc',
          'proxy/ppp_class_proxy.h',
          'proxy/ppp_content_decryptor_private_proxy.cc',
          'proxy/ppp_content_decryptor_private_proxy.h',
          'proxy/ppp_graphics_3d_proxy.cc',
          'proxy/ppp_graphics_3d_proxy.h',
          'proxy/ppp_input_event_proxy.cc',
          'proxy/ppp_input_event_proxy.h',
          'proxy/ppp_instance_private_proxy.cc',
          'proxy/ppp_instance_private_proxy.h',
          'proxy/ppp_instance_proxy.cc',
          'proxy/ppp_instance_proxy.h',
          'proxy/ppp_messaging_proxy.cc',
          'proxy/ppp_messaging_proxy.h',
          'proxy/ppp_mouse_lock_proxy.cc',
          'proxy/ppp_mouse_lock_proxy.h',
          'proxy/ppp_printing_proxy.cc',
          'proxy/ppp_printing_proxy.h',
          'proxy/ppp_text_input_proxy.cc',
          'proxy/ppp_text_input_proxy.h',
          'proxy/ppp_video_decoder_proxy.cc',
          'proxy/ppp_video_decoder_proxy.h',
          'proxy/proxy_array_output.cc',
          'proxy/proxy_array_output.h',
          'proxy/proxy_channel.cc',
          'proxy/proxy_channel.h',
          'proxy/proxy_completion_callback_factory.h',
          'proxy/proxy_module.cc',
          'proxy/proxy_module.h',
          'proxy/proxy_object_var.cc',
          'proxy/proxy_object_var.h',
          'proxy/resource_creation_proxy.cc',
          'proxy/resource_creation_proxy.h',
          'proxy/resource_message_params.cc',
          'proxy/resource_message_params.h',
          'proxy/serialized_flash_menu.cc',
          'proxy/serialized_flash_menu.h',
          'proxy/serialized_structs.cc',
          'proxy/serialized_structs.h',
          'proxy/serialized_var.cc',
          'proxy/serialized_var.h',
          'proxy/var_serialization_rules.h',
        ],
        'defines': [
          'PPAPI_PROXY_IMPLEMENTATION',
        ],
        'include_dirs': [
          '..',
        ],
        'target_conditions': [
          ['>(nacl_untrusted_build)==1', {
            'sources!': [
              'proxy/broker_dispatcher.cc',
              'proxy/ppb_audio_input_proxy.cc',
              'proxy/ppb_broker_proxy.cc',
              'proxy/ppb_buffer_proxy.cc',
              'proxy/ppb_file_chooser_proxy.cc',
              'proxy/ppb_flash_device_id_proxy.cc',
              'proxy/ppb_flash_proxy.cc',
              'proxy/ppb_flash_menu_proxy.cc',
              'proxy/ppb_flash_message_loop_proxy.cc',
              'proxy/ppb_host_resolver_private_proxy.cc',
              'proxy/ppb_network_monitor_private_proxy.cc',
              'proxy/ppb_pdf_proxy.cc',
              'proxy/ppb_talk_private_proxy.cc',
              'proxy/ppb_tcp_server_socket_private_proxy.cc',
              'proxy/ppb_tcp_socket_private_proxy.cc',
              'proxy/ppb_testing_proxy.cc',
              'proxy/ppb_udp_socket_private_proxy.cc',
              'proxy/ppb_video_capture_proxy.cc',
              'proxy/ppb_video_decoder_proxy.cc',
              'proxy/ppb_x509_certificate_private_proxy.cc',
              'proxy/ppp_content_decryptor_private_proxy.cc',
              'proxy/ppp_instance_private_proxy.cc',
              'proxy/ppp_video_decoder_proxy.cc',
              'proxy/serialized_flash_menu.cc',
            ],
          }],
        ],
      }],
    ],
  },
}
