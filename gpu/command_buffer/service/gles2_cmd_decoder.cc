// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gles2_cmd_decoder.h"

#include <stdio.h>

#include <vector>
#include <string>
#include <map>
#include <build/build_config.h>  // NOLINT
#include "base/linked_ptr.h"
#include "base/scoped_ptr.h"
#define GLES2_GPU_SERVICE 1
#include "gpu/command_buffer/service/buffer_manager.h"
#include "gpu/command_buffer/common/gles2_cmd_format.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/service/cmd_buffer_engine.h"
#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/command_buffer/service/gles2_cmd_validation.h"
#include "gpu/command_buffer/service/id_manager.h"
#include "gpu/command_buffer/service/program_manager.h"
#include "gpu/command_buffer/service/shader_manager.h"
#include "gpu/command_buffer/service/texture_manager.h"
#if defined(UNIT_TEST)
#elif defined(OS_LINUX)
// XWindowWrapper is stubbed out for unit-tests.
#include "gpu/command_buffer/service/x_utils.h"
#elif defined(OS_MACOSX)
// The following two #includes CAN NOT go above the inclusion of
// gl_utils.h and therefore glew.h regardless of what the Google C++
// style guide says.
#include <CoreFoundation/CoreFoundation.h>  // NOLINT
#include <OpenGL/OpenGL.h>  // NOLINT
#include "base/scoped_cftyperef.h"
#include "chrome/common/io_surface_support_mac.h"
#endif

namespace gpu {
namespace gles2 {

// Check that certain assumptions the code makes are true. There are places in
// the code where shared memory is passed direclty to GL. Example, glUniformiv,
// glShaderSource. The command buffer code assumes GLint and GLsizei (and maybe
// a few others) are 32bits. If they are not 32bits the code will have to change
// to call those GL functions with service side memory and then copy the results
// to shared memory, converting the sizes.
COMPILE_ASSERT(sizeof(GLint) == sizeof(uint32),  // NOLINT
               GLint_not_same_size_as_uint32);
COMPILE_ASSERT(sizeof(GLsizei) == sizeof(uint32),  // NOLINT
               GLint_not_same_size_as_uint32);
COMPILE_ASSERT(sizeof(GLfloat) == sizeof(float),  // NOLINT
               GLfloat_not_same_size_as_float);

// TODO(kbr): the use of this anonymous namespace core dumps the
// linker on Mac OS X 10.6 when the symbol ordering file is used
// namespace {

static size_t GetGLTypeSize(GLenum type) {
  switch (type) {
    case GL_BYTE:
      return sizeof(GLbyte);  // NOLINT
    case GL_UNSIGNED_BYTE:
      return sizeof(GLubyte);  // NOLINT
    case GL_SHORT:
      return sizeof(GLshort);  // NOLINT
    case GL_UNSIGNED_SHORT:
      return sizeof(GLushort);  // NOLINT
    case GL_FLOAT:
      return sizeof(GLfloat);  // NOLINT
    default:
      return 0;
  }
}

// Returns the address of the first byte after a struct.
template <typename T>
const void* AddressAfterStruct(const T& pod) {
  return reinterpret_cast<const uint8*>(&pod) + sizeof(pod);
}

// Returns the address of the frst byte after the struct or NULL if size >
// immediate_data_size.
template <typename RETURN_TYPE, typename COMMAND_TYPE>
RETURN_TYPE GetImmediateDataAs(const COMMAND_TYPE& pod,
                               uint32 size,
                               uint32 immediate_data_size) {
  return (size <= immediate_data_size) ?
      static_cast<RETURN_TYPE>(const_cast<void*>(AddressAfterStruct(pod))) :
      NULL;
}

// Computes the data size for certain gl commands like glUniform.
uint32 ComputeImmediateDataSize(
    uint32 immediate_data_size,
    GLuint count,
    size_t size,
    unsigned int elements_per_unit) {
  return count * size * elements_per_unit;
}

// A struct to hold info about each command.
struct CommandInfo {
  int arg_flags;  // How to handle the arguments for this command
  int arg_count;  // How many arguments are expected for this command.
};

// A table of CommandInfo for all the commands.
const CommandInfo g_command_info[] = {
  #define GLES2_CMD_OP(name) {                                            \
    name::kArgFlags,                                                      \
    sizeof(name) / sizeof(CommandBufferEntry) - 1, },  /* NOLINT */       \

  GLES2_COMMAND_LIST(GLES2_CMD_OP)

  #undef GLES2_CMD_OP
};

namespace GLErrorBit {
enum GLErrorBit {
  kNoError = 0,
  kInvalidEnum = (1 << 0),
  kInvalidValue = (1 << 1),
  kInvalidOperation = (1 << 2),
  kOutOfMemory = (1 << 3),
  kInvalidFrameBufferOperation = (1 << 4),
};
}

uint32 GLErrorToErrorBit(GLenum error) {
  switch (error) {
    case GL_INVALID_ENUM:
      return GLErrorBit::kInvalidEnum;
    case GL_INVALID_VALUE:
      return GLErrorBit::kInvalidValue;
    case GL_INVALID_OPERATION:
      return GLErrorBit::kInvalidOperation;
    case GL_OUT_OF_MEMORY:
      return GLErrorBit::kOutOfMemory;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return GLErrorBit::kInvalidFrameBufferOperation;
    default:
      DCHECK(false);
      return GLErrorBit::kNoError;
  }
}

GLenum GLErrorBitToGLError(uint32 error_bit) {
  switch (error_bit) {
    case GLErrorBit::kInvalidEnum:
      return GL_INVALID_ENUM;
    case GLErrorBit::kInvalidValue:
      return GL_INVALID_VALUE;
    case GLErrorBit::kInvalidOperation:
      return GL_INVALID_OPERATION;
    case GLErrorBit::kOutOfMemory:
      return GL_OUT_OF_MEMORY;
    case GLErrorBit::kInvalidFrameBufferOperation:
      return GL_INVALID_FRAMEBUFFER_OPERATION;
    default:
      DCHECK(false);
      return GL_NO_ERROR;
  }
}

// }  // anonymous namespace.

#if defined(UNIT_TEST)
GLES2Decoder::GLES2Decoder()
    : debug_(false) {
#elif defined(OS_LINUX)
GLES2Decoder::GLES2Decoder()
    : debug_(false),
      window_(NULL) {
#elif defined(OS_WIN)
GLES2Decoder::GLES2Decoder()
    : debug_(false),
      hwnd_(NULL) {
#else
GLES2Decoder::GLES2Decoder()
    : debug_(false) {
#endif
}

// This class implements GLES2Decoder so we don't have to expose all the GLES2
// cmd stuff to outside this class.
class GLES2DecoderImpl : public GLES2Decoder {
 public:
  // Info about Vertex Attributes. This is used to track what the user currently
  // has bound on each Vertex Attribute so that checking can be done at
  // glDrawXXX time.
  class VertexAttribInfo {
   public:
    VertexAttribInfo()
        : enabled_(false),
          size_(0),
          type_(0),
          offset_(0),
          real_stride_(0),
          buffer_(0),
          buffer_size_(0),
          num_elements_(0) {
    }
    // Returns true if this VertexAttrib can access index.
    bool CanAccess(GLuint index);

    void set_enabled(bool enabled) {
      enabled_ = enabled;
    }

    GLuint buffer() const {
      return buffer_;
    }

    GLsizei offset() const {
      return offset_;
    }

    void Clear() {
      buffer_ = 0;
      SetBufferSize(0);
    }

    void SetBufferSize(GLsizeiptr buffer_size) {
      buffer_size_ = buffer_size;
      if (offset_ > buffer_size || real_stride_ == 0) {
        num_elements_ = 0;
      } else {
        uint32 usable_size = buffer_size - offset_;
        num_elements_ = usable_size / real_stride_ +
            ((usable_size % real_stride_) >=
             (GetGLTypeSize(type_) * size_) ? 1 : 0);
      }
    }

    void SetInfo(
        GLuint buffer,
        GLsizeiptr buffer_size,
        GLint size,
        GLenum type,
        GLsizei real_stride,
        GLsizei offset) {
      DCHECK_GT(real_stride, 0);
      buffer_ = buffer;
      size_ = size;
      type_ = type;
      real_stride_ = real_stride;
      offset_ = offset;
      SetBufferSize(buffer_size);
    }

   private:
    // Whether or not this attribute is enabled.
    bool enabled_;

    // number of components (1, 2, 3, 4)
    GLint size_;

    // GL_BYTE, GL_FLOAT, etc. See glVertexAttribPointer.
    GLenum type_;

    // The offset into the buffer.
    GLsizei offset_;

    // The stride that will be used to access the buffer. This is the actual
    // stide, NOT the GL bogus stride. In other words there is never a stride
    // of 0.
    GLsizei real_stride_;

    // The service side name of the buffer bound to this attribute. 0 = invalid
    GLuint buffer_;

    // The size of the buffer.
    GLsizeiptr buffer_size_;

    // The number of elements that can be accessed.
    GLuint num_elements_;
  };

  GLES2DecoderImpl();

  // Overridden from AsyncAPIInterface.
  virtual Error DoCommand(unsigned int command,
                               unsigned int arg_count,
                               const void* args);

  // Overridden from AsyncAPIInterface.
  virtual const char* GetCommandName(unsigned int command_id) const;

  // Overridden from GLES2Decoder.
  virtual bool Initialize();
  virtual void Destroy();
  virtual bool MakeCurrent();
  virtual uint32 GetServiceIdForTesting(uint32 client_id);

#if defined(OS_MACOSX)
  // Overridden from GLES2Decoder.
  virtual uint64 SetWindowSize(int32 width, int32 height);
#endif

  virtual void SetSwapBuffersCallback(Callback0::Type* callback);

 private:
  friend void GLGenTexturesHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids);
  friend void GLDeleteTexturesHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids);
  friend void GLGenBuffersHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids);
  friend void GLDeleteBuffersHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids);

