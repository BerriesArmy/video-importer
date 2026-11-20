#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct LoadSettings {
    int gridWidth = 40;
    int fps = 12;
    int colorLevels = 16;
    int maxFrames = 600;
    int maxCells = 899;
    int stillWidth = 120;
    int stillMaxCells = 12000;
    double stillSeconds = 5.0;
};

struct Clip {
    int width = 0;
    int height = 0;
    int fps = 1;
    double seconds = 0.0;
    bool still = false;
    std::vector<std::vector<Rgb>> frames;
    std::string error;
};

Clip loadClip(std::filesystem::path const& source, LoadSettings const& settings);
