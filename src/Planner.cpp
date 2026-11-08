#include "Planner.hpp"

#include <algorithm>
#include <cstdlib>

namespace {

int distance(Rgb const& a, Rgb const& b) {
    return std::max({
        std::abs(static_cast<int>(a.r) - b.r),
        std::abs(static_cast<int>(a.g) - b.g),
        std::abs(static_cast<int>(a.b) - b.b)
    });
}

}

Plan makePlan(Clip const& clip, int threshold) {
    Plan plan;
    plan.gridWidth = clip.width;
    plan.gridHeight = clip.height;
    plan.fps = clip.fps;

    size_t cells = static_cast<size_t>(clip.width) * clip.height;
    std::vector<Rgb> shown(cells);
    std::vector<bool> known(cells, false);

    for (auto const& frame : clip.frames) {
        std::vector<Change> changes;
        for (size_t i = 0; i < cells; ++i) {
            Rgb const& next = frame[i];
            if (known[i] && distance(shown[i], next) <= threshold) continue;
            shown[i] = next;
            known[i] = true;
            changes.push_back({static_cast<int>(i), next});
        }
        plan.changeCount += changes.size();
        plan.frames.push_back(std::move(changes));
    }
    return plan;
}
