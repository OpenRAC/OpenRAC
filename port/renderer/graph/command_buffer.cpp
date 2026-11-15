// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Builds and validates backend-independent commands against pass declarations.
#include "renderer/graph/command_buffer.h"

#include <algorithm>
#include <type_traits>

namespace openrac::renderer::graph {
namespace {
constexpr State kBufferRead{Stage::Transfer, Access::Read, Layout::Buffer};
constexpr State kImageRead{Stage::Transfer, Access::Read, Layout::TransferSource};
constexpr State kImageWrite{Stage::Transfer, Access::Write, Layout::TransferDestination};

ImageRange transfer_range(const Command& command) {
    const auto* transfer = std::get_if<FullImageTransfer>(&command);
    return transfer ? ImageRange{transfer->aspect} : ImageRange{};
}

State transfer_state(const Command& command, Access access) {
    const auto* transfer = std::get_if<FullImageTransfer>(&command);
    if (transfer && transfer->kind == ImageTransferKind::ResolveDepth) {
        return {Stage::DepthTests | Stage::ColourOutput, access, Layout::DepthAttachment};
    }
    return access == Access::Read ? kImageRead : kImageWrite;
}

// Rejects empty, overflowing and overlapping native transfer ranges.
bool valid_transfer(const Command& command) {
    return std::visit(
        [](const auto& op) {
            using T = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<T, BufferCopy>) {
                if (!op.bytes.size || op.bytes.offset > UINT64_MAX - op.bytes.size
                    || op.destination_offset > UINT64_MAX - op.bytes.size) {
                    return false;
                }
                return op.source != op.destination
                       || op.bytes.offset + op.bytes.size <= op.destination_offset
                       || op.destination_offset + op.bytes.size <= op.bytes.offset;
            } else if constexpr (std::is_same_v<T, BufferToImageCopy>) {
                return op.bytes.size != 0;
            } else {
                return op.source != op.destination;
            }
        },
        command
    );
}

// Matches image reads and writes to commands and rejects multiple writes to one image.
Result<void> validate_image_commands(const Pass& required, const Pass& pass) {
    const auto image_matches = [](const ImageUse& a, const ImageUse& b) {
        return a.image == b.image && a.range == b.range && a.state == b.state;
    };
    for (std::size_t index = 0; index < required.images.size(); ++index) {
        const auto& use = required.images[index];
        if (!std::ranges::any_of(pass.images, [&](const ImageUse& declared) {
                return image_matches(use, declared);
            })) {
            return unexpected(Error{pass.name + ": command lacks a matching transfer declaration"});
        }
        if (use.state.access != Access::Read
            && std::ranges::any_of(
                std::span(required.images).first(index), [&](const ImageUse& previous) {
                    return previous.state.access != Access::Read && previous.image == use.image;
                }
            )) {
            return unexpected(Error{pass.name + ": multiple commands write the same image"});
        }
    }
    for (const auto& use : pass.images) {
        if (use.state.access != Access::Read
            && !std::ranges::any_of(required.images, [&](const ImageUse& actual) {
                   return image_matches(use, actual);
               })) {
            return unexpected(Error{pass.name + ": image write has no matching command"});
        }
    }
    return {};
}

// Matches buffer accesses to commands without permitting overlapping output bytes.
Result<void> validate_buffer_commands(const Pass& required, const Pass& pass) {
    const auto buffer_matches = [](const BufferUse& a, const BufferUse& b) {
        return a.buffer == b.buffer && a.range == b.range && a.state == b.state;
    };
    for (std::size_t index = 0; index < required.buffers.size(); ++index) {
        const auto& use = required.buffers[index];
        if (!std::ranges::any_of(pass.buffers, [&](const BufferUse& declared) {
                return buffer_matches(use, declared);
            })) {
            return unexpected(Error{pass.name + ": command lacks a matching transfer declaration"});
        }
        if (use.state.access != Access::Read
            && std::ranges::any_of(
                std::span(required.buffers).first(index), [&](const BufferUse& previous) {
                    return previous.state.access != Access::Read && previous.buffer == use.buffer
                           && previous.range.offset < use.range.offset + use.range.size
                           && use.range.offset < previous.range.offset + previous.range.size;
                }
            )) {
            return unexpected(
                Error{pass.name + ": multiple commands write overlapping buffer bytes"}
            );
        }
    }
    for (const auto& use : pass.buffers) {
        if (use.state.access != Access::Read
            && !std::ranges::any_of(required.buffers, [&](const BufferUse& actual) {
                   return buffer_matches(use, actual);
               })) {
            return unexpected(Error{pass.name + ": buffer write has no matching command"});
        }
    }
    return {};
}

void append_declarations(Pass& pass, std::span<const Command> commands) {
    for (const auto& command : commands) {
        if (const auto* copy = std::get_if<BufferCopy>(&command)) {
            pass.buffers.push_back({copy->source, copy->bytes, kBufferRead});
            pass.buffers.push_back(
                {copy->destination,
                 {copy->destination_offset, copy->bytes.size},
                 {Stage::Transfer, Access::Write, Layout::Buffer}}
            );
        } else if (const auto* upload = std::get_if<BufferToImageCopy>(&command)) {
            pass.buffers.push_back({upload->source, upload->bytes, kBufferRead});
            pass.images.push_back({upload->destination, {}, kImageWrite});
        } else {
            const auto append = [&](const auto& op) {
                const auto range = transfer_range(command);
                pass.images.push_back({op.source, range, transfer_state(command, Access::Read)});
                pass.images.push_back(
                    {op.destination, range, transfer_state(command, Access::Write)}
                );
            };
            if (const auto* blit = std::get_if<ImageBlit>(&command)) {
                append(*blit);
            } else {
                append(std::get<FullImageTransfer>(command));
            }
        }
    }
}
}  // namespace

