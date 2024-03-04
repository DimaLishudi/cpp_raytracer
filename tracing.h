#pragma once

#include <options/render_options.h>
#include <filesystem>
#include <geometry.h>
#include <scene.h>
#include <cmath>

std::pair<std::optional<Intersection>, const Material*> FirstIntersection(const Ray& ray,
                                                                          const Scene& scene) {
    const Object* obj_ptr = nullptr;
    std::optional<Intersection> min_inter;
    const Material* material = nullptr;
    // check spheres
    for (const auto& sphere_obj : scene.GetSphereObjects()) {
        auto maybe_inter = GetIntersection(ray, sphere_obj.sphere);
        if (maybe_inter) {
            if (!min_inter || maybe_inter->GetDistance() < min_inter->GetDistance()) {
                min_inter = maybe_inter;
                material = sphere_obj.material;
            }
        }
    }
    // check triangles
    for (const auto& obj : scene.GetObjects()) {
        auto maybe_inter = GetIntersection(ray, obj.polygon);
        if (maybe_inter) {
            if (!min_inter || maybe_inter->GetDistance() < min_inter->GetDistance()) {
                min_inter = maybe_inter;
                obj_ptr = &obj;
                material = obj.material;
            }
        }
    }
    // recalculate normals for triangle
    if (obj_ptr && obj_ptr->HasNormals()) {
        Triangle normals = obj_ptr->GetNormals();
        Vector weights = GetBarycentricCoords(obj_ptr->polygon, min_inter->GetPosition());
        Vector normal = normals[0] * weights[0] + normals[1] * weights[1] + normals[2] * weights[2];
        if (DotProduct(normal, ray.GetDirection()) > 0) {
            min_inter->SetNormal(-normal);
        } else {
            min_inter->SetNormal(normal);
        }
    }
    return std::pair(min_inter, material);
}

Vector CalcDiffSpecular(const Light& light, const Scene& scene, const Material* material,
                        const Vector& origin, const Vector& ray_dir, const Vector& normal) {
    // more lightweight version of FirstIntersection to check if in shadow
    bool intersects = false;
    Ray ray(origin, light.position - origin);
    double light_dist = (light.position - origin).Norm();
    for (const auto& sphere_obj : scene.GetSphereObjects()) {
        auto maybe_inter = GetIntersection(ray, sphere_obj.sphere);
        if (maybe_inter && maybe_inter->GetDistance() < light_dist) {
            intersects = true;
            break;
        }
    }
    if (!intersects) {
        for (const auto& obj : scene.GetObjects()) {
            auto maybe_inter = GetIntersection(ray, obj.polygon);
            if (maybe_inter && maybe_inter->GetDistance() < light_dist) {
                intersects = true;
                break;
            }
        }
    }
    if (intersects) {
        return Vector();
    }
    double diff_cos = DotProduct(ray.GetDirection(), normal);
    double spec_cos = DotProduct(Reflect(-ray.GetDirection(), normal), -ray_dir);
    if (spec_cos < 0.) {
        spec_cos = 0.;
    }
    if (diff_cos < 0.) {
        diff_cos = 0.;
    }
    spec_cos = pow(spec_cos, material->specular_exponent);
    Vector res = material->diffuse_color * light.intensity * diff_cos;
    res += material->specular_color * light.intensity * spec_cos;
    return res;
}

Vector TraceRay(const Ray& ray, const Scene& scene, const RenderOptions& render_options, int depth,
                bool inside = false, size_t i = 0, size_t j = 0,
                std::optional<std::vector<std::vector<bool>>>* inter_mask = nullptr) {
    auto [maybe_inter, material] = FirstIntersection(ray, scene);

    if (render_options.mode == RenderMode::kDepth) {
        if (!maybe_inter) {
            return Vector(1, 1, 1);
        }
        double dist = maybe_inter->GetDistance();
        inter_mask->value()[i][j] = true;
        return Vector(dist, dist, dist);
    }
    if (!maybe_inter) {
        return Vector();
    }
    if (render_options.mode == RenderMode::kNormal) {
        return (maybe_inter->GetNormal() + 1) / 2;
    }

    Vector result;
    // shift intersection position, so next rays won't intersect with current object
    Vector eps_pos_out = maybe_inter->GetPosition() + geom::kEPS * maybe_inter->GetNormal();
    Vector eps_pos_in = maybe_inter->GetPosition() - geom::kEPS * maybe_inter->GetNormal();
    // Calc Diffuse and Specular terms
    if (material->albedo[0] > geom::kEPS) {
        for (const Light& light : scene.GetLights()) {
            result += CalcDiffSpecular(light, scene, material, eps_pos_out, ray.GetDirection(),
                                       maybe_inter->GetNormal());
        }
    }

    result = material->ambient_color + material->intensity + result * material->albedo[0];
    if (depth >= render_options.depth) {
        return result;
    }
    // recursive Reflect
    if (material->albedo[1] > geom::kEPS && !inside) {
        Ray reflect_ray(eps_pos_out, Reflect(ray.GetDirection(), maybe_inter->GetNormal()));
        result += material->albedo[1] * TraceRay(reflect_ray, scene, render_options, depth + 1);
    }
    // recurcive Refract
    if (material->albedo[2] > geom::kEPS) {
        double eta = material->refraction_index;
        if (!inside) {
            eta = 1. / eta;
        }
        auto maybe_refract_dir = Refract(ray.GetDirection(), maybe_inter->GetNormal(), eta);
        if (maybe_refract_dir) {
            Ray refract_ray(eps_pos_in, *maybe_refract_dir);
            Vector refract_res = TraceRay(refract_ray, scene, render_options, depth + 1, !inside);
            if (!inside) {
                refract_res *= material->albedo[2];
            }
            result += refract_res;
        }
    }
    return result;
}