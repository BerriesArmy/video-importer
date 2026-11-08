#include "Writer.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

namespace {

constexpr size_t chunkSize = 1500;
constexpr float blockUnits = 30.f;
constexpr int backgroundLayer = -5;
constexpr int movieLayer = 3;
constexpr int colorTriggerId = 899;
constexpr int moveTriggerId = 901;
constexpr int spawnTriggerId = 1268;
constexpr int endTriggerId = 3600;
constexpr int hidePlayerId = 1612;
constexpr int disableTrailId = 33;
constexpr int backgroundChannel = 1000;
constexpr float triggerSpacing = 3.f;
constexpr size_t triggersPerRow = 400;

class ObjectList {
public:
    std::vector<std::string> chunks;
    size_t total = 0;

    void add(std::string const& object) {
        current += object;
        current += ';';
        ++pending;
        ++total;
        if (pending == chunkSize) flush();
    }

    void flush() {
        if (pending == 0) return;
        chunks.push_back(std::move(current));
        current.clear();
        pending = 0;
    }

private:
    std::string current;
    size_t pending = 0;
};

std::string pixel(int id, float x, float y, int channel, int layer, float scale, int group) {
    return fmt::format(
        "1,{},2,{:.2f},3,{:.2f},21,{},22,{},24,{},32,{:.3f},57,{},64,1,67,1,121,1",
        id, x, y, channel, channel, layer, scale, group
    );
}

std::string colorChange(float x, float y, int channel, Rgb color, int group) {
    return fmt::format(
        "1,{},2,{:.1f},3,{:.1f},7,{},8,{},9,{},10,0,23,{},62,1,87,1,57,{}",
        colorTriggerId, x, y, int(color.r), int(color.g), int(color.b), channel, group
    );
}

std::string spawnAfter(float x, float y, int target, double delay, int group) {
    return fmt::format(
        "1,{},2,{:.1f},3,{:.1f},51,{},63,{:.4f},62,1,87,1,57,{}",
        spawnTriggerId, x, y, target, delay, group
    );
}

std::string spawnOnReach(float x, float y, int target) {
    return fmt::format("1,{},2,{:.1f},3,{:.1f},51,{}", spawnTriggerId, x, y, target);
}

std::string spawned(int id, float x, float y, int group) {
    return fmt::format("1,{},2,{:.1f},3,{:.1f},62,1,87,1,57,{}", id, x, y, group);
}

}

BuiltLevel buildLevel(Plan const& plan, Layout const& layout) {
    BuiltLevel built;
    BuildInfo& info = built.info;
    info.videoGroup = layout.firstGroup;
    info.startGroup = layout.firstGroup + 1;
    info.endGroup = layout.firstGroup + 2;
    info.frameCount = static_cast<int>(plan.frames.size());

    double frameTime = 1.0 / std::max(1, plan.fps);
    info.seconds = info.frameCount * frameTime;

    float cell = layout.videoWidth / plan.gridWidth;
    float scale = cell / blockUnits;
    float videoHeight = cell * plan.gridHeight;
    float triggerX = layout.centerX - layout.videoWidth / 2.f;
    float triggerY = layout.centerY + videoHeight / 2.f + 200.f;
    info.anchorX = triggerX;
    info.anchorY = triggerY - 40.f;

    ObjectList list;
    int layer = layout.background ? backgroundLayer : movieLayer;

    for (int row = 0; row < plan.gridHeight; ++row) {
        for (int col = 0; col < plan.gridWidth; ++col) {
            int cellIndex = row * plan.gridWidth + col;
            float x = layout.centerX + (col - (plan.gridWidth - 1) / 2.f) * cell;
            float y = layout.centerY + ((plan.gridHeight - 1) / 2.f - row) * cell;
            list.add(pixel(layout.blockId, x, y, layout.firstChannel + cellIndex, layer, scale, info.videoGroup));
        }
    }

    size_t placed = 0;
    auto nextX = [&] { return triggerX + static_cast<float>(placed % triggersPerRow) * triggerSpacing; };
    auto nextY = [&] { return triggerY + static_cast<float>(placed / triggersPerRow) * triggerSpacing; };

    int frameGroupBase = layout.firstGroup + 3;
    for (int k = 0; k < info.frameCount; ++k) {
        auto const& changes = plan.frames[k];
        if (changes.empty()) continue;
        int group = frameGroupBase + k;
        for (auto const& change : changes) {
            list.add(colorChange(nextX(), nextY(), layout.firstChannel + change.cell, change.color, group));
            ++placed;
        }
        list.add(spawnAfter(nextX(), nextY(), group, k * frameTime, info.startGroup));
        ++placed;
    }

    list.add(spawnOnReach(0.f, 15.f, info.startGroup));
    list.flush();

    built.chunks = std::move(list.chunks);
    built.objectCount = list.total;
    return built;
}

std::string buildExtras(Extras const& extras, BuildInfo const& info) {
    std::string out;
    int slot = 0;

    auto put = [&](auto&& make) {
        float x = info.anchorX + static_cast<float>(slot) * 6.f;
        ++slot;
        out += make(x, info.anchorY);
        out += ';';
    };

    bool loop = extras.loop && !extras.endLevel;

    if (extras.hidePlayer) {
        put([&](float x, float y) { return spawned(hidePlayerId, x, y, info.startGroup); });
        put([&](float x, float y) { return spawned(disableTrailId, x, y, info.startGroup); });
    }

    if (extras.lockCamera) {
        double lockSeconds = loop ? 3600.0 : info.seconds + 1.0;
        out += fmt::format(
            "1,{},2,0,3,{:.1f},10,{:.2f},51,{},141,1,142,1;",
            moveTriggerId, info.anchorY, lockSeconds, info.videoGroup
        );
    }

    if (loop) {
        put([&](float x, float y) { return spawnAfter(x, y, info.startGroup, info.seconds, info.startGroup); });
    }

    if (extras.endLevel) {
        put([&](float x, float y) { return spawned(endTriggerId, x, y, info.endGroup); });
        put([&](float x, float y) { return spawnAfter(x, y, info.endGroup, info.seconds, info.startGroup); });
    }

    if (extras.blackBackground) {
        put([&](float x, float y) {
            return fmt::format(
                "1,{},2,{:.1f},3,{:.1f},7,0,8,0,9,0,10,0,23,{},62,1,87,1,57,{}",
                colorTriggerId, x, y, backgroundChannel, info.startGroup
            );
        });
    }

    return out;
}
