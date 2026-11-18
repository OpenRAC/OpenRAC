// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Review tasks and their JSON persistence. The window thread owns the loaded tasks.
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "renderer/math.h"

namespace openrac::frontend::debug_overlay {

struct ReviewTask {
    std::string id, title, description;
    int level = -1;
    renderer::Vec3 position{}, forward{};
    std::string verdict, feedback, saved_feedback;
};

// Uses the configured path or the system temporary directory.
const std::filesystem::path& review_queue_path();

// Replaces the task list only after validating the entire document.
bool load_review_tasks(int level_count, std::vector<ReviewTask>& tasks, std::string& message);

// Edits read the current document so other tasks and their metadata survive.
bool save_review(ReviewTask& task, std::string_view verdict, std::string& message);
bool save_review_camera(
    ReviewTask& task, renderer::Vec3 position, renderer::Vec3 forward, std::string& message
);
bool delete_review_task(const ReviewTask& task, std::string& message);

}  // namespace openrac::frontend::debug_overlay
