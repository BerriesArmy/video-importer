#include "Media.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iterator>
#include <system_error>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "vendor/stb_image.h"

namespace {

struct Pixels {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgba;
};

std::string utf8(std::filesystem::path const& path) {
    auto text = path.u8string();
    return std::string(text.begin(), text.end());
}

std::string lowerExtension(std::filesystem::path const& path) {
    std::string ext = utf8(path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext;
}

bool isImageExtension(std::string const& ext) {
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

std::vector<uint8_t> readFile(std::filesystem::path const& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool naturalLess(std::string const& a, std::string const& b) {
    size_t i = 0;
    size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (std::isdigit(static_cast<unsigned char>(a[i])) && std::isdigit(static_cast<unsigned char>(b[j]))) {
            size_t startA = i;
            size_t startB = j;
            while (i < a.size() && std::isdigit(static_cast<unsigned char>(a[i]))) ++i;
            while (j < b.size() && std::isdigit(static_cast<unsigned char>(b[j]))) ++j;
            std::string numA = a.substr(startA, i - startA);
            std::string numB = b.substr(startB, j - startB);
            numA.erase(0, std::min(numA.find_first_not_of('0'), numA.size()));
            numB.erase(0, std::min(numB.find_first_not_of('0'), numB.size()));
            if (numA.size() != numB.size()) return numA.size() < numB.size();
            if (numA != numB) return numA < numB;
        } else {
            int charA = std::tolower(static_cast<unsigned char>(a[i]));
            int charB = std::tolower(static_cast<unsigned char>(b[j]));
            if (charA != charB) return charA < charB;
            ++i;
            ++j;
        }
    }
    return (a.size() - i) < (b.size() - j);
}

uint8_t snap(long total, long count, int levels) {
    double value = static_cast<double>(total) / static_cast<double>(count);
    double step = 255.0 / (levels - 1);
    double snapped = std::round(value / step) * step;
    return static_cast<uint8_t>(std::clamp(std::lround(snapped), 0L, 255L));
}

std::vector<Rgb> toGrid(uint8_t const* rgba, int width, int height, int gridW, int gridH, int levels) {
    std::vector<Rgb> cells(static_cast<size_t>(gridW) * gridH);
    for (int gy = 0; gy < gridH; ++gy) {
        int y0 = gy * height / gridH;
        int y1 = std::min(height, std::max(y0 + 1, (gy + 1) * height / gridH));
        for (int gx = 0; gx < gridW; ++gx) {
            int x0 = gx * width / gridW;
            int x1 = std::min(width, std::max(x0 + 1, (gx + 1) * width / gridW));
            long red = 0;
            long green = 0;
            long blue = 0;
            long count = 0;
            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    uint8_t const* p = rgba + (static_cast<size_t>(y) * width + x) * 4;
                    int alpha = p[3];
                    red += p[0] * alpha / 255;
                    green += p[1] * alpha / 255;
                    blue += p[2] * alpha / 255;
                    ++count;
                }
            }
            cells[static_cast<size_t>(gy) * gridW + gx] = Rgb{
                snap(red, count, levels),
                snap(green, count, levels),
                snap(blue, count, levels)
            };
        }
    }
    return cells;
}

bool decode(std::vector<uint8_t> const& bytes, Pixels& out) {
    if (bytes.empty()) return false;
    int components = 0;
    stbi_uc* data = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &out.width, &out.height, &components, 4);
    if (!data) return false;
    out.rgba.assign(data, data + static_cast<size_t>(out.width) * out.height * 4);
    stbi_image_free(data);
    return true;
}

void fitGrid(int sourceW, int sourceH, LoadSettings const& settings, int& gridW, int& gridH) {
    auto heightFor = [&](int w) {
        return std::max(1, static_cast<int>(std::lround(w * static_cast<double>(sourceH) / sourceW)));
    };
    gridW = std::max(1, settings.gridWidth);
    gridH = heightFor(gridW);
    while (gridW > 1 && gridW * gridH > settings.maxCells) {
        --gridW;
        gridH = heightFor(gridW);
    }
}

