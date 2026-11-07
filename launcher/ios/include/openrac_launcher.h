// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void openrac_ios_launcher_init(const char* root, const char* documents, const char* bundle_json);
char* openrac_ios_launcher_command(const char* name, const char* arguments_json);
void openrac_ios_launcher_free(char* reply);
#ifdef __cplusplus
}
#endif
