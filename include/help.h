#ifndef HELP_H
#define HELP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "camera.h"
#include "model.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Creates a simple ground plane
static inline Model temp_create_plane(void)
{
    Vertex ground_vertices[] = {
        {{-500.0f, 0.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 500.0f, 0.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {500.0f, 0.0f}},
        {{ 500.0f, 0.0f,  500.0f}, {0.0f, 1.0f, 0.0f}, {500.0f, 500.0f}},
        {{-500.0f, 0.0f,  500.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 500.0f}}
    };
    uint32_t ground_indices[] = {0, 1, 2, 2, 3, 0};

    Model ground;
    model_create(&ground,
                 ground_vertices, 4,
                 ground_indices, 6,
                 "resources/grass.jpg");

    return ground;
}

static inline Model temp_create_sphere(int sectorCount, int stackCount, float radius) {
    int vertex_count = (sectorCount + 1) * (stackCount + 1);
    int index_count = sectorCount * stackCount * 6;
    Vertex* vertices = malloc(sizeof(Vertex) * vertex_count);
    uint32_t* indices = malloc(sizeof(uint32_t) * index_count);
    int v = 0;
    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = M_PI / 2 - i * (M_PI / stackCount);
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * (2 * M_PI / sectorCount);
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;
            float s = (float)j / sectorCount;
            float t = (float)i / stackCount;
            vertices[v].position[0] = x;
            vertices[v].position[1] = y;
            vertices[v].position[2] = z;
            vertices[v].normal[0] = nx;
            vertices[v].normal[1] = ny;
            vertices[v].normal[2] = nz;
            vertices[v].texcoord[0] = s;
            vertices[v].texcoord[1] = t;
            v++;
        }
    }
    int idx = 0;
    for (int i = 0; i < stackCount; ++i) {
        for (int j = 0; j < sectorCount; ++j) {
            int k1 = i * (sectorCount + 1) + j;
            int k2 = k1 + sectorCount + 1;
            indices[idx++] = k1;
            indices[idx++] = k2;
            indices[idx++] = k1 + 1;
            indices[idx++] = k1 + 1;
            indices[idx++] = k2;
            indices[idx++] = k2 + 1;
        }
    }

    Model sphere;
    model_create(&sphere, vertices, vertex_count, indices, index_count, "mystical.png");
    free(vertices);
    free(indices);
    return sphere;
}

static inline Model temp_create_model(const char* path, const char* tex, ModelHashMap* cache) {
    Model mdl;

    if (path != NULL) {
        Model mdl = model_load(path, tex, cache);
        return mdl;
    }

    mdl = temp_create_sphere(32, 16, 0.5f);

    return mdl;


}

// Creates a pyramid mesh
static inline Model temp_create_pyramid(void)
{
    Vertex pyramid_vertices[] = {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.0f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}}
    };
    uint32_t pyramid_indices[] = {
        0, 4, 1,
        1, 4, 2,
        2, 4, 3,
        3, 4, 0
    };

    Model pyramid;
    model_create(&pyramid,
                 pyramid_vertices, 5,
                 pyramid_indices, 12,
                 "mystical.png");

    return pyramid;
}