  bool InitPlatformSpecific();
  bool InitGlew();

  // Template to help call glGenXXX functions.
  template <void gl_gen_function(GLES2DecoderImpl*, GLsizei, GLuint*)>
  bool GenGLObjects(GLsizei n, const GLuint* client_ids) {
    if (n < 0) {
      SetGLError(GL_INVALID_VALUE);
      return true;
    }
    if (!ValidateIdsAreUnused(n, client_ids)) {
      return false;
    }
    scoped_array<GLuint>temp(new GLuint[n]);
    gl_gen_function(this, n, temp.get());
    return RegisterObjects(n, client_ids, temp.get());
  }

  // Template to help call glDeleteXXX functions.
  template <void gl_delete_function(GLES2DecoderImpl*, GLsizei, GLuint*)>
  bool DeleteGLObjects(GLsizei n, const GLuint* client_ids) {
    scoped_array<GLuint>temp(new GLuint[n]);
    UnregisterObjects(n, client_ids, temp.get());
    gl_delete_function(this, n, temp.get());
    return true;
  }

  // Check that the given ids are not used.
  bool ValidateIdsAreUnused(GLsizei n, const GLuint* client_ids);

  // Register client ids with generated service ids.
  bool RegisterObjects(
      GLsizei n, const GLuint* client_ids, const GLuint* service_ids);

  // Unregisters client ids with service ids.
  void UnregisterObjects(
    GLsizei n, const GLuint* client_ids, GLuint* service_ids);

  // Creates a TextureInfo for the given texture.
  void CreateTextureInfo(GLuint texture) {
    texture_manager_->CreateTextureInfo(texture);
  }

  // Gets the texture info for the given texture. Returns NULL if none exists.
  TextureManager::TextureInfo* GetTextureInfo(GLuint texture) {
    return texture_manager_->GetTextureInfo(texture);
  }

  // Deletes the texture info for the given texture.
  void RemoveTextureInfo(GLuint texture);

  // Wrapper for CompressedTexImage2D commands.
  error::Error DoCompressedTexImage2D(
    GLenum target,
    GLint level,
    GLenum internal_format,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei image_size,
    const void* data);

  // Wrapper for TexImage2D commands.
  error::Error DoTexImage2D(
    GLenum target,
    GLint level,
    GLenum internal_format,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    const void* pixels,
    uint32 pixels_size);

  // Creates a ProgramInfo for the given program.
  void CreateProgramInfo(GLuint program) {
    program_manager_->CreateProgramInfo(program);
  }

  // Gets the program info for the given program. Returns NULL if none exists.
  // Programs that have no had glLinkProgram succesfully called on them will
  // not exist.
  ProgramManager::ProgramInfo* GetProgramInfo(GLuint program) {
    return program_manager_->GetProgramInfo(program);
  }

  // Deletes the program info for the given program.
  void RemoveProgramInfo(GLuint program) {
    program_manager_->RemoveProgramInfo(program);
  }

  // Creates a ShaderInfo for the given shader.
  void CreateShaderInfo(GLuint shader) {
    shader_manager_->CreateShaderInfo(shader);
  }

  // Gets the shader info for the given shader. Returns NULL if none exists.
  ShaderManager::ShaderInfo* GetShaderInfo(GLuint shader) {
    return shader_manager_->GetShaderInfo(shader);
  }

  // Deletes the shader info for the given shader.
  void RemoveShaderInfo(GLuint shader) {
    shader_manager_->RemoveShaderInfo(shader);
  }

  // Creates a buffer info for the given buffer.
  void CreateBufferInfo(GLuint buffer) {
    return buffer_manager_->CreateBufferInfo(buffer);
  }

  // Helper for glShaderSource.
  error::Error ShaderSourceHelper(
      GLuint shader, const char* data, uint32 data_size);

  // Gets the buffer info for the given buffer.
  BufferManager::BufferInfo* GetBufferInfo(GLuint buffer) {
    return buffer_manager_->GetBufferInfo(buffer);
  }

  // Removes any buffers in the VertexAtrribInfos and BufferInfos. This is used
  // on glDeleteBuffers so we can make sure the user does not try to render
  // with deleted buffers.
  void RemoveBufferInfo(GLuint buffer_id);

  // Update VertexAttribInfo.
  void UpdateVertexAttribInfo(GLuint buffer, GLsizeiptr size);

  // Wrapper for glCreateProgram
  void CreateProgramHelper(GLuint client_id);

  // Wrapper for glCreateShader
  void CreateShaderHelper(GLenum type, GLuint client_id);

  // Wrapper for glBindBuffer since we need to track the current targets.
  void DoBindBuffer(GLenum target, GLuint buffer);

  // Wrapper for glBindTexture since we need to track the current targets.
  void DoBindTexture(GLenum target, GLuint texture);

  // Wrapper for glCompileShader.
  void DoCompileShader(GLuint shader);

  // Wrapper for glDrawArrays.
  void DoDrawArrays(GLenum mode, GLint first, GLsizei count);

  // Wrapper for glDisableVertexAttribArray.
  void DoDisableVertexAttribArray(GLuint index);

  // Wrapper for glEnableVertexAttribArray.
  void DoEnableVertexAttribArray(GLuint index);

  // Wrapper for glGenerateMipmap
  void DoGenerateMipmap(GLenum target);

  // Wrapper for glGetShaderSource.
  void DoGetShaderSource(
      GLuint shader, GLsizei bufsize, GLsizei* length, char* dst);

  // Wrapper for glLinkProgram
  void DoLinkProgram(GLuint program);

  // Swaps the buffers (copies/renders to the current window).
  void DoSwapBuffers();

  // Wrapper for glUseProgram
  void DoUseProgram(GLuint program);

  // Gets the GLError through our wrapper.
  GLenum GetGLError();

  // Sets our wrapper for the GLError.
  void SetGLError(GLenum error);

  // Copies the real GL errors to the wrapper. This is so we can
  // make sure there are no native GL errors before calling some GL function
  // so that on return we know any error generated was for that specific
  // command.
  void CopyRealGLErrorsToWrapper();

  // Checks if the current program and vertex attributes are valid for drawing.
  bool IsDrawValid(GLuint max_vertex_accessed);

  // Gets the buffer id for a given target.
  GLuint GetBufferForTarget(GLenum target) {
    DCHECK(target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER);
    return target == GL_ARRAY_BUFFER ? bound_array_buffer_ :
                                       bound_element_array_buffer_;
  }

  // Gets the texture id for a given target.
  GLuint GetTextureForTarget(GLenum target) {
    switch (target) {
      case GL_TEXTURE_2D:
        return bound_texture_2d_;
      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return bound_texture_cube_map_;
      default:
        NOTREACHED();
        return 0;
    }
  }

  // Validates the program and location for a glGetUniform call and returns
  // a SizeResult setup to receive the result. Returns true if glGetUniform
  // should be called.
  bool GetUniformSetup(
      GLuint program, GLint location,
      uint32 shm_id, uint32 shm_offset,
      error::Error* error, GLuint* service_id, void** result);

  // Generate a member function prototype for each command in an automated and
  // typesafe way.
  #define GLES2_CMD_OP(name) \
     Error Handle ## name(             \
       uint32 immediate_data_size,          \
       const gles2::name& args);            \

  GLES2_COMMAND_LIST(GLES2_CMD_OP)

  #undef GLES2_CMD_OP

  // Current GL error bits.
  uint32 error_bits_;

  // Map of client ids to GL ids.
  scoped_ptr<IdManager> id_manager_;

  // Util to help with GL.
  GLES2Util util_;

  // pack alignment as last set by glPixelStorei
  GLint pack_alignment_;

  // unpack alignment as last set by glPixelStorei
  GLint unpack_alignment_;

  // The currently bound array buffer. If this is 0 it is illegal to call
  // glVertexAttribPointer.
  GLuint bound_array_buffer_;

  // The currently bound element array buffer. If this is 0 it is illegal
  // to call glDrawElements.
  GLuint bound_element_array_buffer_;

  // texture currently bound to GL_TEXTURE_2D with glBindTexture
  GLuint bound_texture_2d_;

  // texture currently bound to GL_TEXTURE_CUBE_MAP with glBindTexture
  GLuint bound_texture_cube_map_;

  // The maximum vertex attributes.
  GLuint max_vertex_attribs_;

  // Info for each vertex attribute saved so we can check at glDrawXXX time
  // if it is safe to draw.
  scoped_array<VertexAttribInfo> vertex_attrib_infos_;

  scoped_ptr<BufferManager> buffer_manager_;

  scoped_ptr<TextureManager> texture_manager_;

  scoped_ptr<ProgramManager> program_manager_;

  scoped_ptr<ShaderManager> shader_manager_;

  // The program in use by glUseProgram
  GLuint current_program_;

#if defined(UNIT_TEST)
#elif defined(OS_WIN)
  HDC device_context_;
  HGLRC gl_context_;
#elif defined(OS_MACOSX)
  CGLContextObj gl_context_;
  CGLPBufferObj pbuffer_;
  scoped_cftyperef<CFTypeRef> io_surface_;
  int32 surface_width_;
  int32 surface_height_;
  GLuint texture_;
  GLuint fbo_;
  GLuint depth_renderbuffer_;
  // For tracking whether the default framebuffer / renderbuffer or
  // ones created by the end user are currently bound
  GLuint bound_fbo_;
  GLuint bound_renderbuffer_;
