// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// platform/show_folder.h on Android: GameActivity.showFiles (port/android/java), called through
// the activity's own class (a game thread's FindClass would not see the app's classes).

#include <jni.h>
#include <string>

#include <SDL3/SDL.h>

#include "common/log.h"

namespace openrac::platform {

bool android_show_files(const std::string& path) {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto* activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (env == nullptr || activity == nullptr) {
        return false;
    }
    jclass type = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    bool shown = false;
    const jmethodID method = env->GetStaticMethodID(type, "showFiles", "(Ljava/lang/String;)Z");
    if (method != nullptr && !env->ExceptionCheck()) {
        jstring where = env->NewStringUTF(path.c_str());
        shown = env->CallStaticBooleanMethod(type, method, where) != JNI_FALSE;
        env->DeleteLocalRef(where);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        log::warn("showFiles failed in Java");
        shown = false;
    }
    env->DeleteLocalRef(type);
    return shown;
}

}  // namespace openrac::platform
