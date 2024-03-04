#pragma once

#include <geometry.h>
#include <options/camera_options.h>

class Camera {
public:
    Camera(const CameraOptions& options) {
        orig_ = options.look_from;
        Vector forward = options.look_to - options.look_from;
        forward.Normalize();
        double dot = DotProduct(forward, geom::kOY);
        if (dot < -1 + geom::kEPS || 1 - geom::kEPS < dot) {
            // camera is parallel to Oy
            up_ = CrossProduct(geom::kOX, forward);
            up_.Normalize();
            right_ = CrossProduct(forward, up_);
        } else {
            right_ = CrossProduct(forward, geom::kOY);
            right_.Normalize();
            up_ = CrossProduct(right_, forward);
        }
        double delta = 2 * tan(options.fov / 2) / options.screen_height;
        up_ *= delta;
        right_ *= delta;
        base_ = forward + up_ * (options.screen_height - 1) / 2.0;
        base_ -= right_ * (options.screen_width - 1) / 2.0;
    }

    Ray GetRay(size_t i, size_t j) const {
        return Ray(orig_, base_ + j * right_ - i * up_);
    }

private:
    Vector orig_, base_, up_, right_, delta_;
};