#endif

  bool anti_aliased_;

  scoped_ptr<Callback0::Type> swap_buffers_callback_;

  DISALLOW_COPY_AND_ASSIGN(GLES2DecoderImpl);
};

GLES2Decoder* GLES2Decoder::Create() {
  return new GLES2DecoderImpl();
}

GLES2DecoderImpl::GLES2DecoderImpl()
    : GLES2Decoder(),
      error_bits_(0),
      util_(0),  // TODO(gman): Set to actual num compress texture formats.
      pack_alignment_(4),
      unpack_alignment_(4),
      bound_array_buffer_(0),
      bound_element_array_buffer_(0),
      bound_texture_2d_(0),
      bound_texture_cube_map_(0),
      max_vertex_attribs_(0),
      current_program_(0),
#if defined(UNIT_TEST)
#elif defined(OS_WIN)
      device_context_(NULL),
      gl_context_(NULL),
#elif defined(OS_MACOSX)
      gl_context_(NULL),
      pbuffer_(NULL),
      surface_width_(0),
      surface_height_(0),
      texture_(0),
      fbo_(0),
      depth_renderbuffer_(0),
      bound_fbo_(0),
      bound_renderbuffer_(0),
#endif
      anti_aliased_(false) {
}

bool GLES2DecoderImpl::Initialize() {
  bool success = false;

  id_manager_.reset(new IdManager());
  buffer_manager_.reset(new BufferManager());
  shader_manager_.reset(new ShaderManager());
  program_manager_.reset(new ProgramManager());

  if (InitPlatformSpecific()) {
    if (MakeCurrent()) {
      if (InitGlew()) {
        CHECK_GL_ERROR();

        // Lookup GL things we need to know.
        GLint value;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
        max_vertex_attribs_ = value;

        GLint max_texture_size;
        GLint max_cube_map_texture_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_cube_map_texture_size);
        texture_manager_.reset(new TextureManager(max_texture_size,
                                                  max_cube_map_texture_size));
        DCHECK_GE(max_vertex_attribs_, 8u);

        vertex_attrib_infos_.reset(new VertexAttribInfo[max_vertex_attribs_]);
        memset(vertex_attrib_infos_.get(), 0,
               sizeof(vertex_attrib_infos_[0]) * max_vertex_attribs_);

        // glBindFramebuffer(0, 0);
        success = true;
      }
    }
  }

  return success;
}

// TODO(kbr): the use of this anonymous namespace core dumps the
// linker on Mac OS X 10.6 when the symbol ordering file is used
// namespace {

#if defined(UNIT_TEST)
#elif defined(OS_WIN)

const PIXELFORMATDESCRIPTOR kPixelFormatDescriptor = {
  sizeof(kPixelFormatDescriptor),    // Size of structure.
  1,                       // Default version.
  PFD_DRAW_TO_WINDOW |     // Window drawing support.
  PFD_SUPPORT_OPENGL |     // OpenGL support.
  PFD_DOUBLEBUFFER,        // Double buffering support (not stereo).
  PFD_TYPE_RGBA,           // RGBA color mode (not indexed).
  24,                      // 24 bit color mode.
  0, 0, 0, 0, 0, 0,        // Don't set RGB bits & shifts.
  8, 0,                    // 8 bit alpha
  0,                       // No accumulation buffer.
  0, 0, 0, 0,              // Ignore accumulation bits.
  24,                      // 24 bit z-buffer size.
  8,                       // 8-bit stencil buffer.
  0,                       // No aux buffer.
  PFD_MAIN_PLANE,          // Main drawing plane (not overlay).
  0,                       // Reserved.
  0, 0, 0,                 // Layer masks ignored.
};

LRESULT CALLBACK IntermediateWindowProc(HWND window,
                                        UINT message,
                                        WPARAM w_param,
                                        LPARAM l_param) {
  return ::DefWindowProc(window, message, w_param, l_param);
}

// Helper routine that returns the highest quality pixel format supported on
// the current platform.  Returns true upon success.
bool GetWindowsPixelFormat(HWND window,
                           bool anti_aliased,
                           int* pixel_format) {
  // We must initialize a GL context before we can determine the multi-sampling
  // supported on the current hardware, so we create an intermediate window
  // and context here.
  HINSTANCE module_handle;
  if (!::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           reinterpret_cast<wchar_t*>(IntermediateWindowProc),
                           &module_handle)) {
    return false;
  }

  WNDCLASS intermediate_class;
  intermediate_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  intermediate_class.lpfnWndProc = IntermediateWindowProc;
  intermediate_class.cbClsExtra = 0;
  intermediate_class.cbWndExtra = 0;
  intermediate_class.hInstance = module_handle;
  intermediate_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  intermediate_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  intermediate_class.hbrBackground = NULL;
  intermediate_class.lpszMenuName = NULL;
  intermediate_class.lpszClassName = L"Intermediate GL Window";

  ATOM class_registration = ::RegisterClass(&intermediate_class);
  if (!class_registration) {
    return false;
  }

  HWND intermediate_window = ::CreateWindow(
      reinterpret_cast<wchar_t*>(class_registration),
      L"",
      WS_OVERLAPPEDWINDOW,
      0, 0,
      CW_USEDEFAULT, CW_USEDEFAULT,
      NULL,
      NULL,
      NULL,
      NULL);

  if (!intermediate_window) {
    ::UnregisterClass(reinterpret_cast<wchar_t*>(class_registration),
                      module_handle);
    return false;
  }

  HDC intermediate_dc = ::GetDC(intermediate_window);
  int format_index = ::ChoosePixelFormat(intermediate_dc,
                                         &kPixelFormatDescriptor);
  if (format_index == 0) {
    DLOG(ERROR) << "Unable to get the pixel format for GL context.";
    ::ReleaseDC(intermediate_window, intermediate_dc);
    ::DestroyWindow(intermediate_window);
    ::UnregisterClass(reinterpret_cast<wchar_t*>(class_registration),
                      module_handle);
    return false;
  }
  if (!::SetPixelFormat(intermediate_dc, format_index,
                        &kPixelFormatDescriptor)) {
    DLOG(ERROR) << "Unable to set the pixel format for GL context.";
    ::ReleaseDC(intermediate_window, intermediate_dc);
    ::DestroyWindow(intermediate_window);
    ::UnregisterClass(reinterpret_cast<wchar_t*>(class_registration),
                      module_handle);
    return false;
  }

  // Store the pixel format without multisampling.
  *pixel_format = format_index;
  HGLRC gl_context = ::wglCreateContext(intermediate_dc);
  if (::wglMakeCurrent(intermediate_dc, gl_context)) {
    // GL context was successfully created and applied to the window's DC.
    // Startup GLEW, the GL extensions wrangler.
    GLenum glew_error = ::glewInit();
    if (glew_error == GLEW_OK) {
      DLOG(INFO) << "Initialized GLEW " << ::glewGetString(GLEW_VERSION);
    } else {
      DLOG(ERROR) << "Unable to initialise GLEW : "
                  << ::glewGetErrorString(glew_error);
      ::wglMakeCurrent(intermediate_dc, NULL);
      ::wglDeleteContext(gl_context);
      ::ReleaseDC(intermediate_window, intermediate_dc);
      ::DestroyWindow(intermediate_window);
      ::UnregisterClass(reinterpret_cast<wchar_t*>(class_registration),
                        module_handle);
      return false;
    }

    // If the multi-sample extensions are present, query the api to determine
    // the pixel format.
    if (anti_aliased && WGLEW_ARB_pixel_format && WGLEW_ARB_multisample) {
      int pixel_attributes[] = {
        WGL_SAMPLES_ARB, 4,
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        0, 0};

      float pixel_attributes_f[] = {0, 0};
      int msaa_pixel_format;
      unsigned int num_formats;

      // Query for the highest sampling rate supported, starting at 4x.
      static const int kSampleCount[] = {4, 2};
      static const int kNumSamples = 2;
      for (int sample = 0; sample < kNumSamples; ++sample) {
        pixel_attributes[1] = kSampleCount[sample];
        if (GL_TRUE == ::wglChoosePixelFormatARB(intermediate_dc,
                                                 pixel_attributes,
                                                 pixel_attributes_f,
                                                 1,
                                                 &msaa_pixel_format,
                                                 &num_formats)) {
          *pixel_format = msaa_pixel_format;
          break;
        }
      }
    }
  }

  ::wglMakeCurrent(intermediate_dc, NULL);
  ::wglDeleteContext(gl_context);
  ::ReleaseDC(intermediate_window, intermediate_dc);
  ::DestroyWindow(intermediate_window);
  ::UnregisterClass(reinterpret_cast<wchar_t*>(class_registration),
                    module_handle);
  return true;
}

#endif  // OS_WIN

// These commands convert from c calls to local os calls.
void GLGenBuffersHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids) {
  glGenBuffersARB(n, ids);
  // TODO(gman): handle error
  for (GLsizei ii = 0; ii < n; ++ii) {
    decoder->CreateBufferInfo(ids[ii]);
  }
}

void GLGenFramebuffersHelper(
    GLES2DecoderImpl*, GLsizei n, GLuint* ids) {
  glGenFramebuffersEXT(n, ids);
}

void GLGenRenderbuffersHelper(
    GLES2DecoderImpl*, GLsizei n, GLuint* ids) {
  glGenRenderbuffersEXT(n, ids);
}

void GLGenTexturesHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids) {
  glGenTextures(n, ids);
  // TODO(gman): handle error
  for (GLsizei ii = 0; ii < n; ++ii) {
    decoder->CreateTextureInfo(ids[ii]);
  }
}

void GLDeleteBuffersHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids) {
  glDeleteBuffersARB(n, ids);
  // TODO(gman): handle error
  for (GLsizei ii = 0; ii < n; ++ii) {
    decoder->RemoveBufferInfo(ids[ii]);
  }
}

void GLDeleteFramebuffersHelper(
    GLES2DecoderImpl*, GLsizei n, GLuint* ids) {
  glDeleteFramebuffersEXT(n, ids);
}

void GLDeleteRenderbuffersHelper(
    GLES2DecoderImpl*, GLsizei n, GLuint* ids) {
  glDeleteRenderbuffersEXT(n, ids);
}

void GLDeleteTexturesHelper(
    GLES2DecoderImpl* decoder, GLsizei n, GLuint* ids) {
  glDeleteTextures(n, ids);
  // TODO(gman): handle error
  for (GLsizei ii = 0; ii < n; ++ii) {
    decoder->RemoveTextureInfo(ids[ii]);
  }
}

// }  // anonymous namespace

bool GLES2DecoderImpl::MakeCurrent() {
#if defined(UNIT_TEST)
  return true;
#elif defined(OS_WIN)
  if (::wglGetCurrentDC() == device_context_ &&
      ::wglGetCurrentContext() == gl_context_) {
    return true;
  }
  if (!::wglMakeCurrent(device_context_, gl_context_)) {
    DLOG(ERROR) << "Unable to make gl context current.";
    return false;
  }
  return true;
#elif defined(OS_LINUX)
  return window()->MakeCurrent();
#elif defined(OS_MACOSX)
  if (CGLGetCurrentContext() != gl_context_) {
    if (CGLSetCurrentContext(gl_context_) != kCGLNoError) {
      DLOG(ERROR) << "Unable to make gl context current.";
      return false;
    }
  }
  return true;
#else
  NOTREACHED();
  return false;
#endif
}

uint32 GLES2DecoderImpl::GetServiceIdForTesting(uint32 client_id) {
#if defined(UNIT_TEST)
  GLuint service_id;
  bool result = id_manager_->GetServiceId(client_id, &service_id);
  return result ? service_id : 0u;
#else
  DCHECK(false);
  return 0u;
#endif
}

bool GLES2DecoderImpl::ValidateIdsAreUnused(
    GLsizei n, const GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    GLuint service_id;
    if (id_manager_->GetServiceId(client_ids[ii], &service_id)) {
      return false;
    }
  }
  return true;
}

bool GLES2DecoderImpl::RegisterObjects(
    GLsizei n, const GLuint* client_ids, const GLuint* service_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    if (!id_manager_->AddMapping(client_ids[ii], service_ids[ii])) {
      NOTREACHED();
      return false;
    }
  }
  return true;
}

void GLES2DecoderImpl::UnregisterObjects(
    GLsizei n, const GLuint* client_ids, GLuint* service_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    if (id_manager_->GetServiceId(client_ids[ii], &service_ids[ii])) {
      id_manager_->RemoveMapping(client_ids[ii], service_ids[ii]);
    } else {
      service_ids[ii] = 0;
    }
  }
}

bool GLES2DecoderImpl::InitPlatformSpecific() {
#if defined(UNIT_TEST)
#elif defined(OS_WIN)
  device_context_ = ::GetDC(hwnd());

  int pixel_format;

  if (!GetWindowsPixelFormat(hwnd(),
                             anti_aliased_,
                             &pixel_format)) {
      DLOG(ERROR) << "Unable to determine optimal pixel format for GL context.";
      return false;
  }

  if (!::SetPixelFormat(device_context_, pixel_format,
                        &kPixelFormatDescriptor)) {
    DLOG(ERROR) << "Unable to set the pixel format for GL context.";
    return false;
  }

  gl_context_ = ::wglCreateContext(device_context_);
  if (!gl_context_) {
    DLOG(ERROR) << "Failed to create GL context.";
    return false;
  }
#elif defined(OS_LINUX)
  DCHECK(window());
  if (!window()->Initialize())
    return false;
#elif defined(OS_MACOSX)
  // Create a 1x1 pbuffer and associated context to bootstrap things
  static const CGLPixelFormatAttribute attribs[] = {
    (CGLPixelFormatAttribute) kCGLPFAPBuffer,
    (CGLPixelFormatAttribute) 0
  };
  CGLPixelFormatObj pixelFormat;
  GLint numPixelFormats;
  if (CGLChoosePixelFormat(attribs,
                           &pixelFormat,
                           &numPixelFormats) != kCGLNoError) {
    DLOG(ERROR) << "Error choosing pixel format.";
    return false;
  }
  if (!pixelFormat) {
    return false;
  }
  CGLContextObj context;
  CGLError res = CGLCreateContext(pixelFormat, 0, &context);
  CGLDestroyPixelFormat(pixelFormat);
  if (res != kCGLNoError) {
    DLOG(ERROR) << "Error creating context.";
    return false;
  }
  CGLPBufferObj pbuffer;
  if (CGLCreatePBuffer(1, 1,
                       GL_TEXTURE_2D, GL_RGBA,
                       0, &pbuffer) != kCGLNoError) {
    CGLDestroyContext(context);
    DLOG(ERROR) << "Error creating pbuffer.";
    return false;
  }
  if (CGLSetPBuffer(context, pbuffer, 0, 0, 0) != kCGLNoError) {
    CGLDestroyContext(context);
    CGLDestroyPBuffer(pbuffer);
    DLOG(ERROR) << "Error attaching pbuffer to context.";
    return false;
  }
  gl_context_ = context;
  pbuffer_ = pbuffer;
  // Now we're ready to handle SetWindowSize calls, which will
  // allocate and/or reallocate the IOSurface and associated offscreen
  // OpenGL structures for rendering.
#endif

  return true;
}

bool GLES2DecoderImpl::InitGlew() {
#if !defined(UNIT_TEST)
  DLOG(INFO) << "Initializing GL and GLEW for GLES2Decoder.";

  GLenum glew_error = glewInit();
  if (glew_error != GLEW_OK) {
    DLOG(ERROR) << "Unable to initialise GLEW : "
                << ::glewGetErrorString(glew_error);
    return false;
  }

  // Check to see that we can use the OpenGL vertex attribute APIs
  // TODO(petersont):  Return false if this check fails, but because some
  // Intel hardware does not support OpenGL 2.0, yet does support all of the
  // extensions we require, we only log an error.  A future CL should change
  // this check to ensure that all of the extension strings we require are
  // present.
  if (!GLEW_VERSION_2_0) {
    DLOG(ERROR) << "GL drivers do not have OpenGL 2.0 functionality.";
  }

  bool extensions_found = true;
  if (!GLEW_ARB_vertex_buffer_object) {
    // NOTE: Linux NVidia drivers claim to support OpenGL 2.0 when using
    // indirect rendering (e.g. remote X), but it is actually lying. The
    // ARB_vertex_buffer_object functions silently no-op (!) when using
    // indirect rendering, leading to crashes. Fortunately, in that case, the
    // driver claims to not support ARB_vertex_buffer_object, so fail in that
    // case.
    DLOG(ERROR) << "GL drivers do not support vertex buffer objects.";
    extensions_found = false;
  }
  if (!GLEW_EXT_framebuffer_object) {
    DLOG(ERROR) << "GL drivers do not support framebuffer objects.";
    extensions_found = false;
  }
  // Check for necessary extensions
  if (!GLEW_VERSION_2_0 && !GLEW_EXT_stencil_two_side) {
    DLOG(ERROR) << "Two sided stencil extension missing.";
    extensions_found = false;
  }
  if (!GLEW_VERSION_1_4 && !GLEW_EXT_blend_func_separate) {
    DLOG(ERROR) <<"Separate blend func extension missing.";
    extensions_found = false;
  }
  if (!GLEW_VERSION_2_0 && !GLEW_EXT_blend_equation_separate) {
    DLOG(ERROR) << "Separate blend function extension missing.";
    extensions_found = false;
  }
  if (!extensions_found)
    return false;
#endif

  return true;
}

#if defined(OS_MACOSX)
#if !defined(UNIT_TEST)
static void AddBooleanValue(CFMutableDictionaryRef dictionary,
                            const CFStringRef key,
                            bool value) {
  CFDictionaryAddValue(dictionary, key,
                       (value ? kCFBooleanTrue : kCFBooleanFalse));
}

static void AddIntegerValue(CFMutableDictionaryRef dictionary,
                            const CFStringRef key,
                            int32 value) {
  CFNumberRef number = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
  CFDictionaryAddValue(dictionary, key, number);
}
#endif  // !defined(UNIT_TEST)

