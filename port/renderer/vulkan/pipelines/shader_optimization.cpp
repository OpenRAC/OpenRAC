// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Performance optimization shared by translated graphics, compute and native SDK shaders.

#include <format>
#include <utility>

#include "renderer/vulkan/pipelines/shader_compiler_internal.h"

#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/optimizer.hpp>

namespace openrac::vkgl::compiler_detail {
namespace {

// Keeps offline variant costs bounded by excluding scalar expansion and loop unrolling.
void register_performance_passes(spvtools::Optimizer& optimizer) {
    // Avoid unlimited scalar replacement and loop expansion across thousands of build variants.
    optimizer.RegisterPass(spvtools::CreateWrapOpKillPass())
        .RegisterPass(spvtools::CreateDeadBranchElimPass())
        .RegisterPass(spvtools::CreateMergeReturnPass())
        .RegisterPass(spvtools::CreateInlineExhaustivePass())
        .RegisterPass(spvtools::CreateEliminateDeadFunctionsPass())
        .RegisterPass(spvtools::CreatePrivateToLocalPass())
        .RegisterPass(spvtools::CreateLocalSingleBlockLoadStoreElimPass())
        .RegisterPass(spvtools::CreateLocalSingleStoreElimPass())
        .RegisterPass(spvtools::CreateLocalMultiStoreElimPass())
        .RegisterPass(spvtools::CreateCCPPass())
        .RegisterPass(spvtools::CreateSimplificationPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass(true))
        .RegisterPass(spvtools::CreateVectorDCEPass())
        .RegisterPass(spvtools::CreateDeadInsertElimPass())
        .RegisterPass(spvtools::CreateBlockMergePass())
        .RegisterPass(spvtools::CreateCFGCleanupPass())
        .RegisterPass(spvtools::CreateRedundancyEliminationPass())
        .RegisterPass(spvtools::CreateAggressiveDCEPass(true))
        .RegisterPass(spvtools::CreateCompactIdsPass());
}

// Runs performance passes without changing the renderer's resource or specialization interfaces.
bool optimize_spirv(std::vector<std::uint32_t>& code, std::string& log) {
    // The compiler emits SPIR-V 1.5 only for Vulkan 1.2 viewport outputs.
    constexpr std::uint32_t kSpirv15 = 0x00010500;

    // A missing module cannot be optimized or published as a valid shader.
    if (code.size() < 2) {
        log += "shader compilation produced no SPIR-V";
        return false;
    }

    const auto environment = code[1] >= kSpirv15 ? SPV_ENV_VULKAN_1_2 : SPV_ENV_VULKAN_1_1;
    spvtools::Optimizer optimizer(environment);
    // The synchronous optimizer reports validation and pass failures into the caller's diagnostic.
    const auto diagnostic =
        [&log](
            spv_message_level_t, const char*, const spv_position_t& position, const char* message
        ) {
            log += std::format("SPIR-V word {}: {}\n", position.index, message);
        };
    optimizer.SetMessageConsumer(diagnostic);
    register_performance_passes(optimizer);
    spvtools::OptimizerOptions options;
    options.set_preserve_bindings(true);
    options.set_preserve_spec_constants(true);
    std::vector<std::uint32_t> optimized;

    // Failed transformations must never replace the executable module.
    if (!optimizer.Run(code.data(), code.size(), &optimized, options)) {
        return false;
    }

    spvtools::SpirvTools validator(environment);
    validator.SetMessageConsumer(diagnostic);

    // Validate the transformed module as well as the optimizer's original input.
    if (!validator.Validate(optimized)) {
        return false;
    }

    code = std::move(optimized);
    return true;
}
}  // namespace

bool compile_spirv_for_analysis(
    const glslang::TIntermediate& intermediate, std::vector<std::uint32_t>& code, std::string& log
) {
    glslang::SpvOptions options;
    options.generateDebugInfo = false;
    // glslang's optional optimizer selects size passes; performance passes run explicitly below.
    options.disableOptimizer = true;
    spv::SpvBuildLogger logger;
    glslang::GlslangToSpv(intermediate, code, &logger, &options);
    log += logger.getAllMessages();
    return !code.empty();
}

bool compile_spirv(
    const glslang::TIntermediate& intermediate, std::vector<std::uint32_t>& code, std::string& log
) {
    return compile_spirv_for_analysis(intermediate, code, log) && optimize_spirv(code, log);
}

}  // namespace openrac::vkgl::compiler_detail
