#include <assert.h>
#include <stdio.h>

#include "SDL.h"

#include "Color.h"
#include "Ray.h"
#include "Sphere.h"
#include "Timer.h"
#include "Vec3.h"

#define AA 1

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array[0])))

const u32 WIDTH = 1280;
const u32 HEIGHT = 720;
const u32 NUM_THREADS = 24;

struct thread_data {
    u32 id;
    const SDL_Surface *surface;
    u32 startY;
    u32 numY;
    u32 numRays;
};

struct render_pool {
    render_pool(const SDL_Surface *surface, LPTHREAD_START_ROUTINE function) {
        assert(HEIGHT % NUM_THREADS == 0);
        const u32 numY = HEIGHT / NUM_THREADS;
        for (u32 i = 0; i < NUM_THREADS; ++i) {
            data[i].id = i;
            data[i].surface = surface;
            data[i].startY = i * numY;
            data[i].numY = numY;
            data[i].numRays = 0;
            threadHandles[i] = CreateThread(NULL, 0, function, &data[i], CREATE_SUSPENDED, &threadIds[i]);
            if (threadHandles[i] == NULL) {
                fprintf(stderr, "Error creating thread %d because %d", i, GetLastError());
            }
        }
    }

    u32 traceAllRays() {
        for (u32 i = 0; i < NUM_THREADS; ++i) {
            if (ResumeThread(threadHandles[i]) < 0) {
                fprintf(stderr, "Error resuming thread %d because %d", i, GetLastError());
            }
        }

        WaitForMultipleObjects(NUM_THREADS, threadHandles, TRUE, INFINITE);

        u32 totalRays = 0;
        for (u32 i = 0; i < NUM_THREADS; ++i) {
            totalRays += data[i].numRays;
            CloseHandle(threadHandles[i]);
        }

        return totalRays;
    }

private:
    thread_data data[NUM_THREADS];
    DWORD       threadIds[NUM_THREADS];
    HANDLE      threadHandles[NUM_THREADS];
};




struct point_light {
    point_light(const Vec3 &p, const Color &c) : p(p), c(c) {}

    Vec3 p;
    Color c;
};

bool messageLoop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        }
    }
    return true;
}

const Sphere spheres[] = { 
    Sphere(Vec3(-1.0, 0, -1.5), .60, Color::RED), 
    Sphere(Vec3(0, 0, -1), .75, Color::GREEN), 
    Sphere(Vec3(1.0, 0, -1.5), .60, Color::BLUE) 
};
const u32 numSpheres = ARRAY_LENGTH(spheres);

bool trace(const Ray & r, float *t, const Sphere **s) {
    float minT = FLT_MAX;
    const Sphere *minSphere = NULL;

    for (u32 s = 0; s < numSpheres; ++s) {
        const Sphere &sphere = spheres[s];
        float newT = sphere.intersect(r);
        if (newT < minT) {
            minT = newT;
            minSphere = &sphere;
        }
    }

    if (t != NULL) {
        *t = minT;
        *s = minSphere;
    }

    return minT != FLT_MAX;
}

void setPixel(const SDL_Surface * surface, u32 x, u32 y, u32 pixel) {
    u32 *target_pixel = (u32 *)((u8 *)surface->pixels + y * surface->pitch + x * sizeof(u32));
    *target_pixel = pixel;
}

DWORD WINAPI cast_ray(LPVOID lpParam) {
    //timer t;
    //t.start();

    thread_data *data = (thread_data *)lpParam;
    u32 numRays = 0;

    const float FOV = M_PI / 2.0;
    const float tanFov2 = tan(FOV / 2.0);
    const float aspectRatio = (float)WIDTH / HEIGHT;
    const float aspectTimesTanFov = aspectRatio * tanFov2;

    const point_light lights[] = { point_light(Vec3(3, 5, 1), Color::WHITE) };
    const u32 numLights = ARRAY_LENGTH(lights);

#if AA
    const float offsetsX[] = { 0.2, 0.4, 0.6, 0.8 };
    const float offsetsY[] = { 0.6, 0.2, 0.8, 0.4 };
#else
    const float offsetsX[] = { 0.5 };
    const float offsetsY[] = { 0.5 };
#endif
    const u32 numOffsets = ARRAY_LENGTH(offsetsX);
    const float invNumOffsets = 1.0 / numOffsets;

    for (u32 y = data->startY; y < data->startY + data->numY; ++y) {
        for (u32 x = 0; x < WIDTH; ++x) {
            Color c = Color::BLACK;

            for (u32 o = 0; o < numOffsets; ++o) {
                const float dirX = (2.0 * (x + offsetsX[o]) / WIDTH - 1.0) * aspectTimesTanFov;
                const float dirY = (1.0 - 2.0 * (y + offsetsY[o]) / HEIGHT) * tanFov2;

                Vec3 dir(dirX, dirY, -1);
                dir.normalize();
                Ray r(Vec3::ZERO, dir);

                ++numRays;

                float minT = 0;
                const Sphere *s = NULL;
                if (trace(r, &minT, &s) == true) {
                    Vec3 hitPoint = r.pointAt(minT);
                    for (u32 l = 0; l < numLights; ++l) {
                        const point_light &light = lights[l];
                        Vec3 toLight = light.p - hitPoint;
                        const float distanceToLightSquared = toLight.dot(toLight);
                        toLight.normalize();
                        Vec3 normal = hitPoint - s->o;
                        normal.normalize();
                        Ray lightRay(hitPoint + 1e-6 * normal, toLight);

                        float minLightT = 0;
                        const Sphere *lightSphere = NULL; // don't care

                        ++numRays;
                        bool shadowed = trace(lightRay, &minLightT, &lightSphere) == true && minLightT * minLightT < distanceToLightSquared;
                        if (shadowed == false) {
                            const float ndotl = normal.dot(toLight);
                            if (ndotl > 0) {
                                assert(ndotl <= 1.0);
                                c += s->c * light.c * ndotl;
                            }
                        }
                    }
                }
            }

            c *= invNumOffsets;
            const u32 u32Color = c.toU32();
            setPixel(data->surface, x, y, u32Color);
        }
    }

    data->numRays = numRays;

    //t.stop();
    //fprintf(stdout, "#%d complete in %.3fms\n", data->id, t.elapsedInMs());

    return 0;
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Unable to init SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "pew",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == nullptr) {
        fprintf(stderr, "Unable to create window: %s", SDL_GetError());
        return 1;
    }

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    if (renderer == nullptr) {
        fprintf(stderr, "Unable to create renderer: %s", SDL_GetError());
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);

    render_pool pool(surface, cast_ray);

    Timer t;
    t.start();
    const u32 numRays = pool.traceAllRays();
    t.stop();
    float ms = t.elapsedInMs();

    fprintf(stdout, "%ur in %0.3fms = %.3fmrps", numRays, ms, numRays / ms * 1e-3);

    SDL_UpdateWindowSurface(window);

    while (messageLoop())
        ;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}