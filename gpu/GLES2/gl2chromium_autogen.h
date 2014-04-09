// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_gles2_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

// This file contains Chromium-specific GLES2 declarations.

#ifndef GPU_GLES2_GL2CHROMIUM_AUTOGEN_H_
#define GPU_GLES2_GL2CHROMIUM_AUTOGEN_H_

#define glActiveTexture GLES2_GET_FUN(ActiveTexture)
#define glAttachShader GLES2_GET_FUN(AttachShader)
#define glBindAttribLocation GLES2_GET_FUN(BindAttribLocation)
#define glBindBuffer GLES2_GET_FUN(BindBuffer)
#define glBindFramebuffer GLES2_GET_FUN(BindFramebuffer)
#define glBindRenderbuffer GLES2_GET_FUN(BindRenderbuffer)
#define glBindTexture GLES2_GET_FUN(BindTexture)
#define glBlendColor GLES2_GET_FUN(BlendColor)
#define glBlendEquation GLES2_GET_FUN(BlendEquation)
#define glBlendEquationSeparate GLES2_GET_FUN(BlendEquationSeparate)
#define glBlendFunc GLES2_GET_FUN(BlendFunc)
#define glBlendFuncSeparate GLES2_GET_FUN(BlendFuncSeparate)
#define glBufferData GLES2_GET_FUN(BufferData)
#define glBufferSubData GLES2_GET_FUN(BufferSubData)
#define glCheckFramebufferStatus GLES2_GET_FUN(CheckFramebufferStatus)
#define glClear GLES2_GET_FUN(Clear)
#define glClearColor GLES2_GET_FUN(ClearColor)
#define glClearDepthf GLES2_GET_FUN(ClearDepthf)
#define glClearStencil GLES2_GET_FUN(ClearStencil)
#define glColorMask GLES2_GET_FUN(ColorMask)
#define glCompileShader GLES2_GET_FUN(CompileShader)
#define glCompressedTexImage2D GLES2_GET_FUN(CompressedTexImage2D)
#define glCompressedTexSubImage2D GLES2_GET_FUN(CompressedTexSubImage2D)
#define glCopyTexImage2D GLES2_GET_FUN(CopyTexImage2D)
#define glCopyTexSubImage2D GLES2_GET_FUN(CopyTexSubImage2D)
#define glCreateProgram GLES2_GET_FUN(CreateProgram)
#define glCreateShader GLES2_GET_FUN(CreateShader)
#define glCullFace GLES2_GET_FUN(CullFace)
#define glDeleteBuffers GLES2_GET_FUN(DeleteBuffers)
#define glDeleteFramebuffers GLES2_GET_FUN(DeleteFramebuffers)
#define glDeleteProgram GLES2_GET_FUN(DeleteProgram)
#define glDeleteRenderbuffers GLES2_GET_FUN(DeleteRenderbuffers)
#define glDeleteShader GLES2_GET_FUN(DeleteShader)
#define glDeleteTextures GLES2_GET_FUN(DeleteTextures)
#define glDepthFunc GLES2_GET_FUN(DepthFunc)
#define glDepthMask GLES2_GET_FUN(DepthMask)
#define glDepthRangef GLES2_GET_FUN(DepthRangef)
#define glDetachShader GLES2_GET_FUN(DetachShader)
#define glDisable GLES2_GET_FUN(Disable)
#define glDisableVertexAttribArray GLES2_GET_FUN(DisableVertexAttribArray)
#define glDrawArrays GLES2_GET_FUN(DrawArrays)
#define glDrawElements GLES2_GET_FUN(DrawElements)
#define glEnable GLES2_GET_FUN(Enable)
#define glEnableVertexAttribArray GLES2_GET_FUN(EnableVertexAttribArray)
#define glFinish GLES2_GET_FUN(Finish)
#define glFlush GLES2_GET_FUN(Flush)
#define glFramebufferRenderbuffer GLES2_GET_FUN(FramebufferRenderbuffer)
#define glFramebufferTexture2D GLES2_GET_FUN(FramebufferTexture2D)
#define glFrontFace GLES2_GET_FUN(FrontFace)
#define glGenBuffers GLES2_GET_FUN(GenBuffers)
#define glGenerateMipmap GLES2_GET_FUN(GenerateMipmap)
#define glGenFramebuffers GLES2_GET_FUN(GenFramebuffers)
#define glGenRenderbuffers GLES2_GET_FUN(GenRenderbuffers)
#define glGenTextures GLES2_GET_FUN(GenTextures)
#define glGetActiveAttrib GLES2_GET_FUN(GetActiveAttrib)
#define glGetActiveUniform GLES2_GET_FUN(GetActiveUniform)
#define glGetAttachedShaders GLES2_GET_FUN(GetAttachedShaders)
#define glGetAttribLocation GLES2_GET_FUN(GetAttribLocation)
#define glGetBooleanv GLES2_GET_FUN(GetBooleanv)
#define glGetBufferParameteriv GLES2_GET_FUN(GetBufferParameteriv)
#define glGetError GLES2_GET_FUN(GetError)
#define glGetFloatv GLES2_GET_FUN(GetFloatv)
#define glGetFramebufferAttachmentParameteriv \
  GLES2_GET_FUN(GetFramebufferAttachmentParameteriv)
