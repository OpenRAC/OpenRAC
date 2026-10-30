// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Owned native shader bytecode and the SDK's synchronous shader-lookup bridge.
#pragma once
#include <map>
#include <memory>
#include <string>

#include <FidelityFX/host/ffx_types.h>

namespace openrac::enhanced::fsr3_detail {
// Shader permutations owned by one upscaler, independent of its GPU resource context.
class ShaderLibrary {
public:
    // Assembles embedded AMD sources into the owned include lookup.
    ShaderLibrary();
    // Releases cached bytecode and sources; this owner holds no GPU objects.
    ~ShaderLibrary();
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    // Compiles before SDK creation, so a missing shader never reaches Vulkan pipeline creation.
    [[nodiscard]] bool prepare(std::string& error, bool allow_float16 = false);
    // Copies a stable shader view; false for a permutation outside the prepared contract.
    [[nodiscard]] bool find(unsigned pass, unsigned permutation, FfxShaderBlob& out) const;

    [[nodiscard]] bool float16_selected() const { return m_selected_float16; }

    // Makes this owner available only during synchronous SDK context creation on the render thread.
    void activate();
    // Ends the SDK callback scope without releasing cached bytecode.
    void deactivate();

private:
    // Finds original AMD source text, owned until this library is destroyed.
    std::string_view source(std::string_view name) const;
    mutable bool m_selected_float16 =
        false;                     // Observed SDK pipeline selection, not just preparation.
    bool m_allow_float16 = false;  // Only accumulation history filtering has a complete half path.
    struct Shader;                 // Bytecode and stable SDK reflection arrays.
    std::map<std::string, std::string, std::less<>> m_sources;  // Assembled bounded source chunks.
    std::map<std::pair<unsigned, unsigned>, std::unique_ptr<Shader>>
        m_shaders;  // Owned compiled permutations.
};
}  // namespace openrac::enhanced::fsr3_detail
