// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Reads and updates the review document independently of the window and travel state.
#include "debug/review_queue.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

#include "common/env.h"
#include "common/json.h"
#include "common/staged_file.h"
#include "openrac/game_host.h"

namespace openrac::frontend::debug_overlay {

const std::filesystem::path& review_queue_path() {
    static const auto path = [] {
        const auto configured = env::value(env::Var::MeatProxyReview);
        if (configured && !configured->empty()) {
            return std::filesystem::path(std::u8string(configured->begin(), configured->end()));
        }
        std::error_code error;
        const auto temp = std::filesystem::temp_directory_path(error);
        return error ? std::filesystem::path{} : temp / "meat-proxy-review.json";
    }();
    return path;
}

namespace {

bool read_document(json::Value& doc, std::string& error) {
    if (review_queue_path().empty()) {
        error = "Cannot locate the system temp folder.";
        return false;
    }
    std::ifstream file(review_queue_path(), std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    constexpr std::streamoff kMaxQueueBytes = 8LL * 1024 * 1024;
    if (size < 0 || size > kMaxQueueBytes) {
        error = "Cannot open the review JSON or it exceeds 8 MiB.";
        return false;
    }
    std::string contents(static_cast<std::size_t>(size), '\0');
    file.seekg(0);
    if (!file.read(contents.data(), size)) {
        error = "Cannot read the review JSON.";
        return false;
    }
    std::string_view text(contents);
    if (text.starts_with("\xef\xbb\xbf")) {
        text.remove_prefix(3);
    }
    if (!json::parse(text, doc, error, json::DuplicateKeys::Reject)) {
        return false;
    }
    if (!doc["tasks"].is_array() || (!doc["reviews"].is_null() && !doc["reviews"].is_object())) {
        error = "Expected a tasks array and an optional reviews object.";
        return false;
    }
    if (!doc["game"].is_null() && doc["game"].string() != openrac_game.id) {
        error = "This review queue belongs to a different game.";
        return false;
    }
    return true;
}

bool read_vector(const json::Value& value, renderer::Vec3& out) {
    if (value.items().size() != 3) {
        return false;
    }
    for (std::size_t i = 0; i < 3; ++i) {
        const double component = value[i].number();
        if (!value[i].is_number() || !std::isfinite(component)
            || std::abs(component) > std::numeric_limits<float>::max()) {
            return false;
        }
        out[i] = static_cast<float>(component);
    }
    return true;
}

}  // namespace

bool load_review_tasks(int level_count, std::vector<ReviewTask>& result, std::string& message) {
    json::Value doc;
    if (!read_document(doc, message)) {
        return false;
    }
    std::vector<ReviewTask> tasks;
    std::set<std::string> ids;
    for (const auto& item : doc["tasks"].items()) {
        ReviewTask task{};
        task.id = item["id"].string();
        task.title = item["title"].string();
        task.description = item["description"].string();
        const double level = item["level"].number(-1);
        if (task.id.empty() || task.title.empty() || !ids.insert(task.id).second
            || !std::isfinite(level) || level < 0 || level >= level_count
            || level != std::floor(level) || !read_vector(item["camera"]["position"], task.position)
            || !read_vector(item["camera"]["forward"], task.forward)
            || task.forward == renderer::Vec3{}) {
            message = "Invalid or duplicate task at row " + std::to_string(tasks.size() + 1);
            return false;
        }
        task.level = static_cast<int>(level);
        const auto& saved = doc["reviews"][task.id];
        task.verdict = saved["verdict"].string();
        task.feedback = task.saved_feedback = saved["feedback"].string();
        tasks.push_back(std::move(task));
    }
    result = std::move(tasks);
    return true;
}

namespace {

void quote(std::ostream& out, std::string_view value) {
    out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') {
            out << '\\' << static_cast<char>(c);
        } else if (c < 0x20) {
            out << std::format("\\u{:04x}", c);
        } else {
            out << static_cast<char>(c);
        }
    }
    out << '"';
}

void write_json(std::ostream& out, const json::Value& value, std::size_t depth);

bool write_members(
    std::ostream& out,
    const json::Value& object,
    std::size_t depth,
    std::string_view skip = {},
    std::string_view also_skip = {}
) {
    bool wrote_member = false;
    for (const auto& [key, value] : object.members()) {
        if ((!skip.empty() && key == skip) || (!also_skip.empty() && key == also_skip)) {
            continue;
        }
        if (wrote_member) {
            out << ',';
        }
        wrote_member = true;
        out << '\n' << std::string(depth * 2, ' ');
        quote(out, key);
        out << ": ";
        write_json(out, value, depth);
    }
    return wrote_member;
}

void write_array(std::ostream& out, const json::Value& value, std::size_t depth) {
    const auto& items = value.items();
    out << '[';
    for (const auto& item : items) {
        if (&item != &items.front()) {
            out << ',';
        }
        out << '\n' << std::string((depth + 1) * 2, ' ');
        write_json(out, item, depth + 1);
    }
    if (!items.empty()) {
        out << '\n' << std::string(depth * 2, ' ');
    }
    out << ']';
}

void write_json(std::ostream& out, const json::Value& value, std::size_t depth) {
    switch (value.type()) {
        case json::Value::Type::Null:
            out << "null";
            break;
        case json::Value::Type::Bool:
            out << (value.boolean() ? "true" : "false");
            break;
        case json::Value::Type::Number:
            out << std::format("{}", value.number());
            break;
        case json::Value::Type::String:
            quote(out, value.string());
            break;
        case json::Value::Type::Array:
            write_array(out, value, depth);
            break;
        case json::Value::Type::Object: {
            out << '{';
            if (write_members(out, value, depth + 1)) {
                out << '\n' << std::string(depth * 2, ' ');
            }
            out << '}';
            break;
        }
    }
}

const json::Value* read_for_edit(const ReviewTask& task, json::Value& doc, std::string& message) {
    if (!read_document(doc, message)) {
        return nullptr;
    }
    const json::Value* selected = nullptr;
    for (const auto& item : doc["tasks"].items()) {
        if (item["id"].string() != task.id) {
            continue;
        }
        if (selected) {
            message = "Duplicate task ID on disk. Fix the queue and reload it.";
            return nullptr;
        }
        selected = &item;
    }
    if (!selected) {
        message = "This task was removed from the file. Reload the queue.";
    }
    return selected;
}

bool same_review(const ReviewTask& task, const json::Value& doc, std::string& message) {
    const auto& saved = doc["reviews"][task.id];
    if (saved["verdict"].string() != task.verdict
        || saved["feedback"].string() != task.saved_feedback) {
        message = "This task's review changed on disk. Reload the queue.";
        return false;
    }
    return true;
}

bool same_camera(const ReviewTask& task, const json::Value& selected, std::string& message) {
    const auto& saved = selected["camera"];
    renderer::Vec3 position{}, forward{};
    if (selected["level"].number(-1) != task.level || !read_vector(saved["position"], position)
        || position != task.position || !read_vector(saved["forward"], forward)
        || forward != task.forward) {
        message = "This task's camera or level changed on disk. Reload the queue.";
        return false;
    }
    return true;
}

bool publish(std::string_view contents, std::string& message) {
    auto staged = StagedFile::create(review_queue_path());
    if (!staged) {
        message = staged.error().message;
        return false;
    }
    std::ofstream out(staged->path(), std::ios::binary);
    out << contents;
    out.close();
    if (!out) {
        message = "Cannot write the review JSON.";
        return false;
    }
    if (const auto result = staged->publish(); !result) {
        message = result.error().message;
        return false;
    }
    return true;
}

}  // namespace