std::span<const Command> CommandBuffer::commands() const {
    if (const auto* single = std::get_if<Command>(&m_commands)) {
        return {single, 1};
    }
    if (const auto* multiple = std::get_if<std::vector<Command>>(&m_commands)) {
        return *multiple;
    }
    return {};
}

void CommandBuffer::append(Command command) {
    if (auto* multiple = std::get_if<std::vector<Command>>(&m_commands)) {
        multiple->push_back(std::move(command));
    } else if (const auto* single = std::get_if<Command>(&m_commands)) {
        std::vector<Command> storage;
        storage.reserve(2);
        storage.push_back(*single);
        storage.push_back(std::move(command));
        m_commands = std::move(storage);
    } else {
        m_commands = std::move(command);
    }
}

void CommandBuffer::copy_buffer_to_image(
    BufferHandle source, BufferRange bytes, ImageHandle destination
) {
    append(BufferToImageCopy{source, bytes, destination});
}

void CommandBuffer::blit(ImageHandle source, ImageHandle destination, BlitRegion region) {
    append(ImageBlit{source, destination, region});
}

void CommandBuffer::transfer_image(
    ImageHandle source, ImageHandle destination, Aspect aspect, ImageTransferKind kind
) {
    append(FullImageTransfer{source, destination, aspect, kind});
}

Pass CommandBuffer::declarations(std::string name) const {
    Pass pass{std::move(name)};
    append_declarations(pass, commands());
    return pass;
}

void CommandBuffer::copy_buffer(
    BufferHandle source,
    BufferRange bytes,
    BufferHandle destination,
    std::uint64_t destination_offset
) {
    append(BufferCopy{source, bytes, destination, destination_offset});
}

Result<void> CommandBuffer::validate(const Pass& pass) const {
    const auto ops = commands();
    if (pass.kind != PassKind::Work && !ops.empty()) {
        return unexpected(Error{pass.name + ": export passes cannot record transfer commands"});
    }
    for (const auto& command : ops) {
        if (!valid_transfer(command)) {
            return unexpected(Error{pass.name + ": invalid or overlapping transfer command"});
        }
    }
    thread_local Pass required;
    required.images.clear();
    required.buffers.clear();
    append_declarations(required, ops);
    const auto images = validate_image_commands(required, pass);
    if (!images) {
        return unexpected(images.error());
    }
    return validate_buffer_commands(required, pass);
}

Result<CommandBuffer> CommandBuffer::remap(const ResourceRemap& resources) const {
    CommandBuffer remapped;
    for (const auto& command : commands()) {
        const auto appended = std::visit(
            [&](auto op) {
                if (!resources.rebind(op.source) || !resources.rebind(op.destination)) {
                    return false;
                }
                remapped.append(std::move(op));
                return true;
            },
            command
        );
        if (!appended) {
            return unexpected(Error{"transfer command refers to an invalid graph resource"});
        }
    }
    return remapped;
}

Result<void> validate_pipeline_command(const Pass& pass, const PipelineCommand& command) {
    const bool compute = std::holds_alternative<Dispatch>(command);
    const Stage stages = compute
                             ? Stage::ComputeShader
                             : Stage::VertexInput | Stage::VertexShader | Stage::FragmentShader
                                   | Stage::ColourOutput | Stage::DepthTests | Stage::DrawIndirect;
    const auto has_stage = [stages](const auto& use) {
        return (static_cast<unsigned>(use.state.stages) & static_cast<unsigned>(stages)) != 0;
    };
    if (pass.kind != PassKind::Work
        || ((static_cast<unsigned>(pass.execution) & static_cast<unsigned>(stages)) == 0
            && !std::ranges::any_of(pass.images, has_stage)
            && !std::ranges::any_of(pass.buffers, has_stage))) {
        return unexpected(Error{pass.name + ": pipeline command does not match the graph pass"});
    }
    return {};
}

}  // namespace openrac::renderer::graph
