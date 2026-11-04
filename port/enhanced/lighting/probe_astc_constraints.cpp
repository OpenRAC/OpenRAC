// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Integer null-space search for paired ASTC weight interpolation.
#include "enhanced/lighting/probe_astc_constraints.h"

#include <algorithm>
#include <astcenc_internal.h>
#include <cmath>
#include <numeric>
#include <optional>

#include "enhanced/lighting/probe_compression_tile.h"

namespace openrac::lighting::compression {
namespace {
using Equation = AstcWeightMove;

// Rows describe equality of interpolated weights, without tying their contributing stored weights.
std::vector<Equation> seam_equations(const decimation_info& di) {
    std::vector<Equation> equations;

    for (unsigned p = 0; p < 64; p++) {
        if (gutter_source(p) != p) {
            Equation row{};

            for (unsigned i = 0; i < di.texel_weight_count[p]; i++) {
                row[di.texel_weights_tr[i][p]] += di.texel_weight_contribs_int_tr[i][p];
            }

            const auto q = gutter_source(p);

            for (unsigned i = 0; i < di.texel_weight_count[q]; i++) {
                row[di.texel_weights_tr[i][q]] -= di.texel_weight_contribs_int_tr[i][q];
            }

            equations.push_back(row);
        }
    }

    return equations;
}

using Matrix = std::vector<std::array<double, 64>>;

// Partial pivoting avoids treating a dependent border equation as another constraint.
bool pivot_column(Matrix& matrix, unsigned column, unsigned row, unsigned width) {
    unsigned best = row;

    for (unsigned candidate = row + 1; candidate < matrix.size(); ++candidate) {
        // Use the largest pivot to limit elimination error.
        if (std::abs(matrix[candidate][column]) > std::abs(matrix[best][column])) {
            best = candidate;
        }
    }

    // Dependent columns belong to the free-variable basis.
    if (std::abs(matrix[best][column]) < 1e-8) {
        return false;
    }

    std::swap(matrix[row], matrix[best]);
    const double divisor = matrix[row][column];

    for (unsigned j = column; j < width; ++j) {
        matrix[row][j] /= divisor;
    }

    for (unsigned other = 0; other < matrix.size(); ++other) {
        // The pivot row is already normalized.
        if (other != row) {
            const double scale = matrix[other][column];

            for (unsigned j = column; j < width; ++j) {
                matrix[other][j] -= scale * matrix[row][j];
            }
        }
    }

    return true;
}

// Each integral basis vector must satisfy the original integer equations exactly.
bool valid_move(const Equation& action, const std::vector<Equation>& equations) {
    for (const auto& row : equations) {
        const int sum = std::inner_product(row.begin(), row.end(), action.begin(), 0);

        // Floating elimination is only a proposal; integer constraints decide acceptance.
        if (sum != 0) {
            return false;
        }
    }

    return std::all_of(action.begin(), action.end(), [](int value) {
        return std::abs(value) <= 64;  // Larger moves cannot fit ASTC's legal weight range.
    });
}

// Powers-of-two interpolation coefficients admit small integral null-space moves.
std::optional<Equation> integer_move(
    const std::array<double, 64>& basis, const std::vector<Equation>& equations, unsigned width
) {
    for (int scale = 1; scale <= 4096; ++scale) {
        Equation action{};
        bool integer = true;

        for (unsigned column = 0; column < width; ++column) {
            const double value = basis[column] * scale;
            action[column] = static_cast<int>(std::round(value));
            integer = integer && std::abs(value - action[column]) < 1e-6;
        }

        // Try another denominator until all coefficients are integral.
        if (!integer) {
            continue;
        }

        int divisor = 0;

        for (auto value : action) {
            divisor = std::gcd(divisor, value);
        }

        for (auto& value : action) {
            value /= divisor;
        }

        // Quantized candidates must preserve every interpolated border pair.
        if (valid_move(action, equations)) {
            return action;
        }

        return std::nullopt;
    }

    return std::nullopt;
}

}  // namespace

// Cached integer null-space moves permit unequal stored weights with equal border interpolation.
const std::vector<AstcWeightMove>& AstcInterpolation::moves(const decimation_info& di) {
    const auto cached = m_moves.find(&di);

    // The descriptor remains immutable for the lifetime of this encoder.
    if (cached != m_moves.end()) {
        return cached->second;
    }

    const auto equations = seam_equations(di);
    Matrix matrix(equations.size());

    for (unsigned row = 0; row < matrix.size(); ++row) {
        std::copy(equations[row].begin(), equations[row].end(), matrix[row].begin());
    }

    std::vector<unsigned> pivots;

    for (unsigned column = 0; column < di.weight_count && pivots.size() < matrix.size(); ++column) {
        // Only independent equations advance the pivot row.
        if (pivot_column(matrix, column, static_cast<unsigned>(pivots.size()), di.weight_count)) {
            pivots.push_back(column);
        }
    }

    std::vector<AstcWeightMove> moves;

    for (unsigned free = 0; free < di.weight_count; ++free) {
        // Pivot columns are dependent variables, not basis generators.
        if (std::find(pivots.begin(), pivots.end(), free) != pivots.end()) {
            continue;
        }

        std::array<double, 64> basis{};
        basis[free] = 1;

        for (unsigned row = 0; row < pivots.size(); ++row) {
            basis[pivots[row]] = -matrix[row][free];
        }

        // Nonintegral or oversized proposals are unusable for stored integer weights.
        if (auto action = integer_move(basis, equations, di.weight_count)) {
            moves.push_back(*action);
        }
    }

    return m_moves.emplace(&di, std::move(moves)).first->second;
}

}  // namespace openrac::lighting::compression
