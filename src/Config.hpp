#pragma once

#include "Media.hpp"
#include "Writer.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>

struct ImportConfig {
    LoadSettings load;
    int threshold = 12;
    int paletteSize = 256;
    Layout layout;
};

inline ImportConfig readConfig() {
    auto mod = geode::Mod::get();
    auto whole = [mod](char const* key) {
        return static_cast<int>(mod->getSettingValue<int64_t>(key));
    };

    ImportConfig config;
    int firstGroup = whole("first-group");
    int firstChannel = whole("first-channel");

    config.load.gridWidth = whole("grid-width");
    config.load.fps = whole("fps");
    config.load.colorLevels = std::max(2, whole("color-levels"));
    config.load.maxCells = std::max(1, 999 - firstChannel + 1);
    config.load.maxFrames = std::max(1, std::min(whole("max-frames"), 9999 - firstGroup - 3));
    config.threshold = whole("change-threshold");

    int limit = whole("object-limit");
    int channels = std::max(1, 999 - firstChannel + 1);
    config.paletteSize = std::min(channels, 256);
    config.load.stillWidth = whole("still-grid-width");
    config.load.stillMaxCells = std::max(100, limit - 600 - config.paletteSize);
    config.load.stillSeconds = static_cast<double>(whole("still-seconds"));
    config.layout.objectLimit = static_cast<size_t>(limit);

    config.layout.background = mod->getSettingValue<std::string>("placement") == "Background";
    config.layout.centerX = static_cast<float>(whole("center-x"));
    config.layout.centerY = static_cast<float>(whole("center-y"));
    config.layout.videoWidth = static_cast<float>(whole("video-width"));
    config.layout.blockId = whole("block-id");
    config.layout.firstChannel = firstChannel;
    config.layout.firstGroup = firstGroup;
    return config;
}
