#pragma once

#include "Media.hpp"

struct Change {
    int channel = 0;
    Rgb color;
};

struct Plan {
    int gridWidth = 0;
    int gridHeight = 0;
    int fps = 1;
    double seconds = 0.0;
    int channelCount = 0;
    bool still = false;
    std::vector<int> cellChannel;
    std::vector<std::vector<Change>> frames;
    size_t changeCount = 0;
};

Plan makePlan(Clip const& clip, int threshold, int paletteSize);