uint64 GLES2DecoderImpl::SetWindowSize(int32 width, int32 height) {
#if defined(UNIT_TEST)
  return 0;
#else
  if (surface_width_ == width && surface_height_ == height) {
    // Return 0 to indicate to the caller that no new backing store
    // allocation occurred.
    return 0;
  }

  IOSurfaceSupport* io_surface_support = IOSurfaceSupport::Initialize();
  if (!io_surface_support)
    return 0;

  if (!MakeCurrent())
    return 0;

  // GL_TEXTURE_RECTANGLE_ARB is the best supported render target on
  // Mac OS X and is required for IOSurface interoperability.
  GLenum target = GL_TEXTURE_RECTANGLE_ARB;

  if (!texture_) {
    // Generate the texture object.
    glGenTextures(1, &texture_);
    glBindTexture(target, texture_);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Generate and bind the framebuffer object.
    glGenFramebuffersEXT(1, &fbo_);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
    bound_fbo_ = fbo_;
    // Generate (but don't bind) the depth buffer -- we don't need
    // this bound in order to do offscreen rendering.
    glGenRenderbuffersEXT(1, &depth_renderbuffer_);
  }

  // Allocate a new IOSurface, which is the GPU resource that can be
  // shared across processes.
  scoped_cftyperef<CFMutableDictionaryRef> properties;
  properties.reset(CFDictionaryCreateMutable(kCFAllocatorDefault,
                                             0,
                                             &kCFTypeDictionaryKeyCallBacks,
                                             &kCFTypeDictionaryValueCallBacks));
  AddIntegerValue(properties,
                  io_surface_support->GetKIOSurfaceWidth(), width);
  AddIntegerValue(properties,
                  io_surface_support->GetKIOSurfaceHeight(), height);
  AddIntegerValue(properties,
                  io_surface_support->GetKIOSurfaceBytesPerElement(), 4);
  AddBooleanValue(properties,
                  io_surface_support->GetKIOSurfaceIsGlobal(), true);
  // I believe we should be able to unreference the IOSurfaces without
  // synchronizing with the browser process because they are
  // ultimately reference counted by the operating system.
  io_surface_.reset(io_surface_support->IOSurfaceCreate(properties));

  // Reallocate the depth buffer.
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_renderbuffer_);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                           GL_DEPTH_COMPONENT,
                           width,
                           height);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, bound_renderbuffer_);

  // Reallocate the texture object.
  glBindTexture(target, texture_);
  // Don't think we need to identify a plane.
  GLuint plane = 0;
  io_surface_support->CGLTexImageIOSurface2D(gl_context_,
                                             target,
                                             GL_RGBA,
                                             width,
                                             height,
                                             GL_BGRA,
                                             GL_UNSIGNED_INT_8_8_8_8_REV,
                                             io_surface_.get(),
                                             plane);

  // Set up the frame buffer object.
  if (bound_fbo_ != fbo_) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
  }
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            target,
                            texture_,
                            0);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                               GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT,
                               depth_renderbuffer_);
  if (bound_fbo_ != fbo_) {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, bound_fbo_);
  }

  surface_width_ = width;
  surface_height_ = height;

  // Now send back an identifier for the IOSurface. We originally
  // intended to send back a mach port from IOSurfaceCreateMachPort
  // but it looks like Chrome IPC would need to be modified to
  // properly send mach ports between processes. For the time being we
  // make our IOSurfaces global and send back their identifiers. On
  // the browser process side the identifier is reconstituted into an
  // IOSurface for on-screen rendering.
  return io_surface_support->IOSurfaceGetID(io_surface_);
#endif  // !defined(UNIT_TEST)
}
#endif  // defined(OS_MACOSX)

void GLES2DecoderImpl::SetSwapBuffersCallback(Callback0::Type* callback) {
  swap_buffers_callback_.reset(callback);
}

void GLES2DecoderImpl::Destroy() {
#if defined(UNIT_TEST)
#elif defined(OS_LINUX)
  DCHECK(window());
  window()->Destroy();
#elif defined(OS_MACOSX)
  if (gl_context_)
    CGLDestroyContext(gl_context_);
  if (pbuffer_)
    CGLDestroyPBuffer(pbuffer_);
#endif
}

const char* GLES2DecoderImpl::GetCommandName(unsigned int command_id) const {
  if (command_id > kStartPoint && command_id < kNumCommands) {
    return gles2::GetCommandName(static_cast<CommandId>(command_id));
  }
  return GetCommonCommandName(static_cast<cmd::CommandId>(command_id));
}

// Decode command with its arguments, and call the corresponding GL function.
// Note: args is a pointer to the command buffer. As such, it could be changed
// by a (malicious) client at any time, so if validation has to happen, it
// should operate on a copy of them.
error::Error GLES2DecoderImpl::DoCommand(
    unsigned int command,
    unsigned int arg_count,
    const void* cmd_data) {
  error::Error result = error::kNoError;
  if (debug()) {
    // TODO(gman): Change output to something useful for NaCl.
    printf("cmd: %s\n", GetCommandName(command));
  }
  unsigned int command_index = command - kStartPoint - 1;
  if (command_index < arraysize(g_command_info)) {
    const CommandInfo& info = g_command_info[command_index];
    unsigned int info_arg_count = static_cast<unsigned int>(info.arg_count);
    if ((info.arg_flags == cmd::kFixed && arg_count == info_arg_count) ||
        (info.arg_flags == cmd::kAtLeastN && arg_count >= info_arg_count)) {
      uint32 immediate_data_size =
          (arg_count - info_arg_count) * sizeof(CommandBufferEntry);  // NOLINT
      switch (command) {
        #define GLES2_CMD_OP(name)                                 \
          case name::kCmdId:                                       \
            result = Handle ## name(                               \
                immediate_data_size,                               \
                *static_cast<const name*>(cmd_data));              \
            break;                                                 \

        GLES2_COMMAND_LIST(GLES2_CMD_OP)
        #undef GLES2_CMD_OP
      }
      if (debug()) {
        GLenum error;
        while ((error = glGetError()) != GL_NO_ERROR) {
          // TODO(gman): Change output to something useful for NaCl.
          SetGLError(error);
          printf("GL ERROR b4: %s\n", GetCommandName(command));
        }
      }
    } else {
      result = error::kInvalidArguments;
    }
  } else {
    result = DoCommonCommand(command, arg_count, cmd_data);
  }
  return result;
}

void GLES2DecoderImpl::CreateProgramHelper(GLuint client_id) {
  // TODO(gman): verify client_id is unused.
  GLuint service_id = glCreateProgram();
  if (service_id) {
    id_manager_->AddMapping(client_id, service_id);
    CreateProgramInfo(service_id);
  }
}

void GLES2DecoderImpl::CreateShaderHelper(GLenum type, GLuint client_id) {
  // TODO(gman): verify client_id is unused.
  GLuint service_id = glCreateShader(type);
  if (service_id) {
    id_manager_->AddMapping(client_id, service_id);
    CreateShaderInfo(service_id);
  }
}

void GLES2DecoderImpl::DoBindBuffer(GLenum target, GLuint buffer) {
  if (buffer) {
    BufferManager::BufferInfo* info = GetBufferInfo(buffer);
    if (!info) {
      SetGLError(GL_INVALID_OPERATION);
      return;
    }
  }
  switch (target) {
    case GL_ARRAY_BUFFER:
      bound_array_buffer_ = buffer;
      break;
    case GL_ELEMENT_ARRAY_BUFFER:
      bound_element_array_buffer_ = buffer;
      break;
    default:
      NOTREACHED();  // Validation should prevent us getting here.
      break;
  }
  glBindBuffer(target, buffer);
}

void GLES2DecoderImpl::DoBindTexture(GLenum target, GLuint texture) {
  if (texture) {
    TextureManager::TextureInfo* info = GetTextureInfo(texture);
    if (!info || (info->target() != 0 && info->target() != target)) {
      SetGLError(GL_INVALID_OPERATION);
      return;
    }
    if (info->target() == 0) {
      texture_manager_->SetInfoTarget(info, target);
    }
  }
  glBindTexture(target, texture);
  switch (target) {
    case GL_TEXTURE_2D:
      bound_texture_2d_ = texture;
      break;
    case GL_TEXTURE_CUBE_MAP:
      bound_texture_cube_map_ = texture;
      break;
    default:
      NOTREACHED();  // Validation should prevent us getting here.
      break;
  }
}

void GLES2DecoderImpl::DoDisableVertexAttribArray(GLuint index) {
  if (index < max_vertex_attribs_) {
    vertex_attrib_infos_[index].set_enabled(false);
    glDisableVertexAttribArray(index);
  } else {
    SetGLError(GL_INVALID_VALUE);
  }
}

void GLES2DecoderImpl::DoEnableVertexAttribArray(GLuint index) {
  if (index < max_vertex_attribs_) {
    vertex_attrib_infos_[index].set_enabled(true);
    glEnableVertexAttribArray(index);
  } else {
    SetGLError(GL_INVALID_VALUE);
  }
}

void GLES2DecoderImpl::DoGenerateMipmap(GLenum target) {
  GLuint texture = GetTextureForTarget(target);
  TextureManager::TextureInfo* info = GetTextureInfo(texture);
  if (!info || !info->CanGenerateMipmaps()) {
    SetGLError(GL_INVALID_OPERATION);
    return;
  }
  glGenerateMipmapEXT(target);
  info->MarkMipmapsGenerated();
}

error::Error GLES2DecoderImpl::HandleDeleteShader(
    uint32 immediate_data_size, const gles2::DeleteShader& c) {
  GLuint shader = c.shader;
  GLuint service_id;
  if (!id_manager_->GetServiceId(shader, &service_id)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  RemoveShaderInfo(service_id);
  glDeleteShader(service_id);
  id_manager_->RemoveMapping(shader, service_id);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteProgram(
    uint32 immediate_data_size, const gles2::DeleteProgram& c) {
  GLuint program = c.program;
  GLuint service_id;
  if (!id_manager_->GetServiceId(program, &service_id)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  RemoveProgramInfo(service_id);
  glDeleteProgram(service_id);
  id_manager_->RemoveMapping(program, service_id);
  return error::kNoError;
}

void GLES2DecoderImpl::DoDrawArrays(
    GLenum mode, GLint first, GLsizei count) {
  if (IsDrawValid(first + count - 1)) {
    glDrawArrays(mode, first, count);
  }
}

void GLES2DecoderImpl::DoLinkProgram(GLuint program) {
  ProgramManager::ProgramInfo* info = GetProgramInfo(program);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return;
  }
  CopyRealGLErrorsToWrapper();
  glLinkProgram(program);
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    RemoveProgramInfo(program);
    SetGLError(error);
  } else {
    info->Update();
  }
};

void GLES2DecoderImpl::DoSwapBuffers() {
#if defined(UNIT_TEST)
#elif defined(OS_WIN)
  ::SwapBuffers(device_context_);
#elif defined(OS_LINUX)
  DCHECK(window());
  window()->SwapBuffers();
#elif defined(OS_MACOSX)
  if (bound_fbo_ == fbo_) {
    // Bind and unbind the framebuffer to make changes to the
    // IOSurface show up in the other process.
    glFlush();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
  }
#endif
  if (swap_buffers_callback_.get()) {
    swap_buffers_callback_->Run();
  }
}

void GLES2DecoderImpl::DoUseProgram(GLuint program) {
  ProgramManager::ProgramInfo* info = GetProgramInfo(program);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
  } else {
    current_program_ = program;
    glUseProgram(program);
  }
}

