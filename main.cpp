#include <stdio.h>
#include <algorithm>

#include <stdexcept>
#include <chrono>
#include <thread>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "ray_tracing_lib.h"

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 400

SDL_Renderer *renderer;
SDL_Window *window;

void clear_frame(SDL_Surface *surface, SDL_Surface *canvas, uint32_t *pixels) {
    SDL_LockSurface(canvas);

    RayHitInfo hit;
    for (int x = 0; x < WINDOW_WIDTH; x++) {
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            pixels[x + y * WINDOW_WIDTH] = 0x00000000;
        }
    }

    SDL_memcpy(canvas->pixels, pixels, WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(uint32_t));
    SDL_UnlockSurface(canvas);
    SDL_BlitSurface(canvas, 0, surface, 0);

    SDL_UpdateWindowSurface(window);
}

uint32_t add_color_channels(uint32_t a, uint32_t b) {
    uint32_t result { 0 };

    for (int i = 0; i < 4; i++) {
        uint32_t channelA = (a >> (i * 8)) & static_cast<uint32_t>(0xFF);
        uint32_t channelB = (b >> (i * 8)) & static_cast<uint32_t>(0xFF);

        uint32_t addition = channelA + channelB;
        addition = std::min(addition, static_cast<uint32_t>(255));

        result = result | (addition << (i * 8));
    }

    return result;
}

vec3 channels_to_vec(uint32_t channels) {
    return vec3( ((channels >> 24) & 0xFF) / 255.f, ((channels >> 16) & 0xFF) / 255.f, ((channels >> 8) & 0xFF) / 255.f );
}

uint32_t vec_to_channels(vec3 vec) {
    return static_cast<uint32_t>(std::min(1.f, vec.r) * 255.f) << 24 | static_cast<uint32_t>(std::min(1.f, vec.g) * 255.f) << 16 | static_cast<uint32_t>(std::min(1.f, vec.b) * 255.f) << 8 | 255 << 0;
}

float renderedFrames{ 0 };
void redraw_frame(SDL_Surface *surface, SDL_Surface *canvas, uint32_t *pixels, uint32_t *oldPixels, Camera& camera, Scene& scene) {

    // clearing the new pixel buffer
    #pragma omp parallel for
    for (int x = 0; x < WINDOW_WIDTH; x++) {
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            pixels[x + y * WINDOW_WIDTH] = 0;
        }
    }

    // calculating all rays
    RayHitInfo hit;
    #pragma omp parallel for
    for (int i = 0; i < RAYS_PER_PIXEL; i++) {
        // SDL_LockSurface(canvas);

        for (int x = 0; x < WINDOW_WIDTH; x++) {
            for (int y = 0; y < WINDOW_HEIGHT; y++) {
                // pixels[x + y * WINDOW_WIDTH] = add_color_channels(
                //     pixels[x + y * WINDOW_WIDTH],
                //     encode_color(
                //         trace_ray({ camera.position, camera_ray_direction(x, y, camera) }, scene)
                //             / (float) RAYS_PER_PIXEL
                //     )
                // );

                float weight = 1.f / (RAYS_PER_PIXEL);
                pixels[x + y * WINDOW_WIDTH] = 
                    vec_to_channels(
                        channels_to_vec(pixels[x + y * WINDOW_WIDTH]) + 
                        trace_ray({ camera.position, camera_ray_direction(x, y, camera) }, scene) * weight
                    );
            }
            // printf("\rstatus: %.2f %%                 ", (float)x / WINDOW_WIDTH * 100.f);
        }

        // SDL_memcpy(canvas->pixels, pixels, WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(uint32_t));
        // SDL_UnlockSurface(canvas);
        // SDL_BlitSurface(canvas, 0, surface, 0);
        // SDL_UpdateWindowSurface(window);
    }

    // adding to old frame
    #pragma omp parallel for
    for (int x = 0; x < WINDOW_WIDTH; x++) {
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            float weight = 1.f / (renderedFrames + 1.f);
            pixels[x + y * WINDOW_WIDTH] = 
                vec_to_channels(
                    channels_to_vec(oldPixels[x + y * WINDOW_WIDTH]) * (1.f - weight) + 
                    channels_to_vec(pixels[x + y * WINDOW_WIDTH]) * weight
                );
        }
    }

    SDL_memcpy(canvas->pixels, pixels, WINDOW_HEIGHT * WINDOW_WIDTH * sizeof(uint32_t));
    SDL_UnlockSurface(canvas);
    SDL_BlitSurface(canvas, 0, surface, 0);
    SDL_UpdateWindowSurface(window);

    renderedFrames++;
}

