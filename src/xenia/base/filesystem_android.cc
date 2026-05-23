/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <android/log.h>
#include <cstdio>
#include <filesystem>
#include <jni.h>
#include <cstring>

#include "xenia/base/filesystem.h"
#include "xenia/base/main_android.h"
#include "xenia/base/mapped_memory.h"
#include "xenia/base/math.h"
#include "xenia/base/string.h"

namespace xe {
namespace filesystem {

// Using Android logging because the file system may need to be initialized
// before logging.

static jobject android_content_resolver_;
static jclass android_content_resolver_class_;
static jmethodID android_content_resolver_open_file_descriptor_;

static jclass android_parcel_file_descriptor_class_;
static jmethodID android_parcel_file_descriptor_detach_fd_;

static jclass android_uri_class_;
static jmethodID android_uri_parse_;

static bool android_content_resolver_initialized_;

static void AndroidShutdownContentResolver() {
  android_content_resolver_initialized_ = false;
  android_uri_parse_ = nullptr;
  android_parcel_file_descriptor_detach_fd_ = nullptr;
  android_content_resolver_open_file_descriptor_ = nullptr;
  JNIEnv* jni_env = GetAndroidThreadJniEnv();
  if (jni_env) {
    if (android_uri_class_) {
      jni_env->DeleteGlobalRef(android_uri_class_);
    }
    if (android_parcel_file_descriptor_class_) {
      jni_env->DeleteGlobalRef(
          reinterpret_cast<jobject>(android_parcel_file_descriptor_class_));
    }
    if (android_content_resolver_class_) {
      jni_env->DeleteGlobalRef(
          reinterpret_cast<jobject>(android_content_resolver_class_));
    }
    if (android_content_resolver_) {
      jni_env->DeleteGlobalRef(
          reinterpret_cast<jobject>(android_content_resolver_));
    }
  }
  android_uri_class_ = nullptr;
  android_parcel_file_descriptor_class_ = nullptr;
  android_content_resolver_class_ = nullptr;
  android_content_resolver_ = nullptr;
}

static void AndroidInitializeContentResolver() {
  JNIEnv* jni_env = GetAndroidThreadJniEnv();
  if (!jni_env) {
    return;
  }
  jobject application_context = GetAndroidApplicationContext();
  if (!application_context) {
    return;
  }
  {
    jclass context_class = jni_env->GetObjectClass(application_context);
    if (!context_class) {
      __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                          "Failed to get the context class");
      AndroidShutdownContentResolver();
      return;
    }
    jmethodID context_get_content_resolver =
        jni_env->GetMethodID(context_class, "getContentResolver",
                             "()Landroid/content/ContentResolver;");
    if (!context_get_content_resolver) {
      __android_log_write(
          ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
          "Failed to get the getContentResolver method of the context");
      jni_env->DeleteLocalRef(reinterpret_cast<jobject>(context_class));
      AndroidShutdownContentResolver();
      return;
    }
    jobject content_resolver = jni_env->CallObjectMethod(
        application_context, context_get_content_resolver);
    jni_env->DeleteLocalRef(reinterpret_cast<jobject>(context_class));
    if (!content_resolver) {
      __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                          "Failed to get the content resolver");
      AndroidShutdownContentResolver();
      return;
    }
    android_content_resolver_ = jni_env->NewGlobalRef(content_resolver);
    jni_env->DeleteLocalRef(content_resolver);
  }
  if (!android_content_resolver_) {
    __android_log_write(
        ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
        "Failed to create a global reference to the content resolver");
    AndroidShutdownContentResolver();
    return;
  }
  {
    jobject content_resolver_class =
        jni_env->GetObjectClass(android_content_resolver_);
    if (!content_resolver_class) {
      __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                          "Failed to get the content resolver class");
      AndroidShutdownContentResolver();
      return;
    }
    android_content_resolver_class_ =
        reinterpret_cast<jclass>(jni_env->NewGlobalRef(
            reinterpret_cast<jobject>(content_resolver_class)));
    jni_env->DeleteLocalRef(reinterpret_cast<jobject>(content_resolver_class));
  }
  if (!android_content_resolver_class_) {
    __android_log_write(
        ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
        "Failed to create a global reference to the content resolver class");
    AndroidShutdownContentResolver();
    return;
  }
  android_content_resolver_open_file_descriptor_ = jni_env->GetMethodID(
      android_content_resolver_class_, "openFileDescriptor",
      "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;");
  if (!android_content_resolver_open_file_descriptor_) {
    __android_log_write(
        ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
        "Failed to get the openFileDescriptor method of the content resolver");
    AndroidShutdownContentResolver();
    return;
  }

  {
    jclass parcel_file_descriptor_class =
        jni_env->FindClass("android/os/ParcelFileDescriptor");
    if (!parcel_file_descriptor_class) {
      __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                          "Failed to get the parcel file descriptor class");
      AndroidShutdownContentResolver();
      return;
    }
    android_parcel_file_descriptor_class_ =
        reinterpret_cast<jclass>(jni_env->NewGlobalRef(
            reinterpret_cast<jobject>(parcel_file_descriptor_class)));
    jni_env->DeleteLocalRef(
        reinterpret_cast<jobject>(parcel_file_descriptor_class));
  }
  if (!android_parcel_file_descriptor_class_) {
    __android_log_write(
        ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
        "Failed to create a global reference to the parcel file descriptor "
        "class");
    AndroidShutdownContentResolver();
    return;
  }
  android_parcel_file_descriptor_detach_fd_ = jni_env->GetMethodID(
      android_parcel_file_descriptor_class_, "detachFd", "()I");
  if (!android_parcel_file_descriptor_detach_fd_) {
    __android_log_write(
        ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
        "Failed to get the detachFd method of the parcel file descriptor");
    AndroidShutdownContentResolver();
    return;
  }

  {
    jclass uri_class = jni_env->FindClass("android/net/Uri");
    if (!uri_class) {
      __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                          "Failed to get the URI class");
      AndroidShutdownContentResolver();
      return;
    }
    android_uri_class_ = reinterpret_cast<jclass>(
        jni_env->NewGlobalRef(reinterpret_cast<jobject>(uri_class)));
    jni_env->DeleteLocalRef(reinterpret_cast<jobject>(uri_class));
  }
  if (!android_uri_class_) {
    __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                        "Failed to create a global reference to the URI class");
    AndroidShutdownContentResolver();
    return;
  }
  android_uri_parse_ = jni_env->GetStaticMethodID(
      android_uri_class_, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
  if (!android_uri_parse_) {
    __android_log_write(ANDROID_LOG_ERROR, "AndroidInitializeContentResolver",
                        "Failed to get the parse method of the URI class");
    AndroidShutdownContentResolver();
    return;
  }

  android_content_resolver_initialized_ = true;
}