GLenum GLES2DecoderImpl::GetGLError() {
  // Check the GL error first, then our wrapped error.
  GLenum error = glGetError();
  if (error == GL_NO_ERROR && error_bits_ != 0) {
    for (uint32 mask = 1; mask != 0; mask = mask << 1) {
      if ((error_bits_ & mask) != 0) {
        error = GLErrorBitToGLError(mask);
        break;
      }
    }
  }

  if (error != GL_NO_ERROR) {
    // There was an error, clear the corresponding wrapped error.
    error_bits_ &= ~GLErrorToErrorBit(error);
  }
  return error;
}

void GLES2DecoderImpl::SetGLError(GLenum error) {
  error_bits_ |= GLErrorToErrorBit(error);
}

void GLES2DecoderImpl::CopyRealGLErrorsToWrapper() {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    SetGLError(error);
  }
}

void GLES2DecoderImpl::RemoveTextureInfo(GLuint texture_id) {
  // TODO(gman): This code needs to change for shared resources. It needs to
  // handle a different decoder deleting the texture. (or, the code that gets
  // a texture always needs to check that it still exists).
  if (bound_texture_2d_ == texture_id) {
    bound_texture_2d_ = 0;
  }
  if (bound_texture_cube_map_ == texture_id) {
    bound_texture_cube_map_ = 0;
  }
  texture_manager_->RemoveTextureInfo(texture_id);
}

void GLES2DecoderImpl::UpdateVertexAttribInfo(GLuint buffer, GLsizeiptr size) {
  // go through VertexAttribInfo and update any info that references the buffer.
  // TODO(gman): This code needs to change for shared resources.
  for (GLuint ii = 0; ii < max_vertex_attribs_; ++ii) {
    if (vertex_attrib_infos_[ii].buffer() == buffer) {
      vertex_attrib_infos_[ii].SetBufferSize(size);
    }
  }
}

void GLES2DecoderImpl::RemoveBufferInfo(GLuint buffer_id) {
  // TODO(gman): This code needs to change for shared resources.
  for (GLuint ii = 0; ii < max_vertex_attribs_; ++ii) {
    if (vertex_attrib_infos_[ii].buffer() == buffer_id) {
      vertex_attrib_infos_[ii].Clear();
    }
  }
  if (bound_array_buffer_ == buffer_id) {
    bound_array_buffer_ = 0;
  }
  if (bound_element_array_buffer_ == buffer_id) {
    bound_element_array_buffer_ = 0;
  }
  buffer_manager_->RemoveBufferInfo(buffer_id);
}

bool GLES2DecoderImpl::VertexAttribInfo::CanAccess(GLuint index) {
  return !enabled_ || (buffer_ != 0 && index < num_elements_);
}

bool GLES2DecoderImpl::IsDrawValid(GLuint max_vertex_accessed) {
  if (current_program_) {
    ProgramManager::ProgramInfo* info = GetProgramInfo(current_program_);
    if (!info) {
      // The program does not exist.
      SetGLError(GL_INVALID_OPERATION);
      return false;
    }
    // Validate that all attribs current program needs are setup correctly.
    const ProgramManager::ProgramInfo::AttribInfoVector& infos =
        info->GetAttribInfos();
    for (size_t ii = 0; ii < infos.size(); ++ii) {
      GLint location = infos[ii].location;
      if (location < 0) {
        return false;
      }
      DCHECK_LT(static_cast<GLuint>(location), max_vertex_attribs_);
      if (!vertex_attrib_infos_[location].CanAccess(max_vertex_accessed)) {
        SetGLError(GL_INVALID_OPERATION);
        return false;
      }
    }
    return true;
  }
  // We do not set a GL error here because the GL spec says no error if the
  // program is invalid.
  return false;
};

error::Error GLES2DecoderImpl::HandleDrawElements(
    uint32 immediate_data_size, const gles2::DrawElements& c) {
  if (bound_element_array_buffer_ != 0) {
    GLenum mode = c.mode;
    GLsizei count = c.count;
    GLenum type = c.type;
    int32 offset = c.index_offset;
    if (!ValidateGLenumDrawMode(mode) ||
        !ValidateGLenumIndexType(type)) {
      SetGLError(GL_INVALID_ENUM);
    } else {
      // TODO(gman): We could cache this lookup in glBindBuffer.
      BufferManager::BufferInfo* info =
          GetBufferInfo(bound_element_array_buffer_);
      if (!info) {
        SetGLError(GL_INVALID_OPERATION);
      } else {
        GLsizeiptr buffer_size = info->size();
        if (offset > buffer_size) {
          SetGLError(GL_INVALID_OPERATION);
        } else {
          GLsizei usable_size = buffer_size - offset;
          GLsizei num_elements = usable_size / GetGLTypeSize(type);
          if (count > num_elements) {
            SetGLError(GL_INVALID_OPERATION);
          } else {
            const GLvoid* indices = reinterpret_cast<const GLvoid*>(offset);
            // TODO(gman): Validate indices. Get maximum index.
            //
            // This value should be computed by walking the index buffer from 0
            // to count and finding the maximum vertex accessed. For now we'll
            // pass 0 so it should always pass.
            GLuint max_vertex_accessed = info->GetMaxValueForRange(
                offset, count, type);
            if (IsDrawValid(max_vertex_accessed)) {
              glDrawElements(mode, count, type, indices);
            }
          }
        }
      }
    }
  } else {
    SetGLError(GL_INVALID_OPERATION);
  }
  return error::kNoError;
}

// Calls glShaderSource for the various versions of the ShaderSource command.
// Assumes that data / data_size points to a piece of memory that is in range
// of whatever context it came from (shared memory, immediate memory, bucket
// memory.)
error::Error GLES2DecoderImpl::ShaderSourceHelper(
    GLuint shader, const char* data, uint32 data_size) {
  ShaderManager::ShaderInfo* info = GetShaderInfo(shader);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  // Note: We don't actually call glShaderSource here. We wait until
  // the call to glCompileShader.
  info->Update(std::string(data, data + data_size));
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleShaderSource(
    uint32 immediate_data_size, const gles2::ShaderSource& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  uint32 data_size = c.data_size;
  const char* data = GetSharedMemoryAs<const char*>(
      c.data_shm_id, c.data_shm_offset, data_size);
  if (!data) {
    return error::kOutOfBounds;
  }
  return ShaderSourceHelper(shader, data, data_size);
}

error::Error GLES2DecoderImpl::HandleShaderSourceImmediate(
  uint32 immediate_data_size, const gles2::ShaderSourceImmediate& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  uint32 data_size = c.data_size;
  const char* data = GetImmediateDataAs<const char*>(
      c, data_size, immediate_data_size);
  if (!data) {
    return error::kOutOfBounds;
  }
  return ShaderSourceHelper(shader, data, data_size);
}

void GLES2DecoderImpl::DoCompileShader(GLuint shader) {
  ShaderManager::ShaderInfo* info = GetShaderInfo(shader);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return;
  }
  // TODO(gman): Run shader through compiler that converts GL ES 2.0 shader
  // to DesktopGL shader and pass that to glShaderSource and then
  // glCompileShader.
  const char* ptr = info->source().c_str();
  glShaderSource(shader, 1, &ptr, NULL);
  glCompileShader(shader);
};

void GLES2DecoderImpl::DoGetShaderSource(
      GLuint shader, GLsizei bufsize, GLsizei* length, char* dst) {
  ShaderManager::ShaderInfo* info = GetShaderInfo(shader);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return;
  }
  const std::string& source = info->source();
  GLsizei size = std::min(bufsize - 1, static_cast<GLsizei>(source.size()));
  if (length) {
    *length = size;
  }
  memcpy(dst, source.c_str(), size);
  dst[size] = '\0';
}

