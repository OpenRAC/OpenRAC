// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// A shared library opened at run time (mod_library.h).

#include "mod_library.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#include <string>

#include <sys/stat.h>
#endif

namespace openrac::mods::library {

#if defined(_WIN32)

void* open(const std::filesystem::path& path, std::string& error) {
    // The mod's own folder is searched for the libraries it needs, not the program's working one.
    HMODULE module = LoadLibraryExW(path.wstring().c_str(), nullptr,
                                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (module == nullptr) {
        const DWORD code = GetLastError();
        char text[256] = {};
        if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, text,
                           sizeof(text), nullptr) == 0) {
            error = "Windows error " + std::to_string(code);
        } else {
            error = text;
            while (!error.empty() && (error.back() == '\n' || error.back() == '\r' || error.back() == '.')) {
                error.pop_back();
            }
        }
        return nullptr;
    }
    return module;
}

void* symbol(void* library, const char* name) {
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
}

void close(void* library) {
    FreeLibrary(static_cast<HMODULE>(library));
}

const void* base_of(const void* address) {
    HMODULE module = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCSTR>(address), &module) != 0) {
        return module;
    }
    return nullptr;
}

#else

#if !defined(__ANDROID__)
// Why a library any other user of this computer could swap is not loaded: the file writable by
// all, or a folder above it that all may write and rename in (a sticky one, as /tmp, is safe).
std::string writable_by_others(const std::filesystem::path& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) == 0 && (st.st_mode & S_IWOTH) != 0) {
        return "its library can be changed by any user of this computer (chmod o-w " + path.string()
               + ")";
    }
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::absolute(path, ec).parent_path();
    for (; !dir.empty(); dir = dir.parent_path()) {
        if (stat(dir.c_str(), &st) == 0 && (st.st_mode & S_IWOTH) != 0
            && (st.st_mode & S_ISVTX) == 0) {
            return "its folder " + dir.string()
                   + " can be changed by any user of this computer (chmod o-w)";
        }
        if (dir == dir.root_path()) {
            break;
        }
    }
    return {};
}
#endif

void* open(const std::filesystem::path& path, std::string& error) {
#if !defined(__ANDROID__)
    if (std::string why = writable_by_others(path); !why.empty()) {
        error = why;
        return nullptr;
    }
#endif
#if defined(__ANDROID__)
    // Android 14 loads code from the app's own files only when they are read-only. The launcher
    // replaces a mod's folder whole when it updates it, so the library itself never changes.
    chmod(path.c_str(), 0500);
#endif
    void* library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        const char* why = dlerror();
        error = why != nullptr ? why : "cannot be opened";
    }
    return library;
}

void* symbol(void* library, const char* name) {
    return dlsym(library, name);
}

void close(void* library) {
    dlclose(library);
}

const void* base_of(const void* address) {
    Dl_info info{};
    return dladdr(address, &info) != 0 ? info.dli_fbase : nullptr;
}

#endif

}  // namespace openrac::mods::library
