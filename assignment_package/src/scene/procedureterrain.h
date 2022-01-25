#ifndef PROCEDURETERRAIN_H
#define PROCEDURETERRAIN_H

#include <glm_includes.h>

class ProcedureTerrain {
public:
    // constructor
    ProcedureTerrain();
    ~ProcedureTerrain();

    static int getHeight(int x, int z, int* isGrass);
    static float grasslandCoord(float x, float z);
    static float mountainCoord(float x, float z);
    static float fractalNoise2D(float x, float z, float persistence);
    static float perlinNoise2D(glm::vec2 xz);
    static float surflets(glm::vec2 point, glm::vec2 pointOnGrid);
    static float worleyNoise2D(float x, float z);

    static float perlinNoise3D(glm::vec3 xyz);
    static float surflet3D(glm::vec3 point, glm::vec3 pointOnGrid);
    static float fractalNoise3D(glm::vec3 xyz, float persistence);
};

#endif // PROCEDURETERRAIN_H
