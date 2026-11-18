// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Embed CPython only for the project's existing, signed extraction tools. The game remains native.
#import "ios/prepare.h"

#include <Python.h>
#include <atomic>

namespace {
void (^current_progress)(NSString*) = nil;
std::atomic_bool cancelled = false;

PyObject* report(PyObject*, PyObject* args) {
    if (cancelled) {
        PyErr_SetString(
            PyExc_InterruptedError, "Importation annulée. Votre copie précédente est conservée."
        );
        return nullptr;
    }
    const char* message = nullptr;
    if (!PyArg_ParseTuple(args, "s", &message)) {
        return nullptr;
    }
    if (current_progress) {
        current_progress([NSString stringWithUTF8String:message]);
    }
    Py_RETURN_NONE;
}

PyMethodDef report_method = {"progress", report, METH_VARARGS, "Show local extraction progress."};

bool initialize(NSError** error) {
    if (Py_IsInitialized()) {
        return true;
    }
    NSString* bundle = NSBundle.mainBundle.resourcePath;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = 0;
    config.write_bytecode = 0;
    config.buffered_stdio = 0;
    config.parse_argv = 0;
    config.module_search_paths_set = 1;
    wchar_t* home =
        Py_DecodeLocale([[bundle stringByAppendingPathComponent:@"python"] UTF8String], nullptr);
    PyStatus status = PyConfig_SetString(&config, &config.home, home);
    PyMem_RawFree(home);
    for (NSString* path in @[
             @"app_tools/editor",
             @"app_tools/tools",
             @"app_tools/port/ios",
             @"python/lib/python3.14",
             @"python/lib/python3.14/lib-dynload"
         ]) {
        wchar_t* location =
            Py_DecodeLocale([[bundle stringByAppendingPathComponent:path] UTF8String], nullptr);
        if (!PyStatus_Exception(status)) {
            status = PyWideStringList_Append(&config.module_search_paths, location);
        }
        PyMem_RawFree(location);
    }
    if (!PyStatus_Exception(status)) {
        status = Py_InitializeFromConfig(&config);
    }
    if (PyStatus_Exception(status)) {
        if (error) {
            *error = [NSError
                errorWithDomain:@"OpenRACImport"
                           code:1
                       userInfo:@{
                           NSLocalizedDescriptionKey: [NSString
                               stringWithUTF8String:status.err_msg
                                                        ?: "Could not start the disc importer"]
                       }];
        }
        PyConfig_Clear(&config);
        return false;
    }
    PyConfig_Clear(&config);
    PyEval_SaveThread();
    return true;
}
}  // namespace

void openrac_reset_import_cancel() {
    cancelled = false;
}

void openrac_cancel_import() {
    cancelled = true;
}

bool openrac_import_cancelled() {
    return cancelled;
}

bool openrac_prepare_disc(
    NSURL* data, NSDictionary* engine, void (^progress)(NSString*), NSError** error
) {
    if (!initialize(error)) {
        return false;
    }
    PyGILState_STATE state = PyGILState_Ensure();
    current_progress = [progress copy];
    PyObject* module = PyImport_ImportModule("import_disc");
    PyObject* function = module ? PyObject_GetAttrString(module, "prepare") : nullptr;
    PyObject* callback = PyCFunction_New(&report_method, nullptr);
    PyObject* path = PyUnicode_FromString(data.path.UTF8String);
    NSData* metadata = [NSJSONSerialization dataWithJSONObject:engine options:0 error:nil];
    NSString* metadataText = [[NSString alloc] initWithData:metadata encoding:NSUTF8StringEncoding];
    PyObject* description = PyUnicode_FromString(metadataText.UTF8String);
    PyObject* result =
        function ? PyObject_CallFunctionObjArgs(function, path, callback, description, nullptr)
                 : nullptr;
    const bool success = result != nullptr;
    if (!success) {
        PyObject* exception = PyErr_GetRaisedException();
        PyObject* text = exception ? PyObject_Str(exception) : nullptr;
        const char* message = text ? PyUnicode_AsUTF8(text) : nullptr;
        if (error) {
            *error = [NSError
                errorWithDomain:@"OpenRACImport"
                           code:2
                       userInfo:@{
                           NSLocalizedDescriptionKey: [NSString
                               stringWithUTF8String:message ?: "L’importation du disque a échoué."]
                       }];
        }
        Py_XDECREF(text);
        Py_XDECREF(exception);
    }
    Py_XDECREF(result);
    Py_XDECREF(description);
    Py_XDECREF(path);
    Py_XDECREF(callback);
    Py_XDECREF(function);
    Py_XDECREF(module);
    current_progress = nil;
    PyGILState_Release(state);
    return success;
}
