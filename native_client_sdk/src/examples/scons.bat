@echo off

:: Copyright (c) 2011 The Native Client Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

setlocal

:: NACL_SDK_ROOT must be set.
if not defined NACL_SDK_ROOT (
  echo Error: NACL_SDK_ROOT is not set.
  echo Please set NACL_SDK_ROOT to the full path of the Native Client SDK.
  echo For example:
  echo set NACL_SDK_ROOT=D:\nacl_sdk
  goto end
)

:: NACL_TARGET_PLATFORM is really the name of a folder with the base dir -
:: usually NACL_SDK_ROOT - within which the toolchain for the target platform
:: are found.
:: Replace the platform with the name of your target platform.  For example, to
:: build applications that target the pepper_17 API, set
::   NACL_TARGET_PLATFORM=pepper_17
if not defined NACL_TARGET_PLATFORM (
  set NACL_TARGET_PLATFORM=pepper_17
)

set NACL_PLATFORM_DIR=%NACL_SDK_ROOT%\%NACL_TARGET_PLATFORM%

set SCONS_LIB_DIR=%NACL_PLATFORM_DIR%\third_party\scons-2.0.1\engine
set PYTHONPATH=%NACL_PLATFORM_DIR%\third_party\scons-2.0.1\engine

:: We have to do this because scons overrides PYTHONPATH and does not preserve
:: what is provided by the OS.  The custom variable name won't be overwritten.
set PYMOX=%NACL_PLATFORM_DIR%\third_party\pymox

:: Run the included copy of scons.
python -O -OO "%NACL_PLATFORM_DIR%\third_party\scons-2.0.1\script\scons" ^
--warn no-visual-c-missing ^
--file=build.scons ^
--site-dir="%NACL_PLATFORM_DIR%\build_tools\nacl_sdk_scons" %*

:end