error::Error GLES2DecoderImpl::HandleVertexAttribPointer(
    uint32 immediate_data_size, const gles2::VertexAttribPointer& c) {
  if (bound_array_buffer_ != 0) {
    GLuint indx = c.indx;
    GLint size = c.size;
    GLenum type = c.type;
    GLboolean normalized = c.normalized;
    GLsizei stride = c.stride;
    GLsizei offset = c.offset;
    const void* ptr = reinterpret_cast<const void*>(offset);
    if (!ValidateGLenumVertexAttribType(type) ||
        !ValidateGLintVertexAttribSize(size) ||
        indx >= max_vertex_attribs_ ||
        stride < 0) {
      SetGLError(GL_INVALID_VALUE);
      return error::kNoError;
    }
    const BufferManager::BufferInfo* buffer_info =
        GetBufferInfo(bound_array_buffer_);
    GLsizei component_size = GetGLTypeSize(type);
    GLsizei real_stride = stride != 0 ? stride : component_size * size;
    if (offset % component_size > 0) {
      SetGLError(GL_INVALID_VALUE);
      return error::kNoError;
    }
    vertex_attrib_infos_[indx].SetInfo(
        bound_array_buffer_,
        buffer_info ? buffer_info->size() : 0,
        size,
        type,
        real_stride,
        offset);
    glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
  } else {
    SetGLError(GL_INVALID_VALUE);
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleReadPixels(
    uint32 immediate_data_size, const gles2::ReadPixels& c) {
  GLint x = c.x;
  GLint y = c.y;
  GLsizei width = c.width;
  GLsizei height = c.height;
  GLenum format = c.format;
  GLenum type = c.type;
  uint32 pixels_size = GLES2Util::ComputeImageDataSize(
      width, height, format, type, pack_alignment_);
  void* pixels = GetSharedMemoryAs<void*>(
      c.pixels_shm_id, c.pixels_shm_offset, pixels_size);
  if (!pixels) {
    return error::kOutOfBounds;
  }
  if (!ValidateGLenumReadPixelFormat(format) ||
      !ValidateGLenumPixelType(type)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  glReadPixels(x, y, width, height, format, type, pixels);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandlePixelStorei(
    uint32 immediate_data_size, const gles2::PixelStorei& c) {
  GLenum pname = c.pname;
  GLenum param = c.param;
  if (!ValidateGLenumPixelStore(pname) ||
      !ValidateGLintPixelStoreAlignment(param)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  glPixelStorei(pname, param);
  switch (pname) {
  case GL_PACK_ALIGNMENT:
      pack_alignment_ = param;
      break;
  case GL_UNPACK_ALIGNMENT:
      unpack_alignment_ = param;
      break;
  default:
      // Validation should have prevented us from getting here.
      DCHECK(false);
      break;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetAttribLocation(
    uint32 immediate_data_size, const gles2::GetAttribLocation& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(program);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  uint32 name_size = c.data_size;
  const char* name = GetSharedMemoryAs<const char*>(
      c.name_shm_id, c.name_shm_offset, name_size);
  GLint* location = GetSharedMemoryAs<GLint*>(
      c.location_shm_id, c.location_shm_offset, sizeof(GLint));
  if (!location || !name) {
    return error::kOutOfBounds;
  }
  String name_str(name, name_size);
  *location = info->GetAttribLocation(name_str);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetAttribLocationImmediate(
    uint32 immediate_data_size, const gles2::GetAttribLocationImmediate& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(program);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  uint32 name_size = c.data_size;
  const char* name = GetImmediateDataAs<const char*>(
      c, name_size, immediate_data_size);
  GLint* location = GetSharedMemoryAs<GLint*>(
      c.location_shm_id, c.location_shm_offset, sizeof(GLint));
  if (!location || !name) {
    return error::kOutOfBounds;
  }
  String name_str(name, name_size);
  *location = info->GetAttribLocation(name_str);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetUniformLocation(
    uint32 immediate_data_size, const gles2::GetUniformLocation& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(program);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  uint32 name_size = c.data_size;
  const char* name = GetSharedMemoryAs<const char*>(
      c.name_shm_id, c.name_shm_offset, name_size);
  GLint* location = GetSharedMemoryAs<GLint*>(
      c.location_shm_id, c.location_shm_offset, sizeof(GLint));
  if (!location || !name) {
    return error::kOutOfBounds;
  }
  String name_str(name, name_size);
  *location = info->GetUniformLocation(name_str);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetUniformLocationImmediate(
    uint32 immediate_data_size, const gles2::GetUniformLocationImmediate& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(program);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  uint32 name_size = c.data_size;
  const char* name = GetImmediateDataAs<const char*>(
      c, name_size, immediate_data_size);
  GLint* location = GetSharedMemoryAs<GLint*>(
      c.location_shm_id, c.location_shm_offset, sizeof(GLint));
  if (!location || !name) {
    return error::kOutOfBounds;
  }
  String name_str(name, name_size);
  *location = info->GetUniformLocation(name_str);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBufferData(
    uint32 immediate_data_size, const gles2::BufferData& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLsizeiptr size = static_cast<GLsizeiptr>(c.size);
  uint32 data_shm_id = static_cast<uint32>(c.data_shm_id);
  uint32 data_shm_offset = static_cast<uint32>(c.data_shm_offset);
  GLenum usage = static_cast<GLenum>(c.usage);
  const void* data = NULL;
  if (data_shm_id != 0 || data_shm_offset != 0) {
    data = GetSharedMemoryAs<const void*>(data_shm_id, data_shm_offset, size);
    if (!data) {
      return error::kOutOfBounds;
    }
  }
  if (!ValidateGLenumBufferTarget(target) ||
      !ValidateGLenumBufferUsage(usage)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint buffer = GetBufferForTarget(target);
  BufferManager::BufferInfo* info = GetBufferInfo(buffer);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  // Clear the buffer to 0 if no initial data was passed in.
  scoped_array<int8> zero;
  if (!data) {
    zero.reset(new int8[size]);
    memset(zero.get(), 0, size);
    data = zero.get();
  }
  CopyRealGLErrorsToWrapper();
  glBufferData(target, size, data, usage);
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    SetGLError(error);
  } else {
    info->set_size(size);
    UpdateVertexAttribInfo(buffer, size);
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBufferDataImmediate(
    uint32 immediate_data_size, const gles2::BufferDataImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLsizeiptr size = static_cast<GLsizeiptr>(c.size);
  const void* data = GetImmediateDataAs<const void*>(
      c, size, immediate_data_size);
  if (!data) {
    return error::kOutOfBounds;
  }
  GLenum usage = static_cast<GLenum>(c.usage);
  if (!ValidateGLenumBufferTarget(target) ||
      !ValidateGLenumBufferUsage(usage)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint buffer = GetBufferForTarget(target);
  BufferManager::BufferInfo* info = GetBufferInfo(buffer);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  CopyRealGLErrorsToWrapper();
  glBufferData(target, size, data, usage);
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    SetGLError(error);
  } else {
    info->set_size(size);
    UpdateVertexAttribInfo(buffer, size);
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::DoCompressedTexImage2D(
  GLenum target,
  GLint level,
  GLenum internal_format,
  GLsizei width,
  GLsizei height,
  GLint border,
  GLsizei image_size,
  const void* data) {
  // TODO(gman): Validate internal_format
  // TODO(gman): Validate image_size is correct for width, height and format.
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!texture_manager_->ValidForTarget(target, level, width, height, 1) ||
      border != 0) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint texture = GetTextureForTarget(target);
  TextureManager::TextureInfo* info = GetTextureInfo(texture);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  scoped_array<int8> zero;
  if (!data) {
    zero.reset(new int8[image_size]);
    memset(zero.get(), 0, image_size);
    data = zero.get();
  }
  info->SetLevelInfo(
      target, level, internal_format, width, height, 1, border, 0, 0);
  glCompressedTexImage2D(
      target, level, internal_format, width, height, border, image_size, data);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCompressedTexImage2D(
    uint32 immediate_data_size, const gles2::CompressedTexImage2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLenum internal_format = static_cast<GLenum>(c.internalformat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLint border = static_cast<GLint>(c.border);
  GLsizei image_size = static_cast<GLsizei>(c.imageSize);
  uint32 data_shm_id = static_cast<uint32>(c.data_shm_id);
  uint32 data_shm_offset = static_cast<uint32>(c.data_shm_offset);
  const void* data = NULL;
  if (data_shm_id != 0 || data_shm_offset != 0) {
    data = GetSharedMemoryAs<const void*>(
        data_shm_id, data_shm_offset, image_size);
    if (!data) {
      return error::kOutOfBounds;
    }
  }
  return DoCompressedTexImage2D(
      target, level, internal_format, width, height, border, image_size, data);
}

error::Error GLES2DecoderImpl::HandleCompressedTexImage2DImmediate(
    uint32 immediate_data_size, const gles2::CompressedTexImage2DImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLenum internal_format = static_cast<GLenum>(c.internalformat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLint border = static_cast<GLint>(c.border);
  GLsizei image_size = static_cast<GLsizei>(c.imageSize);
  const void* data = GetImmediateDataAs<const void*>(
      c, image_size, immediate_data_size);
  if (!data) {
    return error::kOutOfBounds;
  }
  return DoCompressedTexImage2D(
      target, level, internal_format, width, height, border, image_size, data);
}

// TODO(gman): handle CopyTexImage2D because we need to track what was created.

error::Error GLES2DecoderImpl::DoTexImage2D(
  GLenum target,
  GLint level,
  GLenum internal_format,
  GLsizei width,
  GLsizei height,
  GLint border,
  GLenum format,
  GLenum type,
  const void* pixels,
  uint32 pixels_size) {
  if (!ValidateGLenumTextureTarget(target) ||
      !ValidateGLenumTextureFormat(internal_format) ||
      !ValidateGLenumTextureFormat(format) ||
      !ValidateGLenumPixelType(type)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!texture_manager_->ValidForTarget(target, level, width, height, 1) ||
      border != 0) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint texture = GetTextureForTarget(target);
  TextureManager::TextureInfo* info = GetTextureInfo(texture);
  if (!info) {
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  scoped_array<int8> zero;
  if (!pixels) {
    zero.reset(new int8[pixels_size]);
    memset(zero.get(), 0, pixels_size);
    pixels = zero.get();
  }
  info->SetLevelInfo(
      target, level, internal_format, width, height, 1, border, format, type);
  glTexImage2D(
      target, level, internal_format, width, height, border, format, type,
      pixels);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexImage2D(
    uint32 immediate_data_size, const gles2::TexImage2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint internal_format = static_cast<GLint>(c.internalformat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLint border = static_cast<GLint>(c.border);
  GLenum format = static_cast<GLenum>(c.format);
  GLenum type = static_cast<GLenum>(c.type);
  uint32 pixels_shm_id = static_cast<uint32>(c.pixels_shm_id);
  uint32 pixels_shm_offset = static_cast<uint32>(c.pixels_shm_offset);
  uint32 pixels_size = GLES2Util::ComputeImageDataSize(
      width, height, format, type, unpack_alignment_);
  const void* pixels = NULL;
  if (pixels_shm_id != 0 || pixels_shm_offset != 0) {
    pixels = GetSharedMemoryAs<const void*>(
        pixels_shm_id, pixels_shm_offset, pixels_size);
    if (!pixels) {
      return error::kOutOfBounds;
    }
  }
  return DoTexImage2D(
      target, level, internal_format, width, height, border, format, type,
      pixels, pixels_size);
}

error::Error GLES2DecoderImpl::HandleTexImage2DImmediate(
    uint32 immediate_data_size, const gles2::TexImage2DImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint internal_format = static_cast<GLint>(c.internalformat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLint border = static_cast<GLint>(c.border);
  GLenum format = static_cast<GLenum>(c.format);
  GLenum type = static_cast<GLenum>(c.type);
  uint32 size = GLES2Util::ComputeImageDataSize(
      width, height, format, type, unpack_alignment_);
  const void* pixels = GetImmediateDataAs<const void*>(
      c, size, immediate_data_size);
  if (!pixels) {
    return error::kOutOfBounds;
  }
  DoTexImage2D(
      target, level, internal_format, width, height, border, format, type,
      pixels, size);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetVertexAttribPointerv(
    uint32 immediate_data_size, const gles2::GetVertexAttribPointerv& c) {
  GLuint index = static_cast<GLuint>(c.index);
  GLenum pname = static_cast<GLenum>(c.pname);
  typedef gles2::GetVertexAttribPointerv::Result Result;
  Result* result = GetSharedMemoryAs<Result*>(
        c.pointer_shm_id, c.pointer_shm_offset, Result::ComputeSize(1));
  if (!result) {
    return error::kOutOfBounds;
  }
  // Check that the client initialized the result.
  if (result->size != 0) {
    return error::kInvalidArguments;
  }
  if (!ValidateGLenumVertexPointer(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (index >= max_vertex_attribs_) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  result->SetNumResults(1);
  *result->GetData() = vertex_attrib_infos_[index].offset();
  return error::kNoError;
}

bool GLES2DecoderImpl::GetUniformSetup(
    GLuint program, GLint location,
    uint32 shm_id, uint32 shm_offset,
    error::Error* error, GLuint* service_id, void** result_pointer) {
  *error = error::kNoError;
  // Make sure we have enough room for the result on failure.
  SizedResult<GLint>* result;
  result = GetSharedMemoryAs<SizedResult<GLint>*>(
      shm_id, shm_offset, SizedResult<GLint>::ComputeSize(0));
  if (!result) {
    *error = error::kOutOfBounds;
    return false;
  }
  *result_pointer = result;
  // Set the result size to 0 so the client does not have to check for success.
  result->SetNumResults(0);
  if (!id_manager_->GetServiceId(program, service_id)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(*service_id);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return false;
  }
  GLenum type;
  if (!info->GetUniformTypeByLocation(location, &type)) {
    // No such location.
    SetGLError(GL_INVALID_OPERATION);
    return false;
  }
  GLsizei size = GLES2Util::GetGLDataTypeSize(type);
  if (size == 0) {
    SetGLError(GL_INVALID_OPERATION);
    return false;
  }
  result = GetSharedMemoryAs<SizedResult<GLint>*>(
      shm_id, shm_offset, SizedResult<GLint>::ComputeSizeFromBytes(size));
  if (!result) {
    *error = error::kOutOfBounds;
    return false;
  }
  result->size = size;
  return true;
}

error::Error GLES2DecoderImpl::HandleGetUniformiv(
    uint32 immediate_data_size, const gles2::GetUniformiv& c) {
  GLuint program = c.program;
  GLint location = c.location;
  GLuint service_id;
  Error error;
  void* result;
  if (GetUniformSetup(
      program, location, c.params_shm_id, c.params_shm_offset,
      &error, &service_id, &result)) {
    glGetUniformiv(
        service_id, location,
        static_cast<gles2::GetUniformiv::Result*>(result)->GetData());
  }
  return error;
}

error::Error GLES2DecoderImpl::HandleGetUniformfv(
    uint32 immediate_data_size, const gles2::GetUniformfv& c) {
  GLuint program = c.program;
  GLint location = c.location;
  GLuint service_id;
  Error error;
  void* result;
  typedef gles2::GetUniformfv::Result Result;
  if (GetUniformSetup(
      program, location, c.params_shm_id, c.params_shm_offset,
      &error, &service_id, &result)) {
    glGetUniformfv(
        service_id,
        location,
        static_cast<gles2::GetUniformfv::Result*>(result)->GetData());
  }
  return error;
}

error::Error GLES2DecoderImpl::HandleGetShaderPrecisionFormat(
    uint32 immediate_data_size, const gles2::GetShaderPrecisionFormat& c) {
  GLenum shader_type = static_cast<GLenum>(c.shadertype);
  GLenum precision_type = static_cast<GLenum>(c.precisiontype);
  typedef gles2::GetShaderPrecisionFormat::Result Result;
  Result* result = GetSharedMemoryAs<Result*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result));
  if (!result) {
    return error::kOutOfBounds;
  }
  // Check that the client initialized the result.
  if (result->success != 0) {
    return error::kInvalidArguments;
  }
  if (!ValidateGLenumShaderType(shader_type) ||
      !ValidateGLenumShaderPrecision(precision_type)) {
    SetGLError(GL_INVALID_ENUM);
  } else {
    result->success = 1;  // true
    switch (precision_type) {
      case GL_LOW_INT:
      case GL_MEDIUM_INT:
      case GL_HIGH_INT:
        result->min_range = -31;
        result->max_range = 31;
        result->precision = 0;
      case GL_LOW_FLOAT:
      case GL_MEDIUM_FLOAT:
      case GL_HIGH_FLOAT:
        result->min_range = -62;
        result->max_range = 62;
        result->precision = -16;
        break;
      default:
        NOTREACHED();
        break;
    }
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetAttachedShaders(
    uint32 immediate_data_size, const gles2::GetAttachedShaders& c) {
  GLuint service_id;
  uint32 result_size = c.result_size;
  if (!id_manager_->GetServiceId(c.program, &service_id)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  typedef gles2::GetAttachedShaders::Result Result;
  uint32 max_count = Result::ComputeMaxResults(result_size);
  Result* result = GetSharedMemoryAs<Result*>(
      c.result_shm_id, c.result_shm_offset, Result::ComputeSize(max_count));
  if (!result) {
    return error::kOutOfBounds;
  }
  // Check that the client initialized the result.
  if (result->size != 0) {
    return error::kInvalidArguments;
  }
  GLsizei count = 0;
  glGetAttachedShaders(service_id, max_count, &count, result->GetData());
  for (GLsizei ii = 0; ii < count; ++ii) {
    if (!id_manager_->GetClientId(result->GetData()[ii],
                                  &result->GetData()[ii])) {
      NOTREACHED();
      return error::kGenericError;
    }
  }
  result->SetNumResults(count);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetActiveUniform(
    uint32 immediate_data_size, const gles2::GetActiveUniform& c) {
  GLuint program = c.program;
  GLuint index = c.index;
  uint32 name_bucket_id = c.name_bucket_id;
  GLuint service_id;
  typedef gles2::GetActiveUniform::Result Result;
  Result* result = GetSharedMemoryAs<Result*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result));
  if (!result) {
    return error::kOutOfBounds;
  }
  // Check that the client initialized the result.
  if (result->success != 0) {
    return error::kInvalidArguments;
  }
  if (!id_manager_->GetServiceId(program, &service_id)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(service_id);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  const ProgramManager::ProgramInfo::UniformInfo* uniform_info =
      info->GetUniformInfo(index);
  if (!uniform_info) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  result->success = 1;  // true.
  result->size = uniform_info->size;
  result->type = uniform_info->type;
  Bucket* bucket = CreateBucket(name_bucket_id);
  bucket->SetFromString(uniform_info->name);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetActiveAttrib(
    uint32 immediate_data_size, const gles2::GetActiveAttrib& c) {
  GLuint program = c.program;
  GLuint index = c.index;
  uint32 name_bucket_id = c.name_bucket_id;
  GLuint service_id;
  typedef gles2::GetActiveAttrib::Result Result;
  Result* result = GetSharedMemoryAs<Result*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result));
  if (!result) {
    return error::kOutOfBounds;
  }
  // Check that the client initialized the result.
  if (result->success != 0) {
    return error::kInvalidArguments;
  }
  if (!id_manager_->GetServiceId(program, &service_id)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  ProgramManager::ProgramInfo* info = GetProgramInfo(service_id);
  if (!info) {
    // Program was not linked successfully. (ie, glLinkProgram)
    SetGLError(GL_INVALID_OPERATION);
    return error::kNoError;
  }
  const ProgramManager::ProgramInfo::VertexAttribInfo* attrib_info =
      info->GetAttribInfo(index);
  if (!attrib_info) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  result->success = 1;  // true.
  result->size = attrib_info->size;
  result->type = attrib_info->type;
  Bucket* bucket = CreateBucket(name_bucket_id);
  bucket->SetFromString(attrib_info->name);
  return error::kNoError;
}

// Include the auto-generated part of this file. We split this because it means
// we can easily edit the non-auto generated parts right here in this file
// instead of having to edit some template or the code generator.
#include "gpu/command_buffer/service/gles2_cmd_decoder_autogen.h"

}  // namespace gles2
}  // namespace gpu
