// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// platform/display_hdr.h: GameActivity's static methods (port/android/java), called through the
// activity's own class (a game thread's FindClass would not see the app's classes).

#include "platform/display_hdr.h"

#include <jni.h>

#include <SDL3/SDL.h>

#include "common/log.h"

namespace openrac::platform {

namespace {

// The activity's class and `env`, or null with nothing to clean when SDL has no activity.
jclass activity_class(JNIEnv*& env) {
    env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto* activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (env == nullptr || activity == nullptr) {
        return nullptr;
    }
    jclass type = env->GetObjectClass(activity);
    env->DeleteLocalRef(activity);
    return type;
}

// A Java exception is logged and cleared, so that the next JNI call works.
bool failed(JNIEnv* env, const char* what) {
    if (!env->ExceptionCheck()) {
        return false;
    }
    env->ExceptionClear();
    log::warn("HDR: {} failed in Java", what);
    return true;
}

}  // namespace

bool android_display_hdr(hdr::Display& display) {
    JNIEnv* env = nullptr;
    jclass type = activity_class(env);
    if (type == nullptr) {
        return false;
    }
    bool read = false;
    const jmethodID method = env->GetStaticMethodID(type, "displayHdr", "()[F");
    if (method != nullptr && !failed(env, "displayHdr")) {
        auto* values = static_cast<jfloatArray>(env->CallStaticObjectMethod(type, method));
        if (!failed(env, "displayHdr") && values != nullptr && env->GetArrayLength(values) >= 5) {
            jfloat got[5] = {};
            env->GetFloatArrayRegion(values, 0, 5, got);
            display.known = true;
            display.hdr_enabled = got[0] != 0.0f;
            display.peak_nits = got[1];
            display.sdr_white_nits = got[2];
            display.relative = got[3] != 0.0f;
            display.headroom = got[4];
            read = true;
        }
        if (values != nullptr) {
            env->DeleteLocalRef(values);
        }
    }
    failed(env, "displayHdr");
    env->DeleteLocalRef(type);
    return read;
}

void android_window_hdr(bool on, float headroom) {
    JNIEnv* env = nullptr;
    jclass type = activity_class(env);
    if (type == nullptr) {
        return;
    }
    const jmethodID method = env->GetStaticMethodID(type, "windowHdr", "(ZF)V");
    if (method != nullptr && !failed(env, "windowHdr")) {
        env->CallStaticVoidMethod(
            type, method, static_cast<jboolean>(on), static_cast<jfloat>(headroom)
        );
    }
    failed(env, "windowHdr");
    env->DeleteLocalRef(type);
}

}  // namespace openrac::platform
