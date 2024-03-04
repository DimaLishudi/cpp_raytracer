#pragma once

#include <algorithm>
#include <image.h>
#include <options/camera_options.h>
#include <options/render_options.h>
#include <filesystem>
#include <vector>
#include <geometry.h>
#include <scene.h>
#include <camera.h>
#include <postprocess.h>
#include <tracing.h>

Image Render(const std::filesystem::path& path, const CameraOptions& camera_options,
             const RenderOptions& render_options) {
    Scene scene = ReadScene(path);
    Camera camera(camera_options);
    std::vector<std::vector<Vector>> img_buf;
    const int h = camera_options.screen_height;
    const int w = camera_options.screen_width;

    // for kDepth we need to know which rays don't intersect with object
    // because these rays don't participate in postprocess
    std::optional<std::vector<std::vector<bool>>> inter_mask;
    if (render_options.mode == RenderMode::kDepth) {
        inter_mask.emplace(h, std::vector<bool>(w, false));
    }

    img_buf.reserve(h);
    for (int i = 0; i < h; ++i) {
        img_buf.emplace_back();
        img_buf[i].reserve(w);
        for (int j = 0; j < w; ++j) {
            img_buf[i].push_back(
                TraceRay(camera.GetRay(i, j), scene, render_options, 0, false, i, j, &inter_mask));
        }
    }
    return Postprocess(img_buf, camera_options.screen_width, camera_options.screen_height,
                       render_options.mode, inter_mask);
}
