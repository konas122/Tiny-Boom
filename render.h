#ifndef __RENDER_H__
#define __RENDER_H__

#define _USE_MATH_DEFINES

#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "geometry.h"

using std::string;

const float sphere_radius = 1.5;
// const float noise_amplitude = 2.00;


template <typename T> inline T lerp(const T& v0, const T& v1, float t) {
    return v0 + (v1 - v0) * std::max(0.f, std::min(1.f, t));
}


float hash(const float n) {
    float x = sin(n) * 43758.5453f;
    return x - floor(x);
}


float noise(const Vec3f& x) {
    Vec3f p(floor(x.x), floor(x.y), floor(x.z));
    Vec3f f(x.x - p.x, x.y - p.y, x.z - p.z);
    f = f * (f * (Vec3f(3.f, 3.f, 3.f) - f * 2.f));
    float n = p * Vec3f(1.f, 57.f, 113.f);
    return lerp(lerp(
                     lerp(hash(n +  0.f), hash(n +  1.f), f.x),
                     lerp(hash(n + 57.f), hash(n + 58.f), f.x), f.y),
                lerp(
                    lerp(hash(n + 113.f), hash(n + 114.f), f.x),
                    lerp(hash(n + 170.f), hash(n + 171.f), f.x), f.y), f.z);
}


Vec3f rotate(const Vec3f& v) {
    return Vec3f(Vec3f(0.00, 0.80, 0.60) * v, 
                 Vec3f(-0.80, 0.36, -0.48) * v, 
                 Vec3f(-0.60, -0.48, 0.64) * v
            );
}


float fractal_brownian_motion(const Vec3f& x) {
    // this is a bad noise function with lots of artifacts. 
    // TODO: find a better one
    Vec3f p = rotate(x);
    float f = 0;
    f += 0.5000 * noise(p); p = p * 2.32;
    f += 0.2500 * noise(p); p = p * 3.03;
    f += 0.1250 * noise(p); p = p * 2.61;
    f += 0.0625 * noise(p);
    return f / 0.9375;
}


Vec3f palette_fire(const float d) { 
    // simple linear gradent yellow-orange-red-darkgray-gray. d is supposed to vary from 0 to 1
    const Vec3f   yellow(1.7, 1.3, 1.0); 
    const Vec3f   orange(1.0, 0.6, 0.0);
    const Vec3f      red(1.0, 0.0, 0.0);
    const Vec3f darkgray(0.2, 0.2, 0.2);
    const Vec3f     gray(0.4, 0.4, 0.4);
    
    // note that the color is "hot", i.e. has components >1
    float x = std::max(0.f, std::min(1.f, d));
    if (x < .25f)
        return lerp(gray, darkgray, x * 4.f);
    else if (x < .5f)
        return lerp(darkgray, red, x * 4.f - 1.f);
    else if (x < .75f)
        return lerp(red, orange, x * 4.f - 2.f);
    return lerp(orange, yellow, x * 4.f - 3.f);
}


float signed_distance(const Vec3f& p, float noise_amplitude) {
    // this function defines the implicit surface we render

    // Vec3f s = Vec3f(p).normalize(sphere_radius);
    // float displacement = sin(16 * p.x) * sin(16 * p.y) * sin(16 * p.z) * noise_amplitude;
    float displacement = -fractal_brownian_motion(p * 3.4) * noise_amplitude;
    return p.norm() - (sphere_radius + displacement);
}


bool sphere_trace(const Vec3f& orig, const Vec3f& dir, Vec3f& pos, float noise_amplitude) {
    if (orig * orig - pow(orig * dir, 2) > pow(sphere_radius, 2)) return false; // early discard

    pos = orig;
    // this loop simply slide along the ray
    for (size_t i = 0; i < 128; i++)
    {
        float d = signed_distance(pos, noise_amplitude);
        if (d < 0)
            return true;
        pos = pos + dir * std::max(d * 0.1f, .01f);
    }
    return false;
}


Vec3f distance_field_normal(const Vec3f& pos, float noise_amplitude) {
    // simple finite differences, very sensitive to the choice of the eps constant
    const float eps = 0.1;
    float d = signed_distance(pos, noise_amplitude);
    float nx = signed_distance(pos + Vec3f(eps, 0, 0), noise_amplitude) - d;
    float ny = signed_distance(pos + Vec3f(0, 0, eps), noise_amplitude) - d;
    float nz = signed_distance(pos + Vec3f(0, 0, eps), noise_amplitude) - d;
    return Vec3f(nx, ny, nz).normalize();
}


void render(int frame, float noise_amplitude, int height, int width, float fov) {
    
    std::vector<Vec3f> framebuffer(width * height);

#pragma omp parallel for
    for (size_t j = 0; j < height; j++) {   // actual rendering loop
    	std::cerr << "\r  Scanlines remaining: " << height - 1 - j << ' ' << std::flush;
        for (size_t i = 0; i < width; i++) {
            float dir_x = (i + 0.5) - width / 2;
            float dir_y = -(j + 0.5) + height / 2.; // this flips the image at the same time
            float dir_z = -height / (2. * tan(fov / 2.));
            Vec3f hit;
            if (sphere_trace(Vec3f(0, 0, 3), Vec3f(dir_x, dir_y, dir_z).normalize(), hit, noise_amplitude)) { 
                // the camera is placed to (0,0,3) and it looks along the -z axis
                float noise_level = (sphere_radius - hit.norm()) / noise_amplitude;
                Vec3f light_dir = (Vec3f(10, 10, 10) - hit).normalize();    // one light is placed to (10,10,10)
                float light_intensity = std::max(0.4f, light_dir * distance_field_normal(hit, noise_amplitude));
                framebuffer[i + j * width] = palette_fire((-.2 + noise_level) * 2) * light_intensity;
            } else {
                framebuffer[i + j * width] = Vec3f(0.2, 0.7, 0.8);  // background color
            }
        }
    }

    string str1 = "./frames/out";
    string str2 = std::to_string(10000 + frame);
    str1 += (str2 + ".ppm");

    std::ofstream ofs(str1, std::ios::binary);   // save the framebuffer to file
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i)
        for (size_t j = 0; j < 3; j ++)
            ofs << (char)(std::max(0, std::min(255, static_cast<int>(255 * framebuffer[i][j]))));
    ofs.close();

    // std::cerr << "\nDone.\n";
}

#endif
