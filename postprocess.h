#pragma once

#include <image.h>
#include <vector>
#include <geometry.h>
#include <options/render_options.h>
#include <iostream>

constexpr double kGAMMA = 1. / 2.2;

Image Postprocess(const std::vector<std::vector<Vector>>& buf, unsigned width, unsigned height,
                  RenderMode render_mode,
                  const std::optional<std::vector<std::vector<bool>>>& inter_mask) {
    double max_c = 0;
    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
            if (inter_mask && !inter_mask.value()[i][j]) {
                continue;
            }
            if (buf[i][j][0] > max_c) {
                max_c = buf[i][j][0];
            }
            if (buf[i][j][1] > max_c) {
                max_c = buf[i][j][1];
            }
            if (buf[i][j][2] > max_c) {
                max_c = buf[i][j][2];
            }
        }
    }

    double max_c_sq = max_c * max_c;
    if (max_c_sq == 0.) {
        // if all pixels are black we'll replace max with 1 to get rid of nan's
        max_c_sq = 1;
    }
    Image result(width, height);

    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
            Vector v = buf[i][j];
            if (render_mode == RenderMode::kDepth && inter_mask.value()[i][j]) {
                v = v / max_c;
            } else if (render_mode == RenderMode::kFull) {
                v = v * (1 + v / max_c_sq) / (1 + v);
                v[0] = pow(v[0], kGAMMA);
                v[1] = pow(v[1], kGAMMA);
                v[2] = pow(v[2], kGAMMA);
            }
            RGB pixel(v[0] * 255, v[1] * 255, v[2] * 255);
            result.SetPixel(pixel, i, j);
        }
    }
    return result;
}