int main() {
    // initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "RayTracing Test");

    SDL_Event event;
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);

    uint32_t** pixels = new uint32_t*[2];
    pixels[0] = new uint32_t[WINDOW_HEIGHT * WINDOW_WIDTH];
    pixels[1] = new uint32_t[WINDOW_HEIGHT * WINDOW_WIDTH];
    uint32_t bufferIndex{ 0 };
    uint32_t nextBufferIndex{ 1 };

    Camera camera{ vec3(0, 0, 0), vec3(1, 0, 0), WINDOW_WIDTH, WINDOW_HEIGHT };
    camera.up = vec3(0, 0, 1);
    camera.right = vec3(0, 1, 0);

    Scene scene;

    // RayTracingMaterial firstMat  { vec3(1, 1, 1), vec3(0), 0, 0.3f, 1.0f };
    // RayTracingMaterial secondMat { vec3(1, 1, 1), vec3(0), 0, 0.6f, 1.0f };
    // RayTracingMaterial thirdMat  { vec3(1, 1, 1), vec3(0), 0, 1.0f, 1.0f };

    // RayTracingMaterial yellowMat    { vec3(1.0f, 0.5f, 0.1f), vec3(1.0f, 0.5f, 0.1f), 0.0f, 0.0f, 0.0f };
    // RayTracingMaterial emissiveMat  { vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 3.0f, 0.0f, 0.0f };

    // scene.objects.emplace_back(new Sphere { vec3(5, 3, 0), 1.f, firstMat });
    // scene.objects.emplace_back(new Sphere { vec3(5, 0, 0), 1.f, secondMat });
    // scene.objects.emplace_back(new Sphere { vec3(5, -3, 0), 1.f, thirdMat });

    // cornell box
    const float cornellBoxSize = 1.f;
    const float cornellCoord = cornellBoxSize / 2.f;
    {
        vec3 backTL{ cornellCoord, -cornellCoord, -cornellCoord };
        vec3 backTR{ cornellCoord, cornellCoord, -cornellCoord };
        vec3 backBL{ cornellCoord, -cornellCoord, cornellCoord };
        vec3 backBR{ cornellCoord, cornellCoord, cornellCoord };

        vec3 frontTL{ -cornellCoord, -cornellCoord, -cornellCoord };
        vec3 frontTR{ -cornellCoord, cornellCoord, -cornellCoord };
        vec3 frontBL{ -cornellCoord, -cornellCoord, cornellCoord };
        vec3 frontBR{ -cornellCoord, cornellCoord, cornellCoord };

        vec3 zeroV{ 1.f, 0.f, 0.f };

        RayTracingMaterial greenMat        { vec3(0.f, 1.f, 0.f), vec3(1.f, 1.f, 1.f), 0.0f, 0.0f, 0.0f };
        RayTracingMaterial redMat          { vec3(1.f, 0.f, 0.f), vec3(1.f, 1.f, 1.f), 0.0f, 0.0f, 0.0f };
        RayTracingMaterial whiteMat        { vec3(1.f, 1.f, 1.f), vec3(0.f, 0.f, 0.f), 0.0f, 0.0f, 0.0f };
        RayTracingMaterial glowingWhiteMat { vec3(1.f, 1.f, 1.f), vec3(1.f, 1.f, 1.f), 3.0f, 0.0f, 0.0f };

        // left side
        scene.objects.emplace_back(new Triangle { zeroV, backTL, backBL, frontBL, greenMat });
        scene.objects.emplace_back(new Triangle { zeroV, backTL, frontBL, frontTL, greenMat });

        // right side
        scene.objects.emplace_back(new Triangle { zeroV, backTR, backBR, frontBR, redMat });
        scene.objects.emplace_back(new Triangle { zeroV, backTR, frontBR, frontTR, redMat });

        // back side
        scene.objects.emplace_back(new Triangle { zeroV, backBL, backTL, backTR, whiteMat });
        scene.objects.emplace_back(new Triangle { zeroV, backBL, backBR, backTR, whiteMat });

        // bottom side
        scene.objects.emplace_back(new Triangle { zeroV, backBL, backBR, frontBL, whiteMat });
        scene.objects.emplace_back(new Triangle { zeroV, frontBL, frontBR, backBR, whiteMat });

        // top side
        scene.objects.emplace_back(new Triangle { zeroV, backTL, backTR, frontTL, glowingWhiteMat });
        scene.objects.emplace_back(new Triangle { zeroV, frontTL, frontTR, backTR, glowingWhiteMat });

        // sphere
        scene.objects.emplace_back(new Sphere { zeroV, .3f, RayTracingMaterial{ vec3(1.f, 1.f, 1.f), vec3(1.f, 1.f, 1.f), 0.f, 0.4f, 8.f } });
    }

    // scene.objects.emplace_back(new Sphere { vec3(5, 0, 55), 50.f, yellowMat });

    // scene.objects.emplace_back(new Triangle {vec3(5, 0, -1), vec3(0, 0, 0), vec3(3, -4, 3), vec3(3, 4, 3), secondMat });
    // scene.objects.emplace_back(new Sphere { vec3(5, 0, -8), 5.f, emissiveMat });

    clear_frame(surface, canvas, pixels[0]);
    clear_frame(surface, canvas, pixels[1]);

    // render loop
    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }

        SDL_PumpEvents();
        auto keyboard = SDL_GetKeyboardState(NULL);
        if (keyboard[SDL_SCANCODE_Q] == SDL_PRESSED)
            exit(0);
            
        if (keyboard[SDL_SCANCODE_R] == SDL_PRESSED) {
            auto lastTime = std::chrono::high_resolution_clock::now();

            redraw_frame(surface, canvas, pixels[nextBufferIndex], pixels[bufferIndex], camera, scene);

            std::swap(bufferIndex, nextBufferIndex);
    
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
            lastTime = currentTime;
            printf("\rdelta time: %lli milliseconds     ", deltaTime);
        }
    }

    // tidy up SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete[] pixels;

    return EXIT_SUCCESS;
}