void AndroidInitialize() { AndroidInitializeContentResolver(); }

void AndroidShutdown() { AndroidShutdownContentResolver(); }

bool IsAndroidContentUri(const std::string_view source) {
  // A URI schema is case-insensitive. Though just content: defines the schema,
  // still including // in the comparison to distinguish from a file with a name
  // starting from content: (as this is the main purpose of this code -
  // separating URIs from file paths) more clearly.
  static constexpr char kContentSchema[] = "content://";
  constexpr size_t kContentSchemaLength = xe::countof(kContentSchema) - 1;
  return source.size() >= kContentSchemaLength &&
         !xe_strncasecmp(source.data(), kContentSchema, kContentSchemaLength);
}

int OpenAndroidContentFileDescriptor(const std::string_view uri,
                                     const char* mode) {
  if (!android_content_resolver_initialized_) {
    return -1;
  }
  JNIEnv* jni_env = GetAndroidThreadJniEnv();
  if (!jni_env) {
    return -1;
  }

  jobject uri_object;
  {
    jstring uri_string;
    {
      std::u16string uri_u16 = xe::to_utf16(uri);
      uri_string = jni_env->NewString(
          reinterpret_cast<const jchar*>(uri_u16.data()), uri_u16.size());
    }
    if (!uri_string) {
      return -1;
    }
    uri_object = jni_env->CallStaticObjectMethod(
        android_uri_class_, android_uri_parse_, uri_string);
    jni_env->DeleteLocalRef(uri_string);
  }
  if (!uri_object) {
    return -1;
  }

  jstring mode_string = jni_env->NewStringUTF(mode);
  if (!mode_string) {
    jni_env->DeleteLocalRef(uri_object);
    return -1;
  }

  jobject parcel_file_descriptor = jni_env->CallObjectMethod(
      android_content_resolver_, android_content_resolver_open_file_descriptor_,
      uri_object, mode_string);
  jni_env->DeleteLocalRef(mode_string);
  jni_env->DeleteLocalRef(uri_object);
  if (jni_env->ExceptionCheck()) {
    jni_env->ExceptionClear();
    return -1;
  }
  if (!parcel_file_descriptor) {
    return -1;
  }
  int file_descriptor = jni_env->CallIntMethod(
      parcel_file_descriptor, android_parcel_file_descriptor_detach_fd_);
  jni_env->DeleteLocalRef(parcel_file_descriptor);

  return file_descriptor;
}

