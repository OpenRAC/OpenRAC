// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "common/json.h"
#include "enhanced/lighting/interiors.h"
using namespace openrac;
namespace in = lighting::interiors;

int main(int argc, char** argv) {
    try {
        if (argc < 3 || argc > 4) {
            throw std::runtime_error("openrac-interior-infer INPUT.json OUTPUT.json [SPACING]");
        }
        if (std::filesystem::exists(argv[2])) {
            throw std::runtime_error("refusing to overwrite output");
        }
        json::Value data;
        std::string error;
        if (!json::parse_file(argv[1], data, error)) {
            throw std::runtime_error(error);
        }
        if (data["format"].number() != 1) {
            throw std::runtime_error("unsupported inference input");
        }
        if ((data["game"].string() != "rac1-ntsc" && data["game"].string() != "rac1-pal")
            || data["level"].number(-1) < 0 || data["level"].number() > 18
            || data["source_sha256"].string().size() != 64) {
            throw std::runtime_error("invalid source identity");
        }
        std::vector<in::Triangle> triangles;
        std::vector<in::Transition> transitions;
        for (const auto& row : data["triangles"].items()) {
            if (row.items().size() != 10) {
                throw std::runtime_error("invalid collision triangle");
            }
            in::Triangle t{};
            for (unsigned i = 0; i < 9; ++i) {
                t.vertices[i / 3][i % 3] = static_cast<float>(row[i].number());
            }
            t.support = row[9].number() != 0;
            triangles.push_back(t);
        }
        for (const auto& row : data["transitions"].items()) {
            if (row["inverse"].items().size() != 16 || row["light"].items().size() != 2
                || row["ambient"].items().size() != 2) {
                throw std::runtime_error("invalid transition");
            }
            in::Transition t{};
            t.id = static_cast<unsigned>(row["id"].number());
            for (unsigned i = 0; i < 16; ++i) {
                t.inverse[i] = static_cast<float>(row["inverse"][i].number());
            }
            for (unsigned i = 0; i < 2; ++i) {
                t.light[i] = static_cast<int>(row["light"][i].number());
                for (unsigned k = 0; k < 3; ++k) {
                    t.ambient[i][k] = static_cast<unsigned>(row["ambient"][i][k].number());
                }
            }
            transitions.push_back(t);
        }
        in::Settings settings;
        if (argc == 4) {
            settings.spacing = std::stof(argv[3]);
        }
        const auto result = in::infer(triangles, transitions, settings);
        std::ofstream out(argv[2]);
        out << std::setprecision(8);
        out << "{\"format\":1,\"game\":\"" << data["game"].string()
            << "\",\"level\":" << data["level"].number() << ",\"source_sha256\":\""
            << data["source_sha256"].string() << "\",\"spacing\":" << settings.spacing
            << ",\"height\":" << settings.height << ",\"radius\":" << settings.radius
            << ",\"step\":" << settings.step << ",\"sky_distance\":" << settings.sky_distance
            << ",\"blocked_portal_edges\":" << result.blocked_portal_edges << ",\"transitions\":[";
        auto point = [&](in::Point p) {
            out << '[' << p[0] << ',' << p[1] << ',' << p[2] << ']';
        };
        for (unsigned i = 0; i < transitions.size(); ++i) {
            if (i) {
                out << ',';
            }
            const auto& t = transitions[i];
            out << "{\"id\":" << t.id << ",\"corners\":[";
            for (unsigned k = 0; k < 8; ++k) {
                if (k) {
                    out << ',';
                }
                point(in::world(t, {k & 1 ? 1.f : -1.f, k & 2 ? 1.f : -1.f, k & 4 ? 1.f : -1.f}));
            }
            out << "],\"light\":[" << t.light[0] << ',' << t.light[1] << "],\"ambient\":[";
            for (unsigned k = 0; k < 2; ++k) {
                if (k) {
                    out << ',';
                }
                out << '[' << t.ambient[k][0] << ',' << t.ambient[k][1] << ',' << t.ambient[k][2]
                    << ']';
            }
            out << "]}";
        }
        out << "],\"cells\":[";
        for (unsigned i = 0; i < result.cells.size(); ++i) {
            if (i) {
                out << ',';
            }
            const auto& c = result.cells[i];
            out << '[' << c.position[0] << ',' << c.position[1] << ',' << c.position[2] << ','
                << c.component << ',' << c.sky << ']';
        }
        out << "],\"regions\":[";
        for (unsigned i = 0; i < result.regions.size(); ++i) {
            if (i) {
                out << ',';
            }
            const auto& r = result.regions[i];
            out << "{\"id\":" << i << ",\"count\":" << r.cells.size() << ",\"sky\":" << r.sky
                << ",\"labels\":[";
            for (unsigned k = 0; k < r.labels.size(); ++k) {
                if (k) {
                    out << ',';
                }
                out << r.labels[k];
            }
            out << "]}";
        }
        out << "],\"seeds\":[";
        for (unsigned i = 0; i < result.seeds.size(); ++i) {
            if (i) {
                out << ',';
            }
            const auto& s = result.seeds[i];
            out << "{\"transition\":" << s.transition << ",\"side\":" << s.side
                << ",\"label\":" << s.label << ",\"cell\":" << s.cell << ",\"target\":";
            point(s.target);
            out << '}';
        }
        out << "]}\n";
        out.close();
        if (!out) {
            throw std::runtime_error("failed to save inference output");
        }
        std::cout << result.cells.size() << " walkable samples, " << result.regions.size()
                  << " connected components, " << result.blocked_portal_edges
                  << " portal edges blocked\n";
        for (const auto& s : result.seeds) {
            std::cout << "transition " << s.transition << " side " << s.side << " seed " << s.cell
                      << " label " << s.label << '\n';
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