bool save_review(ReviewTask& task, std::string_view verdict, std::string& message) {
    // Read the current file so adding feedback preserves tasks and other reviews edited externally.
    json::Value doc;
    if (!read_for_edit(task, doc, message) || !same_review(task, doc, message)) {
        return false;
    }
    std::ostringstream out;
    out << '{';
    if (write_members(out, doc, 1, "reviews")) {
        out << ',';
    }
    out << "\n  \"reviews\": {";
    if (write_members(out, doc["reviews"], 2, task.id)) {
        out << ',';
    }
    out << "\n    ";
    quote(out, task.id);
    out << ": {";
    if (write_members(out, doc["reviews"][task.id], 3, "verdict", "feedback")) {
        out << ',';
    }
    out << "\n      \"verdict\": ";
    quote(out, verdict);
    out << ",\n      \"feedback\": ";
    quote(out, task.feedback);
    out << "\n    }\n  }\n}\n";
    if (!publish(out.str(), message)) {
        return false;
    }
    task.verdict = verdict;
    task.saved_feedback = task.feedback;
    message = "Review saved.";
    return true;
}

bool save_review_camera(
    ReviewTask& task, renderer::Vec3 position, renderer::Vec3 forward, std::string& message
) {
    const auto finite = [](const auto& v) {
        return std::all_of(v.begin(), v.end(), [](float n) { return std::isfinite(n); });
    };
    if (!finite(position) || !finite(forward) || forward == renderer::Vec3{}) {
        message = "The current camera pose is unavailable.";
        return false;
    }
    json::Value doc;
    const auto* selected = read_for_edit(task, doc, message);
    if (!selected || !same_camera(task, *selected, message)) {
        return false;
    }
    const auto& saved = (*selected)["camera"];

    std::ostringstream out;
    out << '{';
    if (write_members(out, doc, 1, "tasks")) {
        out << ',';
    }
    out << "\n  \"tasks\": [";
    const auto& tasks = doc["tasks"].items();
    for (const auto& item : tasks) {
        out << (&item == &tasks.front() ? "\n    " : ",\n    ");
        if (&item != selected) {
            write_json(out, item, 2);
            continue;
        }
        out << '{';
        write_members(out, item, 3, "camera");
        out << ",\n      \"camera\": {";
        if (write_members(out, saved, 4, "position", "forward")) {
            out << ',';
        }
        out << std::format(
            "\n        \"position\": [{}, {}, {}],\n        \"forward\": [{}, {}, {}]",
            position[0],
            position[1],
            position[2],
            forward[0],
            forward[1],
            forward[2]
        );
        out << "\n      }\n    }";
    }
    out << "\n  ]\n}\n";
    if (!publish(out.str(), message)) {
        return false;
    }
    task.position = position;
    task.forward = forward;
    message = "Camera saved.";
    return true;
}

bool delete_review_task(const ReviewTask& task, std::string& message) {
    json::Value doc;
    const auto* selected = read_for_edit(task, doc, message);
    if (!selected || !same_camera(task, *selected, message) || !same_review(task, doc, message)) {
        return false;
    }
    std::ostringstream out;
    out << '{';
    if (write_members(out, doc, 1, "tasks", "reviews")) {
        out << ',';
    }
    out << "\n  \"tasks\": [";
    bool first = true;
    for (const auto& item : doc["tasks"].items()) {
        if (item["id"].string() == task.id) {
            continue;
        }
        out << (first ? "\n    " : ",\n    ");
        first = false;
        write_json(out, item, 2);
    }
    out << (first ? "]," : "\n  ],") << "\n  \"reviews\": {";
    if (write_members(out, doc["reviews"], 2, task.id)) {
        out << "\n  ";
    }
    out << "}\n}\n";
    return publish(out.str(), message);
}

}  // namespace openrac::frontend::debug_overlay