bool SetAttributes(const std::filesystem::path& path, uint64_t attributes) {
  return false;
}

static std::filesystem::path AndroidPathFromContextDirMethod(
    const char* method_name) {
  JNIEnv* jni_env = GetAndroidThreadJniEnv();
  jobject application_context = GetAndroidApplicationContext();
  if (!jni_env || !application_context) {
    return {};
  }
  jclass context_class = jni_env->GetObjectClass(application_context);
  if (!context_class) {
    return {};
  }
  jmethodID get_dir = jni_env->GetMethodID(context_class, method_name,
                                           "()Ljava/io/File;");
  jni_env->DeleteLocalRef(context_class);
  if (!get_dir) {
    return {};
  }
  jobject file_object =
      jni_env->CallObjectMethod(application_context, get_dir);
  if (!file_object) {
    return {};
  }
  jclass file_class = jni_env->GetObjectClass(file_object);
  jmethodID get_absolute_path =
      jni_env->GetMethodID(file_class, "getAbsolutePath", "()Ljava/lang/String;");
  jni_env->DeleteLocalRef(file_class);
  if (!get_absolute_path) {
    jni_env->DeleteLocalRef(file_object);
    return {};
  }
  jstring path_string = static_cast<jstring>(
      jni_env->CallObjectMethod(file_object, get_absolute_path));
  jni_env->DeleteLocalRef(file_object);
  if (!path_string) {
    return {};
  }
  const char* path_utf8 = jni_env->GetStringUTFChars(path_string, nullptr);
  std::filesystem::path result;
  if (path_utf8) {
    result = path_utf8;
    jni_env->ReleaseStringUTFChars(path_string, path_utf8);
  }
  jni_env->DeleteLocalRef(path_string);
  return result;
}

std::filesystem::path GetAndroidApplicationFilesDirectory() {
  return AndroidPathFromContextDirMethod("getFilesDir");
}

std::filesystem::path GetAndroidApplicationCacheDirectory() {
  return AndroidPathFromContextDirMethod("getCacheDir");
}

