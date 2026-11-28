// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// What the desktop is given to open a folder (platform/show_folder.h): any name, any letters.
#include "platform/show_folder.h"
#include "tests/check.h"

namespace {

using openrac::platform::folder_target;

void test_url() {
    CHECK(
        folder_target("/home/you/.local/share/openrac/screenshots", false)
        == "file:///home/you/.local/share/openrac/screenshots"
    );
    // Spaces, the URL's own marks and letters outside ASCII (UTF-8, byte by byte).
    CHECK(
        folder_target("/Users/Mateusz Kłysz/a#b?c%d", false)
        == "file:///Users/Mateusz%20K%C5%82ysz/a%23b%3Fc%25d"
    );
    CHECK(
        folder_target("/home/ユーザー", false)
        == "file:///home/%E3%83%A6%E3%83%BC%E3%82%B6%E3%83%BC"
    );
    // A Windows path through a URL: one more slash before the drive.
    CHECK(folder_target("C:/Users/x", false) == "file:///C%3A/Users/x");
}

void test_windows() {
    CHECK(
        folder_target("C:/Users/Mateusz Kłysz/AppData/Local/openrac", true)
        == "C:\\Users\\Mateusz Kłysz\\AppData\\Local\\openrac"
    );
    CHECK(folder_target("//server/share/openrac", true) == "\\\\server\\share\\openrac");
}

}  // namespace

int main() {
    test_url();
    test_windows();
    return openrac::test::result();
}
