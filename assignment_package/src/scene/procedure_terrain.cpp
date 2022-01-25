#include "procedureterrain.h"
#include <glm_includes.h>
#include <iostream>

ProcedureTerrain::ProcedureTerrain()
{}

ProcedureTerrain::~ProcedureTerrain()
{}

int ProcedureTerrain::getHeight(int x, int z, int* isGrass) {
    float grassland = grasslandCoord(x, z);
    float mountain = mountainCoord(x, z);

    float perlin = (fractalNoise2D(x / 2048.f, z / 2048.f, 0.2) + 1) / 2;
    float smoothPerlin = glm::smoothstep(0.5, 0.6, (double) perlin);
    if (smoothPerlin < 0.5) {
        *isGrass = 1;
    } else {
        *isGrass = 0;
    }
    return glm::clamp(glm::mix(grassland, mountain, smoothPerlin), 0.f, 255.f);
}

float ProcedureTerrain::grasslandCoord(float x, float z) {
    int grasslandLower = 128;
    int grasslandUpper = 160;

    float noise = worleyNoise2D(fractalNoise2D(x / 512, z / 512, 0.5), fractalNoise2D(z / 512, x / 512, 0.5));
    return grasslandLower + (grasslandUpper - grasslandLower) * noise;

}

float ProcedureTerrain::mountainCoord(float x, float z) {
    int mountainLower = 160;
    int mountainUpper = 250;

    float noise = glm::abs(fractalNoise2D(x / 4096, z / 4096, 0.92));

    return mountainLower + (mountainUpper - mountainLower) * noise;
}

float ProcedureTerrain::fractalNoise2D(float x, float z, float persistence) {
    float value = 0.0;
    float scale = 1.0;
    float atten = 1.0;
    int octaves = 7;

    for (int i = 0; i < octaves; i++) {
        scale *= 2.0;
        atten *= pow(persistence, i);
        value += perlinNoise2D(glm::vec2(x * scale, z * scale)) * atten;
    }
    return value;
}

glm::vec2 pow(glm::vec2 base, int power) {
    glm::vec2 value = base;
    value.x = pow(base.x, power);
    value.y = pow(base.y, power);
    return value;
}

float ProcedureTerrain::perlinNoise2D(glm::vec2 xz) {
    float sum = 0.0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            sum += surflets(xz, glm::floor(xz) + glm::vec2(i, j));
        }
    }
    return sum;
}

float ProcedureTerrain::surflets(glm::vec2 point, glm::vec2 pointOnGrid) {
    glm::vec2 d = glm::abs(point - pointOnGrid);
    glm::vec2 t = glm::vec2(1.f) - 6.f * pow(d, 5) + 15.f * pow(d, 4) - 10.f * pow(d, 3);
    glm::vec2 difference = point - pointOnGrid;

    pointOnGrid += 0.1;
    float x;
    float y;
    glm::mat2 primes;
    primes = glm::mat2{{126.1, 311.7}, {420.2, 1337.1}};
    x = 43758.5453;
    y = 789221.5453;
    glm::vec2 noise = glm::sin(pointOnGrid * primes);
    noise.x *= x;
    noise.y *= y;
    glm::vec2 gradient = glm::normalize(glm::abs(glm::fract(noise))) * 2.f - glm::vec2(1,1);

    float height = glm::dot(difference, gradient) * t.x * t.y;
    return height;
}

float ProcedureTerrain::worleyNoise2D(float x, float z) {
    int ix = int(x);
    int iz = int(z);
    float fracX = x - float(int(x));
    float fracZ = z - float(int(x));

    float minDist1 = 1;
    float minDist2 = 1;

    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            glm::vec2 neighborDirection = glm::vec2(j, i);
            float voronoi_x = glm::fract(glm::sin(glm::dot(glm::vec2(ix, iz) + neighborDirection, glm::vec2(127.1, 311.7))) * 43758.5453);
            float voronoi_z = glm::fract(glm::sin(glm::dot(glm::vec2(ix, iz) + neighborDirection, glm::vec2(420.2, 1337.1))) * 789221.1234);
            glm::vec2 neighborVoronoiCenter =  glm::vec2(voronoi_x, voronoi_z);
            glm::vec2 diff = neighborDirection + neighborVoronoiCenter - glm::vec2(fracX, fracZ);
            float dist = glm::length(diff);
            if (dist < minDist1) {
                minDist2 = minDist1;
                minDist1 = dist;
            } else if (dist < minDist2) {
                minDist2 = dist;
            }
        }
    }

    return minDist2 - minDist1;
}


float ProcedureTerrain::perlinNoise3D(glm::vec3 xyz) {
    float surfletSum = 0.f;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                surfletSum += surflet3D(xyz, glm::floor(xyz) + glm::vec3(i, j, k));
            }
        }
    }
    return surfletSum;
}

glm::vec3 pow3D(glm::vec3 base, int power) {
    glm::vec3 value = base;
    value.x = pow(base.x, power);
    value.y = pow(base.y, power);
    value.z = pow(base.z, power);
    return value;
}

glm::vec3 random3(glm::vec3 pointOnGrid) {
    float j = 4096.0 * sin(glm::dot(pointOnGrid, glm::vec3(17.0, 59.4, 15.0)));
    glm::vec3 r;
    r.z = glm::fract(512.0 * j);
    j *= .125;
    r.x = glm::fract(512.0 * j);
    j *= .125;
    r.y = glm::fract(512.0 * j);
    r = r - glm::vec3(0.5);
    return r;
}

float ProcedureTerrain::fractalNoise3D(glm::vec3 xyz, float persistence) {
    float value = 0.0;
    float scale = 1.0;
    float atten = 1.0;
    int octaves = 7;

    for (int i = 0; i < octaves; i++) {
        scale *= 2.0;
        atten *= pow(persistence, i);
        value += perlinNoise3D(xyz * scale) * atten;
    }
    return value;
}

float ProcedureTerrain::surflet3D(glm::vec3 point, glm::vec3 pointOnGrid) {
    glm::vec3 d = glm::abs(point - pointOnGrid);
    glm::vec3 t = glm::vec3(1.f) - 6.f * pow3D(d, 5) + 15.f * pow3D(d, 4) - 10.f * pow3D(d, 3);

    glm::vec3 difference = point - pointOnGrid;
    glm::vec3 gradient = random3(pointOnGrid) * 2.f - glm::vec3(1.f, 1.f, 1.f);
    float height = glm::dot(difference, gradient) * t.x * t.y * t.z;

    return height;
}