#define glGetIntegerv GLES2_GET_FUN(GetIntegerv)
#define glGetProgramiv GLES2_GET_FUN(GetProgramiv)
#define glGetProgramInfoLog GLES2_GET_FUN(GetProgramInfoLog)
#define glGetRenderbufferParameteriv GLES2_GET_FUN(GetRenderbufferParameteriv)
#define glGetShaderiv GLES2_GET_FUN(GetShaderiv)
#define glGetShaderInfoLog GLES2_GET_FUN(GetShaderInfoLog)
#define glGetShaderPrecisionFormat GLES2_GET_FUN(GetShaderPrecisionFormat)
#define glGetShaderSource GLES2_GET_FUN(GetShaderSource)
#define glGetString GLES2_GET_FUN(GetString)
#define glGetTexParameterfv GLES2_GET_FUN(GetTexParameterfv)
#define glGetTexParameteriv GLES2_GET_FUN(GetTexParameteriv)
#define glGetUniformfv GLES2_GET_FUN(GetUniformfv)
#define glGetUniformiv GLES2_GET_FUN(GetUniformiv)
#define glGetUniformLocation GLES2_GET_FUN(GetUniformLocation)
#define glGetVertexAttribfv GLES2_GET_FUN(GetVertexAttribfv)
#define glGetVertexAttribiv GLES2_GET_FUN(GetVertexAttribiv)
#define glGetVertexAttribPointerv GLES2_GET_FUN(GetVertexAttribPointerv)
#define glHint GLES2_GET_FUN(Hint)
#define glIsBuffer GLES2_GET_FUN(IsBuffer)
#define glIsEnabled GLES2_GET_FUN(IsEnabled)
#define glIsFramebuffer GLES2_GET_FUN(IsFramebuffer)
#define glIsProgram GLES2_GET_FUN(IsProgram)
#define glIsRenderbuffer GLES2_GET_FUN(IsRenderbuffer)
#define glIsShader GLES2_GET_FUN(IsShader)
#define glIsTexture GLES2_GET_FUN(IsTexture)
#define glLineWidth GLES2_GET_FUN(LineWidth)
#define glLinkProgram GLES2_GET_FUN(LinkProgram)
#define glPixelStorei GLES2_GET_FUN(PixelStorei)
#define glPolygonOffset GLES2_GET_FUN(PolygonOffset)
#define glReadPixels GLES2_GET_FUN(ReadPixels)
#define glReleaseShaderCompiler GLES2_GET_FUN(ReleaseShaderCompiler)
#define glRenderbufferStorage GLES2_GET_FUN(RenderbufferStorage)
#define glSampleCoverage GLES2_GET_FUN(SampleCoverage)
#define glScissor GLES2_GET_FUN(Scissor)
#define glShaderBinary GLES2_GET_FUN(ShaderBinary)
#define glShaderSource GLES2_GET_FUN(ShaderSource)
#define glShallowFinishCHROMIUM GLES2_GET_FUN(ShallowFinishCHROMIUM)
#define glShallowFlushCHROMIUM GLES2_GET_FUN(ShallowFlushCHROMIUM)
#define glStencilFunc GLES2_GET_FUN(StencilFunc)
#define glStencilFuncSeparate GLES2_GET_FUN(StencilFuncSeparate)
#define glStencilMask GLES2_GET_FUN(StencilMask)
#define glStencilMaskSeparate GLES2_GET_FUN(StencilMaskSeparate)
#define glStencilOp GLES2_GET_FUN(StencilOp)
#define glStencilOpSeparate GLES2_GET_FUN(StencilOpSeparate)
#define glTexImage2D GLES2_GET_FUN(TexImage2D)
#define glTexParameterf GLES2_GET_FUN(TexParameterf)
#define glTexParameterfv GLES2_GET_FUN(TexParameterfv)
#define glTexParameteri GLES2_GET_FUN(TexParameteri)
#define glTexParameteriv GLES2_GET_FUN(TexParameteriv)
#define glTexSubImage2D GLES2_GET_FUN(TexSubImage2D)
#define glUniform1f GLES2_GET_FUN(Uniform1f)
#define glUniform1fv GLES2_GET_FUN(Uniform1fv)
#define glUniform1i GLES2_GET_FUN(Uniform1i)
#define glUniform1iv GLES2_GET_FUN(Uniform1iv)
#define glUniform2f GLES2_GET_FUN(Uniform2f)
#define glUniform2fv GLES2_GET_FUN(Uniform2fv)
#define glUniform2i GLES2_GET_FUN(Uniform2i)
#define glUniform2iv GLES2_GET_FUN(Uniform2iv)
#define glUniform3f GLES2_GET_FUN(Uniform3f)
#define glUniform3fv GLES2_GET_FUN(Uniform3fv)
#define glUniform3i GLES2_GET_FUN(Uniform3i)
#define glUniform3iv GLES2_GET_FUN(Uniform3iv)
#define glUniform4f GLES2_GET_FUN(Uniform4f)
#define glUniform4fv GLES2_GET_FUN(Uniform4fv)
#define glUniform4i GLES2_GET_FUN(Uniform4i)
#define glUniform4iv GLES2_GET_FUN(Uniform4iv)
#define glUniformMatrix2fv GLES2_GET_FUN(UniformMatrix2fv)
#define glUniformMatrix3fv GLES2_GET_FUN(UniformMatrix3fv)
#define glUniformMatrix4fv GLES2_GET_FUN(UniformMatrix4fv)
#define glUseProgram GLES2_GET_FUN(UseProgram)
#define glValidateProgram GLES2_GET_FUN(ValidateProgram)
#define glVertexAttrib1f GLES2_GET_FUN(VertexAttrib1f)
#define glVertexAttrib1fv GLES2_GET_FUN(VertexAttrib1fv)
#define glVertexAttrib2f GLES2_GET_FUN(VertexAttrib2f)
#define glVertexAttrib2fv GLES2_GET_FUN(VertexAttrib2fv)
#define glVertexAttrib3f GLES2_GET_FUN(VertexAttrib3f)
#define glVertexAttrib3fv GLES2_GET_FUN(VertexAttrib3fv)
#define glVertexAttrib4f GLES2_GET_FUN(VertexAttrib4f)
#define glVertexAttrib4fv GLES2_GET_FUN(VertexAttrib4fv)
#define glVertexAttribPointer GLES2_GET_FUN(VertexAttribPointer)
#define glViewport GLES2_GET_FUN(Viewport)
#define glBlitFramebufferCHROMIUM GLES2_GET_FUN(BlitFramebufferCHROMIUM)
#define glRenderbufferStorageMultisampleCHROMIUM \
  GLES2_GET_FUN(RenderbufferStorageMultisampleCHROMIUM)
