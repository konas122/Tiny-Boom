#include <iostream>
#include "render.h"


const int height = 720;
const int width = height * 1;
const float fov = M_PI / 3;

int main()
{
    float noise_amplitude = 1.00;
    float interval = 1.000;
    for (int i = 0; i < 1.00 / interval; i++) {
        std::cerr << "\nrendering frame: " << i + 1 << "\n";
        render(i, noise_amplitude, height, width, fov);
        noise_amplitude += interval;
    }
    return 0;
}