Clip fromImages(std::vector<std::filesystem::path> const& files, LoadSettings const& settings) {
    Clip clip;
    clip.fps = settings.fps;
    size_t limit = std::min(files.size(), static_cast<size_t>(settings.maxFrames));
    for (size_t i = 0; i < limit; ++i) {
        Pixels pixels;
        if (!decode(readFile(files[i]), pixels)) {
            clip.error = "Could not read " + utf8(files[i].filename());
            return clip;
        }
        if (i == 0) fitGrid(pixels.width, pixels.height, settings, clip.width, clip.height);
        clip.frames.push_back(toGrid(pixels.rgba.data(), pixels.width, pixels.height, clip.width, clip.height, settings.colorLevels));
    }
    if (clip.frames.empty()) clip.error = "No images found";
    return clip;
}

Clip fromGif(std::vector<uint8_t> const& bytes, LoadSettings const& settings) {
    Clip clip;
    clip.fps = settings.fps;
    int* delays = nullptr;
    int width = 0;
    int height = 0;
    int count = 0;
    int components = 0;
    stbi_uc* data = stbi_load_gif_from_memory(bytes.data(), static_cast<int>(bytes.size()), &delays, &width, &height, &count, &components, 4);
    if (!data || count < 1) {
        if (data) stbi_image_free(data);
        if (delays) stbi_image_free(delays);
        clip.error = "Could not read this GIF";
        return clip;
    }

    fitGrid(width, height, settings, clip.width, clip.height);

    std::vector<long> ends(count);
    long total = 0;
    for (int i = 0; i < count; ++i) {
        int delay = delays ? delays[i] : 100;
        if (delay < 20) delay = 100;
        total += delay;
        ends[i] = total;
    }

    long wanted = std::max(1L, total * settings.fps / 1000);
    int outCount = static_cast<int>(std::min<long>(wanted, settings.maxFrames));

    size_t frameBytes = static_cast<size_t>(width) * height * 4;
    int index = 0;
    int converted = -1;
    std::vector<Rgb> cells;
    for (int k = 0; k < outCount; ++k) {
        double time = k * 1000.0 / settings.fps;
        while (index + 1 < count && ends[index] <= time) ++index;
        if (index != converted) {
            cells = toGrid(data + frameBytes * index, width, height, clip.width, clip.height, settings.colorLevels);
            converted = index;
        }
        clip.frames.push_back(cells);
    }

    stbi_image_free(data);
    if (delays) stbi_image_free(delays);
    return clip;
}

std::vector<std::filesystem::path> imagesIn(std::filesystem::path const& folder) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    std::filesystem::directory_iterator it(folder, error);
    std::filesystem::directory_iterator end;
    for (; !error && it != end; it.increment(error)) {
        std::error_code entryError;
        if (it->is_regular_file(entryError) && isImageExtension(lowerExtension(it->path()))) {
            files.push_back(it->path());
        }
    }
    std::sort(files.begin(), files.end(), [](auto const& a, auto const& b) {
        return naturalLess(utf8(a.filename()), utf8(b.filename()));
    });
    return files;
}

}

Clip loadClip(std::filesystem::path const& source, LoadSettings const& settings) {
    std::error_code error;
    if (std::filesystem::is_directory(source, error)) {
        auto files = imagesIn(source);
        if (files.empty()) {
            Clip clip;
            clip.error = "No images in that folder";
            return clip;
        }
        return fromImages(files, settings);
    }

    std::string ext = lowerExtension(source);
    if (ext == ".gif") {
        auto bytes = readFile(source);
        if (bytes.empty()) {
            Clip clip;
            clip.error = "Could not open that file";
            return clip;
        }
        return fromGif(bytes, settings);
    }
    if (isImageExtension(ext)) {
        return fromImages({source}, settings);
    }

    Clip clip;
    clip.error = "That file type is not supported yet";
    return clip;
}
