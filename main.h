#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cassert>
#include <vector>

struct Vertex {
    float x, y;
    uint8_t r, g, b, a;
};

std::vector<std::byte> load_file(const char* file);
void initialize(HWND hwnd);
void shutdown();
void render();
