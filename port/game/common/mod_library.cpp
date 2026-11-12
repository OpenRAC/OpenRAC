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
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <unistd.h>

#include <sys/stat.h>
#if defined(__APPLE__)
#include <sys/mount.h>
#include <sys/param.h>
#elif !defined(__ANDROID__)
#include <sys/vfs.h>
#endif
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
// Whether a folder's disk keeps owners and modes at all: FAT, exFAT, NTFS, network and FUSE disks
// show every file as writable by all, and refusing there would refuse every mod on them.
bool keeps_modes(const std::filesystem::path& dir) {
    struct statfs fs{};
    if (statfs(dir.c_str(), &fs) != 0) {
        return true;
    }
#if defined(__APPLE__)
    return (fs.f_flags & MNT_IGNORE_OWNERSHIP) == 0 && std::strcmp(fs.f_fstypename, "msdos") != 0
           && std::strcmp(fs.f_fstypename, "exfat") != 0
           && std::strcmp(fs.f_fstypename, "ntfs") != 0
           && std::strcmp(fs.f_fstypename, "smbfs") != 0;
#else
    switch (static_cast<unsigned long>(fs.f_type)) {
        case 0x4d44UL:      // FAT
        case 0x2011BAB0UL:  // exFAT
        case 0x5346544eUL:  // NTFS (ntfs3)
        case 0x65735546UL:  // FUSE (ntfs-3g and others)
        case 0x01021997UL:  // 9p (WSL's Windows drives)
        case 0xFE534D42UL:  // SMB2
        case 0xFF534D42UL:  // CIFS
            return false;
        default:
            return true;
    }
#endif
}

// Why a library another user of this computer could swap is not loaded: the file, or a folder
// above it, writable by all (a sticky folder, as /tmp, only keeps others from renaming what they
// do not own) or owned by another user than this one and root. Links are followed to the file.
std::string writable_by_others(const std::filesystem::path& path) {
    std::error_code ec;
    const std::filesystem::path file = std::filesystem::canonical(path, ec);
    if (ec || !keeps_modes(file.parent_path())) {
        return {};
    }
    const uid_t me = geteuid();
    struct stat st{};
    if (stat(file.c_str(), &st) == 0) {
        if ((st.st_mode & S_IWOTH) != 0) {
            return "its library can be changed by any user of this computer (chmod o-w "
                   + file.string() + ")";
        }
        if (st.st_uid != me && st.st_uid != 0) {
            return "its library " + file.string() + " belongs to another user of this computer";
        }
    }
    for (std::filesystem::path dir = file.parent_path(); !dir.empty(); dir = dir.parent_path()) {
        if (stat(dir.c_str(), &st) == 0) {
            if ((st.st_mode & S_IWOTH) != 0 && (st.st_mode & S_ISVTX) == 0) {
                return "its folder " + dir.string()
                       + " can be changed by any user of this computer (chmod o-w)";
            }
            if (st.st_uid != me && st.st_uid != 0) {
                return "its folder " + dir.string() + " belongs to another user of this computer";
            }
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
