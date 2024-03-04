#pragma once

#include <scene.h>

enum class RenderMode { kDepth, kNormal, kFull };

struct RenderOptions {
    int depth;
    RenderMode mode = RenderMode::kFull;
};