#define glRenderbufferStorageMultisampleEXT \
  GLES2_GET_FUN(RenderbufferStorageMultisampleEXT)
#define glFramebufferTexture2DMultisampleEXT \
  GLES2_GET_FUN(FramebufferTexture2DMultisampleEXT)
#define glTexStorage2DEXT GLES2_GET_FUN(TexStorage2DEXT)
#define glGenQueriesEXT GLES2_GET_FUN(GenQueriesEXT)
#define glDeleteQueriesEXT GLES2_GET_FUN(DeleteQueriesEXT)
#define glIsQueryEXT GLES2_GET_FUN(IsQueryEXT)
#define glBeginQueryEXT GLES2_GET_FUN(BeginQueryEXT)
#define glEndQueryEXT GLES2_GET_FUN(EndQueryEXT)
#define glGetQueryivEXT GLES2_GET_FUN(GetQueryivEXT)
#define glGetQueryObjectuivEXT GLES2_GET_FUN(GetQueryObjectuivEXT)
#define glInsertEventMarkerEXT GLES2_GET_FUN(InsertEventMarkerEXT)
#define glPushGroupMarkerEXT GLES2_GET_FUN(PushGroupMarkerEXT)
#define glPopGroupMarkerEXT GLES2_GET_FUN(PopGroupMarkerEXT)
#define glGenVertexArraysOES GLES2_GET_FUN(GenVertexArraysOES)
#define glDeleteVertexArraysOES GLES2_GET_FUN(DeleteVertexArraysOES)
#define glIsVertexArrayOES GLES2_GET_FUN(IsVertexArrayOES)
#define glBindVertexArrayOES GLES2_GET_FUN(BindVertexArrayOES)
#define glSwapBuffers GLES2_GET_FUN(SwapBuffers)
#define glGetMaxValueInBufferCHROMIUM GLES2_GET_FUN(GetMaxValueInBufferCHROMIUM)
#define glGenSharedIdsCHROMIUM GLES2_GET_FUN(GenSharedIdsCHROMIUM)
#define glDeleteSharedIdsCHROMIUM GLES2_GET_FUN(DeleteSharedIdsCHROMIUM)
#define glRegisterSharedIdsCHROMIUM GLES2_GET_FUN(RegisterSharedIdsCHROMIUM)
#define glEnableFeatureCHROMIUM GLES2_GET_FUN(EnableFeatureCHROMIUM)
#define glMapBufferCHROMIUM GLES2_GET_FUN(MapBufferCHROMIUM)
#define glUnmapBufferCHROMIUM GLES2_GET_FUN(UnmapBufferCHROMIUM)
#define glMapImageCHROMIUM GLES2_GET_FUN(MapImageCHROMIUM)
#define glUnmapImageCHROMIUM GLES2_GET_FUN(UnmapImageCHROMIUM)
#define glMapBufferSubDataCHROMIUM GLES2_GET_FUN(MapBufferSubDataCHROMIUM)
#define glUnmapBufferSubDataCHROMIUM GLES2_GET_FUN(UnmapBufferSubDataCHROMIUM)
#define glMapTexSubImage2DCHROMIUM GLES2_GET_FUN(MapTexSubImage2DCHROMIUM)
#define glUnmapTexSubImage2DCHROMIUM GLES2_GET_FUN(UnmapTexSubImage2DCHROMIUM)
#define glResizeCHROMIUM GLES2_GET_FUN(ResizeCHROMIUM)
#define glGetRequestableExtensionsCHROMIUM \
  GLES2_GET_FUN(GetRequestableExtensionsCHROMIUM)
