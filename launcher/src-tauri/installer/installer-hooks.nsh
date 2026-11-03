; The NSIS installer's look: Modern UI's default light pages, with the header
; picture on the right. Tauri includes this file before any page and nothing
; here defines a hook macro (NSIS_HOOK_*), so the install steps, paths,
; registry keys and shortcuts stay exactly Tauri's.

!define MUI_HEADERIMAGE_RIGHT
