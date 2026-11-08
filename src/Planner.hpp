#pragma once

#include "Media.hpp"

struct Change {
    int cell = 0;
    Rgb color;
};

struct Plan {
    int gridWidth = 0;
    int gridHeight = 0;
    int fps = 1;
    std::vector<std::vector<Change>> frames;
    size_t changeCount = 0;
};

Plan makePlan(Clip const& clip, int threshold);
