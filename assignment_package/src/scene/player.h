#pragma once
#include "entity.h"
#include "camera.h"
#include "terrain.h"
#include "iostream"
#include "math.h"

using namespace std;

class Player : public Entity {
private:
    glm::vec3 m_velocity, m_acceleration;
    Camera m_camera;
    const Terrain &mcr_terrain;

    void processInputs(InputBundle &inputs);
    void computePhysics(float dT, const Terrain &terrain, InputBundle &inputs);
    // MM1
    int interfaceAxis;
    bool isOnGround(const Terrain &terrain, InputBundle &inputs);
    bool gridMarch(glm::vec3 rayOrigin,
                   glm::vec3 rayDirection,
                   const Terrain &terrain,
                   float *out_dist,
                   glm::ivec3 *out_blockHit /*int vec3*/);
    glm::vec3 avoidCollision(glm::vec3 rayDirection, const Terrain &terrain, int coordinate);

public:
    // MM1
    float velocity_amount = 3.f;
    float acceleration_amount = 1.f; // a basic scalar acceleration
    float jump_amount = 10.f;
    float flightHeight = 150.f;
    float G = 50.f; // gravity acceleration
    BlockType addBlock(Terrain *terrain, BlockType currBlockType);
    BlockType removeBlock(Terrain *terrain);

    // Readonly public reference to our camera
    // for easy access from MyGL
    const Camera& mcr_camera;

    Player(glm::vec3 pos, const Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &inputs) override;

    // Player overrides all of Entity's movement
    // functions so that it transforms its camera
    // by the same amount as it transforms itself.
    void moveAlongVector(glm::vec3 dir) override;
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    // For sending the Player's data to the GUI
    // for display
    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;

    BlockType get_camera_block(const Terrain &terrain);
};
