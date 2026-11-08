#pragma once

#include "Planner.hpp"

#include <string>
#include <vector>

struct Layout {
    float centerX = 285.f;
    float centerY = 165.f;
    float videoWidth = 480.f;
    bool background = true;
    int blockId = 211;
    int firstChannel = 101;
    int firstGroup = 1000;
};

struct BuildInfo {
    int videoGroup = 0;
    int startGroup = 0;
    int endGroup = 0;
    int frameCount = 0;
    double seconds = 0.0;
    float anchorX = 0.f;
    float anchorY = 0.f;
};

struct BuiltLevel {
    std::vector<std::string> chunks;
    BuildInfo info;
    size_t objectCount = 0;
};

struct Extras {
    bool hidePlayer = false;
    bool lockCamera = true;
    bool loop = false;
    bool endLevel = false;
    bool blackBackground = false;
};

BuiltLevel buildLevel(Plan const& plan, Layout const& layout);
std::string buildExtras(Extras const& extras, BuildInfo const& info);