// Creates a cube mesh
static inline Model temp_create_skybox(void)
{
    const float h = 700.0f;
    const float u0 = 0.0f;
    const float u1 = 0.25f;
    const float u2 = 0.50f;
    const float u3 = 0.75f;
    const float u4 = 1.00f;
    const float v0 = 0.0f;
    const float v1 = 1.0f / 3.0f;
    const float v2 = 2.0f / 3.0f;
    const float v3 = 1.0f;

    const float du = 0.5f / 1100.0f; //fix bug with edges
    const float dv = 0.5f / 825.0f;

    const float u0p = u0 + du;
    const float u1m = u1 - du;
    const float u1p = u1 + du;
    const float u2m = u2 - du;
    const float u2p = u2 + du;
    const float u3m = u3 - du;
    const float u3p = u3 + du;
    const float u4m = u4 - du;

    const float v0p = v0 + dv;
    const float v1m = v1 - dv;
    const float v1p = v1 + dv;
    const float v2m = v2 - dv;
    const float v2p = v2 + dv;
    const float v3m = v3 - dv;

    Vertex cube_vertices[] = {
        // -Z face (front)
        {{-h, -h, -h}, { 0.0f,  0.0f, -1.0f}, {u1p, v1p}},
        {{ h, -h, -h}, { 0.0f,  0.0f, -1.0f}, {u2m, v1p}},
        {{ h,  h, -h}, { 0.0f,  0.0f, -1.0f}, {u2m, v2m}},
        {{-h,  h, -h}, { 0.0f,  0.0f, -1.0f}, {u1p, v2m}},
        // +Z face (back)
        {{-h, -h,  h}, { 0.0f,  0.0f,  1.0f}, {u4m, v1p}},
        {{ h, -h,  h}, { 0.0f,  0.0f,  1.0f}, {u3p, v1p}},
        {{ h,  h,  h}, { 0.0f,  0.0f,  1.0f}, {u3p, v2m}},
        {{-h,  h,  h}, { 0.0f,  0.0f,  1.0f}, {u4m, v2m}},
        // -X face (left)
        {{-h, -h, -h}, {-1.0f,  0.0f,  0.0f}, {u1m, v1p}},
        {{-h,  h, -h}, {-1.0f,  0.0f,  0.0f}, {u1m, v2m}},
        {{-h,  h,  h}, {-1.0f,  0.0f,  0.0f}, {u0p, v2m}},
        {{-h, -h,  h}, {-1.0f,  0.0f,  0.0f}, {u0p, v1p}},
        // +X face (right)
        {{ h, -h, -h}, { 1.0f,  0.0f,  0.0f}, {u2p, v1p}},
        {{ h,  h, -h}, { 1.0f,  0.0f,  0.0f}, {u2p, v2m}},
        {{ h,  h,  h}, { 1.0f,  0.0f,  0.0f}, {u3m, v2m}},
        {{ h, -h,  h}, { 1.0f,  0.0f,  0.0f}, {u3m, v1p}},
        // -Y face (bottom)
        {{-h, -h, -h}, { 0.0f, -1.0f,  0.0f}, {u1p, v1m}},
        {{ h, -h, -h}, { 0.0f, -1.0f,  0.0f}, {u1p, v0p}},
        {{ h, -h,  h}, { 0.0f, -1.0f,  0.0f}, {u2m, v0p}},
        {{-h, -h,  h}, { 0.0f, -1.0f,  0.0f}, {u2m, v1m}},
        // +Y face (top)
        {{-h,  h, -h}, { 0.0f,  1.0f,  0.0f}, {u1p, v2p}},
        {{ h,  h, -h}, { 0.0f,  1.0f,  0.0f}, {u2m, v2p}},
        {{ h,  h,  h}, { 0.0f,  1.0f,  0.0f}, {u2m, v3m}},
        {{-h,  h,  h}, { 0.0f,  1.0f,  0.0f}, {u1p, v3m}}
    };

    uint32_t cube_indices[] = {
        0,1,2,2,3,0,
        4,5,6,6,7,4,
        8,9,10,10,11,8,
        12,13,14,14,15,12,
        16,17,18,18,19,16,
        20,21,22,22,23,20
    };

    Model cube;
    model_create(&cube,
                 cube_vertices, 24,
                 cube_indices, 36,
                 "resources/sky.png");
    cube.use_lighting = false;
    return cube;
}


static inline Model temp_create_cube(void)
{
    Vertex cube_vertices[] = {
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}
    };

    uint32_t cube_indices[] = {
        0,1,2,2,3,0,
        4,5,6,6,7,4,
        8,9,10,10,11,8,
        12,13,14,14,15,12,
        16,17,18,18,19,16,
        20,21,22,22,23,20
    };

    Model cube;
    model_create(&cube,
                 cube_vertices, 24,
                 cube_indices, 36,
                 "golf.jpeg");

    return cube;
}





#endif // HELP_H