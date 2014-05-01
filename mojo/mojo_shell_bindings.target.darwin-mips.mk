# This file is generated by gyp; do not edit.

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := mojo_mojo_shell_bindings_gyp
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := $(TARGET_$(GYP_VAR_PREFIX)ARCH)
gyp_intermediate_dir := $(call local-intermediates-dir,,$(GYP_VAR_PREFIX))
gyp_shared_intermediate_dir := $(call intermediates-dir-for,GYP,shared,,,$(GYP_VAR_PREFIX))

# Make sure our deps are built first.
GYP_TARGET_DEPENDENCIES :=


### Generated for rule "mojo_mojo_gyp_mojo_shell_bindings_target_Generate_C___source_files_from_mojom_files":
# "{'inputs': ['../mojo/public/tools/bindings/mojom_bindings_generator.py', '../mojo/public/tools/bindings/generators/cpp_templates/enum_declaration.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/enum_traits.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/interface_declaration.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/interface_definition.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/interface_macros.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/interface_proxy_declaration.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/interface_stub_declaration.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/module.cc.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/module.h.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/module-internal.h.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/params_definition.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/struct_builder_definition.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/struct_declaration.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/struct_definition.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/struct_destructor.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/struct_macros.tmpl', '../mojo/public/tools/bindings/generators/cpp_templates/wrapper_class_declaration.tmpl', '../mojo/public/tools/bindings/generators/js_templates/enum_definition.tmpl', '../mojo/public/tools/bindings/generators/js_templates/interface_definition.tmpl', '../mojo/public/tools/bindings/generators/js_templates/module.js.tmpl', '../mojo/public/tools/bindings/generators/js_templates/struct_definition.tmpl', '../mojo/public/tools/bindings/generators/mojom_cpp_generator.py', '../mojo/public/tools/bindings/generators/mojom_js_generator.py', '../mojo/public/tools/bindings/pylib/mojom/__init__.py', '../mojo/public/tools/bindings/pylib/mojom/error.py', '../mojo/public/tools/bindings/pylib/mojom/generate/__init__.py', '../mojo/public/tools/bindings/pylib/mojom/generate/data.py', '../mojo/public/tools/bindings/pylib/mojom/generate/generator.py', '../mojo/public/tools/bindings/pylib/mojom/generate/module.py', '../mojo/public/tools/bindings/pylib/mojom/generate/pack.py', '../mojo/public/tools/bindings/pylib/mojom/generate/template_expander.py', '../mojo/public/tools/bindings/pylib/mojom/parse/__init__.py', '../mojo/public/tools/bindings/pylib/mojom/parse/ast.py', '../mojo/public/tools/bindings/pylib/mojom/parse/lexer.py', '../mojo/public/tools/bindings/pylib/mojom/parse/parser.py', '../mojo/public/tools/bindings/pylib/mojom/parse/translate.py'], 'process_outputs_as_sources': '1', 'extension': 'mojom', 'outputs': ['$(gyp_shared_intermediate_dir)/mojo/%(INPUT_DIRNAME)s/%(INPUT_ROOT)s.mojom.cc', '$(gyp_shared_intermediate_dir)/mojo/%(INPUT_DIRNAME)s/%(INPUT_ROOT)s.mojom.h', '$(gyp_shared_intermediate_dir)/mojo/%(INPUT_DIRNAME)s/%(INPUT_ROOT)s.mojom.js', '$(gyp_shared_intermediate_dir)/mojo/%(INPUT_DIRNAME)s/%(INPUT_ROOT)s.mojom-internal.h'], 'variables': {'mojom_bindings_generator': '../mojo/public/tools/bindings/mojom_bindings_generator.py'}, 'rule_name': 'Generate C++ source files from mojom files', 'rule_sources': ['public/interfaces/shell/shell.mojom'], 'action': ['python', '../mojo/public/tools/bindings/mojom_bindings_generator.py', '%(INPUT_DIRNAME)s/%(INPUT_ROOT)s.mojom', '--use_chromium_bundled_pylibs', '-d', '..', '-o', '$(gyp_shared_intermediate_dir)/mojo/%(INPUT_DIRNAME)s'], 'message': 'Generating Mojo bindings from %(INPUT_DIRNAME)s/%(INPUT_ROOT)s.mojom'}":
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc: $(LOCAL_PATH)/mojo/public/interfaces/shell/shell.mojom $(LOCAL_PATH)/mojo/public/tools/bindings/mojom_bindings_generator.py $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/enum_declaration.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/enum_traits.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/interface_declaration.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/interface_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/interface_macros.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/interface_proxy_declaration.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/interface_stub_declaration.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/module.cc.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/module.h.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/module-internal.h.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/params_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/struct_builder_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/struct_declaration.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/struct_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/struct_destructor.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/struct_macros.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/cpp_templates/wrapper_class_declaration.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/js_templates/enum_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/js_templates/interface_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/js_templates/module.js.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/js_templates/struct_definition.tmpl $(LOCAL_PATH)/mojo/public/tools/bindings/generators/mojom_cpp_generator.py $(LOCAL_PATH)/mojo/public/tools/bindings/generators/mojom_js_generator.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/__init__.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/error.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/generate/__init__.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/generate/data.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/generate/generator.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/generate/module.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/generate/pack.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/generate/template_expander.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/parse/__init__.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/parse/ast.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/parse/lexer.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/parse/parser.py $(LOCAL_PATH)/mojo/public/tools/bindings/pylib/mojom/parse/translate.py $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell; cd $(gyp_local_path)/mojo; python ../mojo/public/tools/bindings/mojom_bindings_generator.py public/interfaces/shell/shell.mojom --use_chromium_bundled_pylibs -d .. -o "$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell"

