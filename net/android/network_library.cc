// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/android/network_library.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "jni/AndroidNetworkLibrary_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetApplicationContext;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfByteArray;
using base::android::ToJavaByteArray;

namespace net {
namespace android {

void VerifyX509CertChain(const std::vector<std::string>& cert_chain,
                         const std::string& auth_type,
                         const std::string& host,
                         CertVerifyStatusAndroid* status,
                         bool* is_issued_by_known_root,
                         std::vector<std::string>* verified_chain) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobjectArray> chain_byte_array =
      ToJavaArrayOfByteArray(env, cert_chain);
  DCHECK(!chain_byte_array.is_null());

  ScopedJavaLocalRef<jstring> auth_string =
      ConvertUTF8ToJavaString(env, auth_type);
  DCHECK(!auth_string.is_null());

  ScopedJavaLocalRef<jstring> host_string =
      ConvertUTF8ToJavaString(env, host);
  DCHECK(!host_string.is_null());

  ScopedJavaLocalRef<jobject> result =
      Java_AndroidNetworkLibrary_verifyServerCertificates(
          env, chain_byte_array.obj(), auth_string.obj(), host_string.obj());

  ExtractCertVerifyResult(result.obj(),
                          status, is_issued_by_known_root, verified_chain);
}

void AddTestRootCertificate(const uint8* cert, size_t len) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jbyteArray> cert_array = ToJavaByteArray(env, cert, len);
  DCHECK(!cert_array.is_null());
  Java_AndroidNetworkLibrary_addTestRootCertificate(env, cert_array.obj());
}

void ClearTestRootCertificates() {
  JNIEnv* env = AttachCurrentThread();
  Java_AndroidNetworkLibrary_clearTestRootCertificates(env);
}

bool StoreKeyPair(const uint8* public_key,
                  size_t public_len,
                  const uint8* private_key,
                  size_t private_len) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jbyteArray> public_array =
      ToJavaByteArray(env, public_key, public_len);
  ScopedJavaLocalRef<jbyteArray> private_array =
      ToJavaByteArray(env, private_key, private_len);
  jboolean ret = Java_AndroidNetworkLibrary_storeKeyPair(env,
      GetApplicationContext(), public_array.obj(), private_array.obj());
  LOG_IF(WARNING, !ret) <<
      "Call to Java_AndroidNetworkLibrary_storeKeyPair failed";
  return ret;
}

void StoreCertificate(net::CertificateMimeType cert_type,
                      const void* data,
                      size_t data_len) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jbyteArray> data_array =
      ToJavaByteArray(env, reinterpret_cast<const uint8*>(data), data_len);
  jboolean ret = Java_AndroidNetworkLibrary_storeCertificate(env,
      GetApplicationContext(), cert_type, data_array.obj());
  LOG_IF(WARNING, !ret) <<
      "Call to Java_AndroidNetworkLibrary_storeCertificate"
      " failed";
  // Intentionally do not return 'ret', there is little the caller can
  // do in case of failure (the CertInstaller itself will deal with
  // incorrect data and display the appropriate toast).
}

bool HaveOnlyLoopbackAddresses() {
  JNIEnv* env = AttachCurrentThread();
  return Java_AndroidNetworkLibrary_haveOnlyLoopbackAddresses(env);
}

std::string GetNetworkList() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> ret =
      Java_AndroidNetworkLibrary_getNetworkList(env);
  return ConvertJavaStringToUTF8(ret);
}

bool GetMimeTypeFromExtension(const std::string& extension,
                              std::string* result) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> extension_string =
      ConvertUTF8ToJavaString(env, extension);
  ScopedJavaLocalRef<jstring> ret =
      Java_AndroidNetworkLibrary_getMimeTypeFromExtension(
          env, extension_string.obj());

  if (!ret.obj())
    return false;
  *result = ConvertJavaStringToUTF8(ret);
  return true;
}

bool RegisterNetworkLibrary(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace android
}  // namespace net
