// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SHADER_MANAGER_H_
#define GPU_COMMAND_BUFFER_SERVICE_SHADER_MANAGER_H_

#include <string>
#include "base/basictypes.h"
#include "base/hash_tables.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/command_buffer/service/shader_translator.h"
#include "gpu/gpu_export.h"

namespace gpu {
namespace gles2 {

// Tracks the Shaders.
//
// NOTE: To support shared resources an instance of this class will
// need to be shared by multiple GLES2Decoders.
class GPU_EXPORT ShaderManager {
 public:
  // This is used to keep the source code for a shader. This is because in order
  // to emluate GLES2 the shaders will have to be re-written before passed to
  // the underlying OpenGL. But, when the user calls glGetShaderSource they
  // should get the source they passed in, not the re-written source.
  class GPU_EXPORT ShaderInfo : public base::RefCounted<ShaderInfo> {
   public:
    typedef scoped_refptr<ShaderInfo> Ref;
    typedef ShaderTranslator::VariableInfo VariableInfo;

    void UpdateSource(const char* source) {
      // If the source is flagged as compiled, then store our previous source
      // for deferred compile and caching.
      if(!deferred_compilation_source_.get()) {
        deferred_compilation_source_.reset(source_.release());
      }
      source_.reset(source ? new std::string(source) : NULL);
      translated_source_.reset(NULL);
    }

    void UpdateTranslatedSource(const char* translated_source) {
      translated_source_.reset(
          translated_source ? new std::string(translated_source) : NULL);
    }

    GLuint service_id() const {
      return service_id_;
    }

    GLenum shader_type() const {
      return shader_type_;
    }

    const std::string* source() const {
      return source_.get();
    }

    const std::string* translated_source() const {
      return translated_source_.get();
    }

    void SetStatus(
        bool valid, const char* log,
        ShaderTranslatorInterface* translator);

    // If the source was actually compiled (compilation wasn't deferred)
    bool source_compiled() const {
      return source_compiled_;
    }

    // The source that was used when the user called CompileShader.
    // This is used for a deferred compile and in the program cache
    const std::string* deferred_compilation_source() const {
      return deferred_compilation_source_.get() != NULL ?
          deferred_compilation_source_.get() :
          source_.get();
    }

    // Resets our deferred compilation source and stores if the source was
    // actually compiled, or if we're expecting a cache hit
    void FlagSourceAsCompiled(bool actually_compiled) {
      source_compiled_ = actually_compiled;
      deferred_compilation_source_.reset();
    }

    const VariableInfo* GetAttribInfo(const std::string& name) const;
    const VariableInfo* GetUniformInfo(const std::string& name) const;

    // If the original_name is not found, return NULL.
    const std::string* GetAttribMappedName(
        const std::string& original_name) const;

    const std::string* log_info() const {
      return log_info_.get();
    }

    bool IsValid() const {
      return valid_;
    }

    bool IsDeleted() const {
      return service_id_ == 0;
    }

    bool InUse() const {
      DCHECK_GE(use_count_, 0);
      return use_count_ != 0;
    }

    // Used by program cache.
    const ShaderTranslator::VariableMap& attrib_map() const {
      return attrib_map_;
    }

    // Used by program cache.
    const ShaderTranslator::VariableMap& uniform_map() const {
      return uniform_map_;
    }

    // Used by program cache.
    void set_attrib_map(const ShaderTranslator::VariableMap& attrib_map) {
      // copied because cache might be cleared
      attrib_map_ = ShaderTranslator::VariableMap(attrib_map);
    }

    // Used by program cache.
    void set_uniform_map(const ShaderTranslator::VariableMap& uniform_map) {
      // copied because cache might be cleared
      uniform_map_ = ShaderTranslator::VariableMap(uniform_map);
    }

   private:
    typedef ShaderTranslator::VariableMap VariableMap;

    friend class base::RefCounted<ShaderInfo>;
    friend class ShaderManager;

    ShaderInfo(GLuint service_id, GLenum shader_type);
    ~ShaderInfo();

    void IncUseCount();
    void DecUseCount();
    void MarkAsDeleted();

    int use_count_;

    // The shader this ShaderInfo is tracking.
    GLuint service_id_;
    // Type of shader - GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
    GLenum shader_type_;

    // True if compilation succeeded.
    bool valid_;

    // The shader source as passed to glShaderSource.
    scoped_ptr<std::string> source_;

    // The translated shader source.
    scoped_ptr<std::string> translated_source_;

    // The shader translation log.
    scoped_ptr<std::string> log_info_;

    // The type info when the shader was last compiled.
    VariableMap attrib_map_;
    VariableMap uniform_map_;

    // If the source was actually compiled (otherwise we're deferring
    // compilation because of a possible cache hit)
    bool source_compiled_;

    // Holds on to the source for a deferred compile.
    scoped_ptr<std::string> deferred_compilation_source_;
  };

  ShaderManager();
  ~ShaderManager();

  // Must call before destruction.
  void Destroy(bool have_context);

  // Creates a shader info for the given shader ID.
  ShaderInfo* CreateShaderInfo(
      GLuint client_id,
      GLuint service_id,
      GLenum shader_type);

  // Gets an existing shader info for the given shader ID. Returns NULL if none
  // exists.
  ShaderInfo* GetShaderInfo(GLuint client_id);

  // Gets a client id for a given service id.
  bool GetClientId(GLuint service_id, GLuint* client_id) const;

  void MarkAsDeleted(ShaderInfo* info);

  // Mark a shader as used
  void UseShader(ShaderInfo* info);

  // Unmark a shader as used. If it has been deleted and is not used
  // then we free the info.
  void UnuseShader(ShaderInfo* info);

  // Check if a ShaderInfo is owned by this ShaderManager.
  bool IsOwned(ShaderInfo* info);

 private:
  // Info for each shader by service side shader Id.
  typedef base::hash_map<GLuint, ShaderInfo::Ref> ShaderInfoMap;
  ShaderInfoMap shader_infos_;

  void RemoveShaderInfoIfUnused(ShaderInfo* info);

  DISALLOW_COPY_AND_ASSIGN(ShaderManager);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SHADER_MANAGER_H_

