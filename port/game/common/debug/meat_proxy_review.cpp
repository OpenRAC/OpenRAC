// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include <algorithm>

#include "common/log.h"
#include "debug/overlay.h"
#include "debug/review_queue.h"
#include "openrac/game_host.h"

namespace openrac::frontend::debug_overlay {
namespace {

struct Review {
    std::vector<ReviewTask> tasks;
    std::string selected;
    std::string message;
    std::optional<ReviewTask> destination;
    std::uint64_t deadline = 0;
    bool travel_started = false;
    bool loaded = false;
};

Review review;

void finish_view(const ReviewTask& task) {
    place_review_camera(task.level, task.position, task.forward);
    log::info("meat proxy review: placed freecam for {} in level {}", task.id, task.level);
    review.message = "Freecam placed.";
}

int resize_feedback(ImGuiInputTextCallbackData* data) {
    auto& feedback = *static_cast<std::string*>(data->UserData);
    feedback.resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = feedback.data();
    return 0;
}

}  // namespace

bool meat_proxy_review_skips_landing() {
    const auto* play = openrac_game_play_host;
    return review.destination && play && play->landing && play->landing(review.destination->level);
}

bool meat_proxy_review_skips_flight() {
    return review.destination && review.travel_started;
}

void meat_proxy_review_update() {
    if (!review.destination) {
        return;
    }
    const auto& task = *review.destination;
    const auto* play = openrac_game_play_host;
    std::int32_t level = -1;
    const int phase = play->phase(&level);
    if (level == task.level && phase == OPENRAC_GAME_PHASE_PLAYING) {
        finish_view(task);
        review.destination.reset();
        return;
    }
    if (SDL_GetTicks() >= review.deadline) {
        review.message = "Timed out waiting for the level to be playable.";
        review.destination.reset();
        return;
    }
    // Freecam suppresses the game's pad input, including the landing skip. Re-enable it
    // at the saved pose once the destination is playable.
    s.camera_tools.enabled = false;
    if (review.travel_started || phase == OPENRAC_GAME_PHASE_LOADING
        || phase == OPENRAC_GAME_PHASE_NONE
        || (level == task.level && phase == OPENRAC_GAME_PHASE_SCENE)) {
        return;
    }
    const bool started = phase == OPENRAC_GAME_PHASE_FRONT_END
                             ? play->start(task.level, nullptr, 0) != 0
                             : play->travel(task.level) != 0;
    if (started) {
        review.travel_started = true;
    } else {
        review.message = "The game refused to open this level.";
        review.destination.reset();
    }
}

namespace {
auto draw_task_list(int level_count) {
    if (!review.loaded || ImGui::Button("Reload JSON")) {
        review.loaded = true;
        if (load_review_tasks(level_count, review.tasks, review.message)) {
            review.message.clear();
        }
    }
    const auto path = review_queue_path().u8string();
    ImGui::TextWrapped("%s", reinterpret_cast<const char*>(path.c_str()));
    if (!review.message.empty()) {
        ImGui::TextWrapped("%s", review.message.c_str());
    }
    auto selected =
        std::find_if(review.tasks.begin(), review.tasks.end(), [](const ReviewTask& task) {
            return task.id == review.selected;
        });
    if (selected == review.tasks.end()) {
        if (review.tasks.empty()) {
            if (review.message.empty()) {
                ImGui::TextUnformatted("No review tasks.");
            }
            return selected;
        }
        selected = review.tasks.begin();
        review.selected = selected->id;
    }
    if (ImGui::BeginCombo("Task", selected->title.c_str())) {
        const bool opening = ImGui::IsWindowAppearing();
        for (auto item = review.tasks.begin(); item != review.tasks.end(); ++item) {
            const auto& task = *item;
            ImGui::PushID(task.id.c_str());
            const std::string label =
                task.title + " [" + (task.verdict.empty() ? "Pending" : task.verdict) + "]";
            const bool is_selected = task.id == review.selected;
            if (ImGui::Selectable(label.c_str(), is_selected)) {
                selected = item;
                review.selected = task.id;
            }
            if (opening && is_selected) {
                ImGui::SetScrollHereY();
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return selected;
}

void draw_camera_controls(const Shown& shown, const openrac_game_play& play, ReviewTask& task) {
    ImGui::BeginDisabled(review.destination.has_value());
    if (ImGui::Button("Open view")) {
        review.destination = task;
        review.deadline = SDL_GetTicks() + 60000;
        review.travel_started = false;
        review.message = "Opening level " + std::to_string(task.level) + "...";
        s.paused = false;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    std::int32_t playing_level = -1;
    const bool can_save_camera =
        !review.destination && shown.camera != nullptr && shown.camera_level == task.level
        && play.phase(&playing_level) == OPENRAC_GAME_PHASE_PLAYING && playing_level == task.level;
    ImGui::BeginDisabled(!can_save_camera);
    if (ImGui::Button("Save current camera")) {
        save_review_camera(
            task, shown.camera->position, camera_forward(*shown.camera), review.message
        );
    }
    ImGui::EndDisabled();
    if (!can_save_camera && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Open the task's level before saving its camera.");
    }
}

void draw_feedback(ReviewTask& task) {
    ImGui::TextUnformatted("Feedback");
    ImGui::InputTextMultiline(
        "##feedback",
        task.feedback.data(),
        task.feedback.capacity() + 1,
        ImVec2(-1, dp(65.0f)),
        ImGuiInputTextFlags_CallbackResize,
        resize_feedback,
        &task.feedback
    );
    if (ImGui::Button("Yay")) {
        save_review(task, "yay", review.message);
    }
    ImGui::SameLine();
    if (ImGui::Button("Nay")) {
        save_review(task, "nay", review.message);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save feedback")) {
        save_review(task, task.verdict, review.message);
    }
}

bool draw_delete_task(const ReviewTask& task) {
    ImGui::BeginDisabled(review.destination.has_value());
    if (ImGui::Button("Delete task")) {
        ImGui::OpenPopup("Delete review task?");
    }
    ImGui::EndDisabled();
    bool deleted = false;
    if (ImGui::BeginPopupModal("Delete review task?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Delete this task and its saved feedback?");
        ImGui::TextUnformatted(task.title.c_str());
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            deleted = delete_review_task(task, review.message);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return deleted;
}

void draw_review(const Shown& shown) {
    const auto* play = openrac_game_play_host;
    if (!play || !play->phase || !play->start || !play->travel) {
        ImGui::TextUnformatted("Level travel is unavailable in this game.");
        return;
    }
    const auto selected = draw_task_list(play->levels);
    if (selected == review.tasks.end()) {
        return;
    }
    auto& task = *selected;
    ImGui::PushID(task.id.c_str());
    ImGui::Text(
        "Level %d | %s", task.level, task.verdict.empty() ? "Pending" : task.verdict.c_str()
    );
    ImGui::TextWrapped("%s", task.description.c_str());
    draw_camera_controls(shown, *play, task);
    draw_feedback(task);
    ImGui::SameLine();
    const bool deleted = draw_delete_task(task);
    ImGui::PopID();
    if (deleted) {
        const auto next = review.tasks.erase(selected);
        if (next != review.tasks.end()) {
            review.selected = next->id;
        } else {
            review.selected = review.tasks.empty() ? "" : review.tasks.back().id;
        }
        review.message = "Task deleted.";
    }
}
}  // namespace

void meat_proxy_review_window(const Shown& shown) {
    if (!s.meat_proxy_review) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(dp(410), dp(360)), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Meat Proxy Review", &s.meat_proxy_review)) {
        draw_review(shown);
    }
    ImGui::End();
}

}  // namespace openrac::frontend::debug_overlay