#define glRequestExtensionCHROMIUM GLES2_GET_FUN(RequestExtensionCHROMIUM)
#define glRateLimitOffscreenContextCHROMIUM \
  GLES2_GET_FUN(RateLimitOffscreenContextCHROMIUM)
#define glGetMultipleIntegervCHROMIUM GLES2_GET_FUN(GetMultipleIntegervCHROMIUM)
#define glGetProgramInfoCHROMIUM GLES2_GET_FUN(GetProgramInfoCHROMIUM)
#define glCreateStreamTextureCHROMIUM GLES2_GET_FUN(CreateStreamTextureCHROMIUM)
#define glCreateImageCHROMIUM GLES2_GET_FUN(CreateImageCHROMIUM)
#define glDestroyImageCHROMIUM GLES2_GET_FUN(DestroyImageCHROMIUM)
#define glGetImageParameterivCHROMIUM GLES2_GET_FUN(GetImageParameterivCHROMIUM)
#define glGetTranslatedShaderSourceANGLE \
  GLES2_GET_FUN(GetTranslatedShaderSourceANGLE)
#define glPostSubBufferCHROMIUM GLES2_GET_FUN(PostSubBufferCHROMIUM)
#define glTexImageIOSurface2DCHROMIUM GLES2_GET_FUN(TexImageIOSurface2DCHROMIUM)
#define glCopyTextureCHROMIUM GLES2_GET_FUN(CopyTextureCHROMIUM)
#define glDrawArraysInstancedANGLE GLES2_GET_FUN(DrawArraysInstancedANGLE)
#define glDrawElementsInstancedANGLE GLES2_GET_FUN(DrawElementsInstancedANGLE)
#define glVertexAttribDivisorANGLE GLES2_GET_FUN(VertexAttribDivisorANGLE)
#define glGenMailboxCHROMIUM GLES2_GET_FUN(GenMailboxCHROMIUM)
#define glProduceTextureCHROMIUM GLES2_GET_FUN(ProduceTextureCHROMIUM)
#define glConsumeTextureCHROMIUM GLES2_GET_FUN(ConsumeTextureCHROMIUM)
#define glBindUniformLocationCHROMIUM GLES2_GET_FUN(BindUniformLocationCHROMIUM)
#define glBindTexImage2DCHROMIUM GLES2_GET_FUN(BindTexImage2DCHROMIUM)
#define glReleaseTexImage2DCHROMIUM GLES2_GET_FUN(ReleaseTexImage2DCHROMIUM)
#define glTraceBeginCHROMIUM GLES2_GET_FUN(TraceBeginCHROMIUM)
#define glTraceEndCHROMIUM GLES2_GET_FUN(TraceEndCHROMIUM)
#define glAsyncTexSubImage2DCHROMIUM GLES2_GET_FUN(AsyncTexSubImage2DCHROMIUM)
#define glAsyncTexImage2DCHROMIUM GLES2_GET_FUN(AsyncTexImage2DCHROMIUM)
#define glWaitAsyncTexImage2DCHROMIUM GLES2_GET_FUN(WaitAsyncTexImage2DCHROMIUM)
#define glWaitAllAsyncTexImage2DCHROMIUM \
  GLES2_GET_FUN(WaitAllAsyncTexImage2DCHROMIUM)
#define glDiscardFramebufferEXT GLES2_GET_FUN(DiscardFramebufferEXT)
#define glLoseContextCHROMIUM GLES2_GET_FUN(LoseContextCHROMIUM)
#define glInsertSyncPointCHROMIUM GLES2_GET_FUN(InsertSyncPointCHROMIUM)
#define glWaitSyncPointCHROMIUM GLES2_GET_FUN(WaitSyncPointCHROMIUM)
#define glDrawBuffersEXT GLES2_GET_FUN(DrawBuffersEXT)
#define glDiscardBackbufferCHROMIUM GLES2_GET_FUN(DiscardBackbufferCHROMIUM)
#define glScheduleOverlayPlaneCHROMIUM \
  GLES2_GET_FUN(ScheduleOverlayPlaneCHROMIUM)

#endif  // GPU_GLES2_GL2CHROMIUM_AUTOGEN_H_