$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.h: $(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc ;
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.js: $(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc ;
$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom-internal.h: $(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc ;


GYP_GENERATED_OUTPUTS := \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.h \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.js \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom-internal.h

# Make sure our deps and generated files are built first.
LOCAL_ADDITIONAL_DEPENDENCIES := $(GYP_TARGET_DEPENDENCIES) $(GYP_GENERATED_OUTPUTS)

LOCAL_CPP_EXTENSION := .cc
$(gyp_intermediate_dir)/shell.mojom.cc: $(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.cc
	mkdir -p $(@D); cp $< $@
LOCAL_GENERATED_SOURCES := \
	$(gyp_intermediate_dir)/shell.mojom.cc \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.h \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom.js \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell/shell.mojom-internal.h

GYP_COPIED_SOURCE_ORIGIN_DIRS := \
	$(gyp_shared_intermediate_dir)/mojo/public/interfaces/shell

LOCAL_SRC_FILES :=


# Flags passed to both C and C++ files.
MY_CFLAGS_Debug := \
	-fstack-protector \
	--param=ssp-buffer-size=4 \
	 \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-EL \
	-mhard-float \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-U_FORTIFY_SOURCE \
	-Wno-extra \
	-Wno-ignored-qualifiers \
	-Wno-type-limits \
	-Wno-unused-but-set-variable \
	-Os \
	-g \
	-fomit-frame-pointer \
	-fdata-sections \
	-ffunction-sections \
	-funwind-tables

MY_DEFS_Debug := \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DBLINK_SCALE_FILTERS_AT_RECORD_TIME' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_LIBJPEG_TURBO=1' \
	'-DENABLE_WEBRTC=1' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DDISCARDABLE_MEMORY_ALWAYS_SUPPORTED_NATIVELY' \
	'-DSYSTEM_NATIVELY_SIGNALS_MEMORY_PRESSURE' \
	'-DENABLE_EGLIMAGE=1' \
	'-DCLD_VERSION=1' \
	'-DENABLE_PRINTING=1' \
	'-DENABLE_MANAGED_USERS=1' \
	'-DUSE_OPENSSL=1' \
	'-DUSE_OPENSSL_CERTS=1' \
	'-D__STDC_CONSTANT_MACROS' \
	'-D__STDC_FORMAT_MACROS' \
	'-DANDROID' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DCHROME_BUILD_ID=""' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=1' \
	'-DWTF_USE_DYNAMIC_ANNOTATIONS=1' \
	'-D_DEBUG'


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Debug := \
	$(LOCAL_PATH) \
	$(gyp_shared_intermediate_dir) \
	$(PWD)/frameworks/wilhelm/include \
	$(PWD)/bionic \
	$(PWD)/external/stlport/stlport


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Debug := \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Wno-uninitialized \
	-Wno-non-virtual-dtor \
	-Wno-sign-promo


# Flags passed to both C and C++ files.
MY_CFLAGS_Release := \
	-fstack-protector \
	--param=ssp-buffer-size=4 \
	 \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-EL \
	-mhard-float \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fstack-protector \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-U_FORTIFY_SOURCE \
	-Wno-extra \
	-Wno-ignored-qualifiers \
	-Wno-type-limits \
	-Wno-unused-but-set-variable \
	-Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer \
	-funwind-tables

MY_DEFS_Release := \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DBLINK_SCALE_FILTERS_AT_RECORD_TIME' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_LIBJPEG_TURBO=1' \
	'-DENABLE_WEBRTC=1' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DDISCARDABLE_MEMORY_ALWAYS_SUPPORTED_NATIVELY' \
	'-DSYSTEM_NATIVELY_SIGNALS_MEMORY_PRESSURE' \
	'-DENABLE_EGLIMAGE=1' \
	'-DCLD_VERSION=1' \
	'-DENABLE_PRINTING=1' \
	'-DENABLE_MANAGED_USERS=1' \
	'-DUSE_OPENSSL=1' \
	'-DUSE_OPENSSL_CERTS=1' \
	'-D__STDC_CONSTANT_MACROS' \
	'-D__STDC_FORMAT_MACROS' \
	'-DANDROID' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DCHROME_BUILD_ID=""' \
	'-DNDEBUG' \
	'-DNVALGRIND' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0' \
	'-D_FORTIFY_SOURCE=2'


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Release := \
	$(LOCAL_PATH) \
	$(gyp_shared_intermediate_dir) \
	$(PWD)/frameworks/wilhelm/include \
	$(PWD)/bionic \
	$(PWD)/external/stlport/stlport


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Release := \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Wno-uninitialized \
	-Wno-non-virtual-dtor \
	-Wno-sign-promo


LOCAL_CFLAGS := $(MY_CFLAGS_$(GYP_CONFIGURATION)) $(MY_DEFS_$(GYP_CONFIGURATION))
LOCAL_C_INCLUDES := $(GYP_COPIED_SOURCE_ORIGIN_DIRS) $(LOCAL_C_INCLUDES_$(GYP_CONFIGURATION))
LOCAL_CPPFLAGS := $(LOCAL_CPPFLAGS_$(GYP_CONFIGURATION))
LOCAL_ASFLAGS := $(LOCAL_CFLAGS)
### Rules for final target.

LOCAL_LDFLAGS_Debug := \
	-Wl,-z,now \
	-Wl,-z,relro \
	-Wl,--fatal-warnings \
	-Wl,-z,noexecstack \
	-fPIC \
	-EL \
	-Wl,--no-keep-memory \
	-nostdlib \
	-Wl,--no-undefined \
	-Wl,--exclude-libs=ALL \
	-Wl,--warn-shared-textrel \
	-Wl,-O1 \
	-Wl,--as-needed


LOCAL_LDFLAGS_Release := \
	-Wl,-z,now \
	-Wl,-z,relro \
	-Wl,--fatal-warnings \
	-Wl,-z,noexecstack \
	-fPIC \
	-EL \
	-Wl,--no-keep-memory \
	-nostdlib \
	-Wl,--no-undefined \
	-Wl,--exclude-libs=ALL \
	-Wl,-O1 \
	-Wl,--as-needed \
	-Wl,--gc-sections \
	-Wl,--warn-shared-textrel


LOCAL_LDFLAGS := $(LOCAL_LDFLAGS_$(GYP_CONFIGURATION))

LOCAL_STATIC_LIBRARIES :=

# Enable grouping to fix circular references
LOCAL_GROUP_STATIC_LIBRARIES := true

LOCAL_SHARED_LIBRARIES := \
	libstlport \
	libdl

# Add target alias to "gyp_all_modules" target.
.PHONY: gyp_all_modules
gyp_all_modules: mojo_mojo_shell_bindings_gyp

# Alias gyp target name.
.PHONY: mojo_shell_bindings
mojo_shell_bindings: mojo_mojo_shell_bindings_gyp

include $(BUILD_STATIC_LIBRARY)