bool AndroidRequestOpenDocument(const char* mime_type) {
  JNIEnv* jni_env = GetAndroidThreadJniEnv();
  jobject application_context = GetAndroidApplicationContext();
  if (!jni_env || !application_context) {
    return false;
  }
  jclass intent_class = jni_env->FindClass("android/content/Intent");
  if (!intent_class) {
    return false;
  }
  jmethodID intent_init = jni_env->GetMethodID(
      intent_class, "<init>", "(Ljava/lang/String;)V");
  jstring action_open =
      jni_env->NewStringUTF("android.intent.action.OPEN_DOCUMENT");
  if (!intent_init || !action_open) {
    jni_env->DeleteLocalRef(intent_class);
    return false;
  }
  jobject intent =
      jni_env->NewObject(intent_class, intent_init, action_open);
  jni_env->DeleteLocalRef(action_open);
  if (!intent) {
    jni_env->DeleteLocalRef(intent_class);
    return false;
  }
  jmethodID add_category = jni_env->GetMethodID(
      intent_class, "addCategory", "(Ljava/lang/String;)Landroid/content/Intent;");
  jmethodID set_type =
      jni_env->GetMethodID(intent_class, "setType", "(Ljava/lang/String;)Landroid/content/Intent;");
  jmethodID add_flags = jni_env->GetMethodID(
      intent_class, "addFlags", "(I)Landroid/content/Intent;");
  if (add_category) {
    jstring category =
        jni_env->NewStringUTF("android.intent.category.OPENABLE");
    jni_env->CallObjectMethod(intent, add_category, category);
    jni_env->DeleteLocalRef(category);
  }
  if (set_type && mime_type) {
    jstring mime = jni_env->NewStringUTF(mime_type);
    jni_env->CallObjectMethod(intent, set_type, mime);
    jni_env->DeleteLocalRef(mime);
  }
  if (add_flags) {
    jni_env->CallObjectMethod(intent, add_flags, 0x10000000);  // FLAG_ACTIVITY_NEW_TASK
  }
  jclass context_class = jni_env->GetObjectClass(application_context);
  jmethodID start_activity = jni_env->GetMethodID(
      context_class, "startActivity", "(Landroid/content/Intent;)V");
  jni_env->DeleteLocalRef(context_class);
  jni_env->DeleteLocalRef(intent_class);
  if (!start_activity) {
    jni_env->DeleteLocalRef(intent);
    return false;
  }
  jni_env->CallVoidMethod(application_context, start_activity, intent);
  jni_env->DeleteLocalRef(intent);
  if (jni_env->ExceptionCheck()) {
    jni_env->ExceptionClear();
    return false;
  }
  return true;
}

std::filesystem::path StageAndroidLaunchPath(const std::filesystem::path& path) {
  const std::string uri = xe::path_to_utf8(path);
  if (!IsAndroidContentUri(uri)) {
    return path;
  }

  auto mmap =
      MappedMemory::OpenForAndroidContentUri(uri, MappedMemory::Mode::kRead);
  if (!mmap || mmap->size() < 4) {
    return {};
  }

  const uint8_t* data = mmap->data();
  auto magic_is = [data](char a, char b, char c, char d) {
    return data[0] == static_cast<uint8_t>(a) &&
           data[1] == static_cast<uint8_t>(b) &&
           data[2] == static_cast<uint8_t>(c) &&
           data[3] == static_cast<uint8_t>(d);
  };
  std::string extension = ".xex";
  if (magic_is('X', 'E', 'X', '1') || magic_is('X', 'E', 'X', '2')) {
    extension = ".xex";
  } else if (magic_is('X', 'S', 'F', static_cast<char>(0x1A))) {
    extension = ".iso";
  } else if (magic_is('C', 'O', 'N', ' ') || magic_is('L', 'I', 'V', 'E') ||
             magic_is('P', 'I', 'R', 'S')) {
    extension = "";
  }

  const auto dest_dir = GetAndroidApplicationCacheDirectory() / "launch";
  std::error_code ec;
  std::filesystem::create_directories(dest_dir, ec);
  std::filesystem::path dest = dest_dir / ("title" + std::string(extension));

  FILE* file = fopen(dest.string().c_str(), "wb");
  if (!file) {
    return {};
  }
  const size_t written =
      fwrite(data, 1, mmap->size(), file) == mmap->size() ? mmap->size() : 0;
  fclose(file);
  if (written != mmap->size()) {
    std::filesystem::remove(dest, ec);
    return {};
  }
  return dest;
}

}  // namespace filesystem
}  // namespace xe
