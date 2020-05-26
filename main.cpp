#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// for QPC
#include <windows.h>

#include "SDL.h"

#define AA 1

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array[0])))

typedef uint8_t u8;
typedef uint32_t u32;

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

struct timer {
    timer() {
        QueryPerformanceFrequency(&frequency);
    }

    void start() {
        QueryPerformanceCounter(&begin);
    }

    void stop() {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        elapsed.QuadPart = (1000000 * (end.QuadPart - begin.QuadPart)) / frequency.QuadPart;
    }

    float elapsedInMs() const {
        return 1e-3 * elapsed.QuadPart;
    }

    float elapsedInUs() const {
        return elapsed.QuadPart;
    }

private:
    LARGE_INTEGER begin, elapsed, frequency;
};

float quadratic(const float a, const float b, const float c) {
    float x = FLT_MAX;
    float disc = b * b - 4 * a * c;
    if (disc == 0) {
        x = -0.5 * b / a;
    }
    else if (disc > 0) {
        float q = (b > 0) ? -0.5 * (b - sqrt(disc)) : -0.5 * (b + sqrt(disc));
        float x0 = q / a;
        float x1 = c / q;
        if (x0 < x1 && x0 >= 0) {
            x = x0;
        }
        else if (x1 >= 0) {
            x = x1;
        }
    }
    return x;
}

struct v3 {
    v3(float x, float y, float z) : x(x), y(y), z(z) { }

    void normalize() {
        float invLen = 1.0 / length();
        if (invLen > 0) {
            x *= invLen;
            y *= invLen;
            z *= invLen;
        }
    }

    float length() const {
        return sqrt(x * x + y * y + z * z);
    }

    float dot(const v3 &v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    v3 operator+(const v3 &b) const {
        return v3(x + b.x, y + b.y, z + b.z);
    }

    v3 operator-(const v3 &b) const {
        return v3(x - b.x, y - b.y, z - b.z);
    }

    float x;
    float y;
    float z;
};

v3 operator*(const float s, const v3 & rhs) {
    return v3(rhs.x * s, rhs.y * s, rhs.z * s);
}

struct ray {
    ray(const v3 &o, const v3 &d) : o(o), d(d) { }

    v3 pointAt(const float t) {
        return o + t * d;
    }

    v3 o;
    v3 d;
};

struct color {
    color(const float a, const float r, const float g, const float b) : a(a), r(r), g(g), b(b) {}

    color &operator+=(const color &rhs) {
        a += rhs.a;
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }

    color &operator*=(const float s) {
        a *= s;
        r *= s;
        g *= s;
        b *= s;
        return *this;
    }

    color operator*(const color &rhs) const {
        return color(a * rhs.a, r * rhs.r, g * rhs.g, b * rhs.b);
    }

    color operator*(const float s) const {
        return color(a * s, r * s, g * s, b * s);
    }

    u32 toU32() const {
        return (((u32)(255 * a) << 24) | ((u32)(255 * r) << 16) | ((u32)(255 * g) << 8) | (u32)(255 * b));
    }

    float a;
    float r;
    float g;
    float b;
};

struct point_light {
    point_light(const v3 &p, const color &c) : p(p), c(c) {}

    v3 p;
    color c;
};

const color BLACK = color(1.0, 0.0, 0.0, 0.0);
const color WHITE = color(1.0, 1.0, 1.0, 1.0);
const color RED = color(1.0, 1.0, 0.0, 0.0);
const color GREEN = color(1.0, 0.0, 1.0, 0.0);
const color BLUE = color(1.0, 0.0, 0.0, 1.0);

struct sphere {
    sphere(const v3 &o, const float r, const color &c) : o(o), r(r), c(c) {}

    float intersect(const ray &r) const {
        v3 l = r.o - o;
        float a = r.d.dot(r.d);
        float b = 2 * r.d.dot(l);
        float c = l.dot(l) - this->r * this->r;
        return quadratic(a, b, c);
    }

    const v3 o;
    const float r;
    const color c;
};


const v3 ZERO = v3(0, 0, 0);

bool messageLoop() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return false;
        }
    }
    return true;
}

const sphere spheres[] = { sphere(v3(-1.0, 0, -1.5), .60, RED), sphere(v3(0, 0, -1), .75, GREEN), sphere(v3(1.0, 0, -1.5), .60, BLUE) };
const u32 numSpheres = ARRAY_LENGTH(spheres);

bool trace(const ray & r, float *t, const sphere * *s) {
    float minT = FLT_MAX;
    const sphere *minSphere = NULL;

    for (u32 s = 0; s < numSpheres; ++s) {
        const sphere &sphere = spheres[s];
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


    const point_light lights[] = { point_light(v3(3, 5, 1), WHITE) };
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
            color c = BLACK;

            for (u32 o = 0; o < numOffsets; ++o) {
                const float dirX = (2.0 * (x + offsetsX[o]) / WIDTH - 1.0) * aspectTimesTanFov;
                const float dirY = (1.0 - 2.0 * (y + offsetsY[o]) / HEIGHT) * tanFov2;

                v3 dir = v3(dirX, dirY, -1);
                dir.normalize();
                ray r = ray(ZERO, dir);

                ++numRays;

                float minT = 0;
                const sphere *s = NULL;
                if (trace(r, &minT, &s) == true) {
                    v3 hitPoint = r.pointAt(minT);
                    for (u32 l = 0; l < numLights; ++l) {
                        const point_light &light = lights[l];
                        v3 toLight = light.p - hitPoint;
                        const float distanceToLightSquared = toLight.dot(toLight);
                        toLight.normalize();
                        v3 normal = hitPoint - s->o;
                        normal.normalize();
                        ray lightRay = ray(hitPoint + 1e-6 * normal, toLight);

                        float minLightT = 0;
                        const sphere *lightSphere = NULL; // don't care

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

    timer t;
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