#include "Planner.hpp"

#include <algorithm>
#include <cstdlib>
#include <numeric>

namespace {

int distance(Rgb const& a, Rgb const& b) {
    return std::max({
        std::abs(static_cast<int>(a.r) - b.r),
        std::abs(static_cast<int>(a.g) - b.g),
        std::abs(static_cast<int>(a.b) - b.b)
    });
}

int component(Rgb const& color, int index) {
    return index == 0 ? color.r : (index == 1 ? color.g : color.b);
}

std::vector<std::vector<size_t>> splitBoxes(std::vector<Rgb> const& cells, size_t target) {
    std::vector<std::vector<size_t>> boxes(1);
    boxes[0].resize(cells.size());
    std::iota(boxes[0].begin(), boxes[0].end(), size_t{0});

    while (boxes.size() < target) {
        size_t pick = boxes.size();
        int bestRange = 0;
        int bestChannel = 0;
        for (size_t b = 0; b < boxes.size(); ++b) {
            if (boxes[b].size() < 2) continue;
            int low[3] = {255, 255, 255};
            int high[3] = {0, 0, 0};
            for (size_t i : boxes[b]) {
                for (int c = 0; c < 3; ++c) {
                    int value = component(cells[i], c);
                    low[c] = std::min(low[c], value);
                    high[c] = std::max(high[c], value);
                }
            }
            for (int c = 0; c < 3; ++c) {
                if (high[c] - low[c] > bestRange) {
                    bestRange = high[c] - low[c];
                    bestChannel = c;
                    pick = b;
                }
            }
        }
        if (pick == boxes.size()) break;

        auto& box = boxes[pick];
        std::sort(box.begin(), box.end(), [&](size_t a, size_t b) {
            return component(cells[a], bestChannel) < component(cells[b], bestChannel);
        });
        size_t middle = box.size() / 2;
        std::vector<size_t> upper(box.begin() + middle, box.end());
        box.resize(middle);
        boxes.push_back(std::move(upper));
    }
    return boxes;
}

Plan stillPlan(Clip const& clip, int paletteSize) {
    Plan plan;
    plan.gridWidth = clip.width;
    plan.gridHeight = clip.height;
    plan.fps = clip.fps;
    plan.seconds = clip.seconds;
    plan.still = true;

    auto const& cells = clip.frames.front();
    auto boxes = splitBoxes(cells, static_cast<size_t>(std::max(1, paletteSize)));
    plan.cellChannel.assign(cells.size(), 0);

    std::vector<Change> changes;
    for (size_t b = 0; b < boxes.size(); ++b) {
        long red = 0;
        long green = 0;
        long blue = 0;
        for (size_t i : boxes[b]) {
            plan.cellChannel[i] = static_cast<int>(b);
            red += cells[i].r;
            green += cells[i].g;
            blue += cells[i].b;
        }
        long count = static_cast<long>(boxes[b].size());
        Rgb average{
            static_cast<uint8_t>(red / count),
            static_cast<uint8_t>(green / count),
            static_cast<uint8_t>(blue / count)
        };
        changes.push_back({static_cast<int>(b), average});
    }

    plan.channelCount = static_cast<int>(boxes.size());
    plan.changeCount = changes.size();
    plan.frames.push_back(std::move(changes));
    return plan;
}

Plan videoPlan(Clip const& clip, int threshold) {
    Plan plan;
    plan.gridWidth = clip.width;
    plan.gridHeight = clip.height;
    plan.fps = clip.fps;
    plan.seconds = clip.seconds;

    size_t cells = static_cast<size_t>(clip.width) * clip.height;
    plan.channelCount = static_cast<int>(cells);
    plan.cellChannel.resize(cells);
    std::iota(plan.cellChannel.begin(), plan.cellChannel.end(), 0);

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

}

Plan makePlan(Clip const& clip, int threshold, int paletteSize) {
    if (clip.still) return stillPlan(clip, paletteSize);
    return videoPlan(clip, threshold);
}
