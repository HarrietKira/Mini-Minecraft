#include "player.h"
#include <QString>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &inputs)
{
    processInputs(inputs);
    computePhysics(dT, mcr_terrain, inputs);
}

void Player::processInputs(InputBundle &inputs)
{
    // MM1
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.

    // compute acceleration vector based on a initial scalar
    // velocity is not computed here cuz it depends on dT
    // flight mode
    if(inputs.flight_mode)
    {
        float times = 10.f;
        if(inputs.wPressed)
        {
            m_acceleration = times*acceleration_amount*m_forward;
        }
        else if(inputs.sPressed)
        {
            m_acceleration = -acceleration_amount*times*m_forward;
        }
        else if(inputs.dPressed)
        {
            m_acceleration = acceleration_amount*times*m_right;
        }
        else if(inputs.aPressed)
        {
            m_acceleration = -acceleration_amount*times*m_right;
        }
        else if(inputs.ePressed)
        {
            m_acceleration = acceleration_amount*times*m_up;
        }
        else if(inputs.qPressed)
        {
            m_acceleration = -acceleration_amount*times *m_up;
        }
        else // no key pressed, keep static
        {
            m_velocity = glm::vec3(0.f);
            m_acceleration = glm::vec3(0.f);
        }
    }
    // normal on ground mode
    else
    {
        if(inputs.spacePressed && isOnGround(mcr_terrain, inputs))
        {
            m_velocity = glm::vec3(0.f, jump_amount, 0.f);
            m_acceleration = glm::vec3(0.f, -G, 0.f);
        }
        else if(inputs.wPressed)
        {
            m_velocity = velocity_amount*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));
            m_acceleration = acceleration_amount*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));
//            cout << m_acceleration.x <<" " << m_acceleration.y << " " << m_acceleration.z << endl;
        }
        else if(inputs.sPressed)
        {
            m_velocity = -velocity_amount*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));
            m_acceleration = -acceleration_amount*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));
        }
        else if(inputs.dPressed)
        {
            m_velocity = velocity_amount*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z));
            m_acceleration = acceleration_amount*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z));
        }
        else if(inputs.aPressed)
        {
            m_velocity = -velocity_amount*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z));
            m_acceleration = -acceleration_amount*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z));
        }
        else if(inputs.fPressed)
        {
            m_velocity = glm::vec3(0.f);
            m_acceleration = glm::vec3(0.f, -G, 0.f);
        }
        else
        {
            if(!isOnGround(mcr_terrain, inputs)) m_acceleration.y = -G;
            else
            {
                m_velocity = glm::vec3(0.f);
                m_acceleration = glm::vec3(0.f);
            }
        }
        if(!isOnGround(mcr_terrain, inputs)) m_acceleration.y = -G;
        else
        {
            m_acceleration.y = 0.f;
        }
    }
}

// update camera&position of player; detect collision before updating
void Player::computePhysics(float dT, const Terrain &terrain, InputBundle &inputs)
{
    // MM1
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    glm::vec3 rayDirection = glm::vec3(0.f);
    if(inputs.flight_mode)
    {
        // modify camera perpective
        if(inputs.ePressed || inputs.qPressed)
        {
            flightHeight += (0.95f * m_velocity + m_acceleration * dT).y;
        }
        rayDirection = glm::vec3(0.f, flightHeight-m_position.y, 0.f);
        moveAlongVector(rayDirection);

        if(!inputs.ePressed && !inputs.qPressed)
        {
            m_velocity = m_velocity * 0.95f; // add simulates and drag
            m_velocity = m_velocity + m_acceleration * dT; // v=v0+at

            rayDirection = m_velocity*dT;
            moveAlongVector(rayDirection*2.0f);
        }
    }
    else if(!inputs.flight_mode)
    {
        // perform whatever movement above ground
        rayDirection = 0.95f*m_velocity * dT + 0.5f*dT*dT*m_acceleration;
        m_velocity.y += dT*m_acceleration.y;
        rayDirection.x = avoidCollision(glm::vec3(rayDirection.x, 0.f, 0.f), terrain, 0).x;
        rayDirection.y = avoidCollision(glm::vec3(0.f, rayDirection.y, 0.f), terrain, 0).y;
        rayDirection.z = avoidCollision(glm::vec3(0.f, 0.f, rayDirection.z), terrain, 0).z;
        moveAlongVector(rayDirection);
    }
}

// check if character is on ground, create falling effect
bool Player::isOnGround(const Terrain &terrain, InputBundle &inputs)
{

    if (terrain.getBlockAt(m_position) != EMPTY && terrain.getBlockAt(m_position) != WATER && terrain.getBlockAt(m_position) != LAVA)
    {
        inputs.onGround = true;
    }
    else
    {
        inputs.onGround = false;
    }
    return inputs.onGround;
}

// on slide, help detect collision
bool Player::gridMarch(glm::vec3 rayOrigin,
                       glm::vec3 rayDirection,
                       const Terrain &terrain,
                       float *out_dist,
                       glm::ivec3 *out_blockHit /*int vec3*/)
{
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen)
    {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i)
        { // Iterate over the three axes
            if(rayDirection[i] != 0)
            { // Is ray parallel to axis i?
                // glm::sign returns 1.0 if x > 0, 0.0 if x == 0, or -1.0 if x < 0
                float offset = max(0.f, glm::sign(rayDirection[i]));
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f)
                {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t)
                {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1)
        {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0, 0, 0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY && cellType != WATER && cellType != LAVA)
        {
            *out_blockHit = currCell;
            *out_dist = min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = min(maxLen, curr_t);
    return false;
}

// avoid 0-vector rayDirection be sent inside
glm::vec3 Player::avoidCollision(glm::vec3 rayDirection, const Terrain &terrain, int coordinate)
{
    if(coordinate == 0 || coordinate == 2)
    {
        bool isCollided = false;
        if(glm::length(rayDirection)!=0)
        {
//            cout << "rayDirection before collision detection: " << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << endl;
//            cout << "rayDirection length before collision detection: " << glm::length(rayDirection) << endl;

//            glm::vec3 bottomLeftLowerVertex = m_position + glm::vec3(-0.5f, 0.f, 0.5f);
            glm::vec3 bottomLeftLowerVertex = m_position - 0.5f*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z))
                                                         - 0.5f*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));
            float distance = glm::length(rayDirection); // final distance that can move
            for (int y = 1; y <= 2; y++)
            {
                for (int z = 0; z <= 1; z++)
                {
                    for (int x = 0; x <= 1; x++)
                    {
                        float outDist = 0.f;
                        glm::ivec3 outBlockHit = glm::ivec3();

                        glm::vec3 rayOrigin = bottomLeftLowerVertex + x*1.f*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z))
                                                                    + y*1.f*glm::normalize(glm::vec3(0.f, 1.f, 0.f))
                                                                    + z*1.f*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));

                        // collide for current vertex
                        if (gridMarch(rayOrigin, rayDirection, terrain, &outDist, &outBlockHit))
                        {
                            distance = min(outDist, distance);
//                            cout << "m_position: " << m_position.x << ", " << m_position.y << ", " << m_position.z << endl;
//                            cout << "outBlockHit: " << outBlockHit.x << ", " << outBlockHit.y << ", " << outBlockHit.z << endl;
                            isCollided = true;
                        }
                    }
                }
            }

            distance = max(distance-0.0001f, 0.f);

            rayDirection = distance * glm::normalize(rayDirection);
//            cout << "collision test: " << isCollided << endl;
//            cout << "rayDirection after collision detection: " << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << endl;
//            cout << "rayDirection length after collision detection: " << glm::length(rayDirection) << endl;
//            cout << "----------------------------------------------------------" << endl;
            return rayDirection;
        }
        return rayDirection;
    }
    else
    {
        bool isCollided = false;
        if(glm::length(rayDirection)!=0)
        {
//            cout << "rayDirection before collision detection: " << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << endl;
//            cout << "rayDirection length before collision detection: " << glm::length(rayDirection) << endl;

            glm::vec3 bottomLeftLowerVertex = m_position - 0.5f*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z))
                                                         - 0.5f*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));

    //        glm::ivec3 outBlockHit = glm::ivec3();
            float distance = glm::length(rayDirection); // final distance that can move
    //        float outDist = 0.f; // intermediate distance get from grid march
            for (int y = 0; y <= 2; y++)
            {
                for (int z = 0; z >= -1; z--)
                {
                    for (int x = 0; x <= 1; x++)
                    {
                        if(y != 1)
                        {
                            float outDist = 0.f;
                            glm::ivec3 outBlockHit = glm::ivec3();

                            glm::vec3 rayOrigin = bottomLeftLowerVertex + x*1.f*glm::normalize(glm::vec3(m_right.x, 0.f, m_right.z))
                                                                        + y*1.f*glm::normalize(glm::vec3(0.f, 1.f, 0.f))
                                                                        + z*1.f*glm::normalize(glm::vec3(m_forward.x, 0.f, m_forward.z));
                            // collide for current vertex
                            if (gridMarch(rayOrigin, rayDirection, terrain, &outDist, &outBlockHit))
                            {
//                                outDist = max(0.f, outDist-0.00001f);

                                distance = min(outDist, distance);
//                                cout << "m_position: " << m_position.x << ", " << m_position.y << ", " << m_position.z << endl;
//                                cout << "outBlockHit: " << outBlockHit.x << ", " << outBlockHit.y << ", " << outBlockHit.z << endl;
                                isCollided = true;
                            }
                        }
                    }
                }
            }
            rayDirection = distance * glm::normalize(rayDirection);
//            cout << "collision test: " << isCollided << endl;
//            cout << "rayDirection after collision detection: " << rayDirection.x << ", " << rayDirection.y << ", " << rayDirection.z << endl;
//            cout << "rayDirection length after collision detection: " << glm::length(rayDirection) << endl;
//            cout << "----------------------------------------------------------" << endl;
            return rayDirection;
        }
        return rayDirection;
    }
}

BlockType Player::addBlock(Terrain *terrain, BlockType currBlockType)
{
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = 3.f * glm::normalize(this->m_forward);
    float outDist = 0.f;
    glm::ivec3 outBlockHit = glm::ivec3();

    if (gridMarch(rayOrigin, rayDirection, *terrain, &outDist, &outBlockHit))
    {
        // check forward
//        glm::vec3 normal_forward = glm::normalize(m_forward);
        BlockType blockType = terrain->getBlockAt(outBlockHit.x-1 , outBlockHit.y, outBlockHit.z);
        if (blockType == EMPTY)
        {

            terrain->setBlockAt(outBlockHit.x-1 , outBlockHit.y, outBlockHit.z, currBlockType);
            terrain->getChunkAt(outBlockHit.x-1, outBlockHit.z).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x-1, outBlockHit.z).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x-1, outBlockHit.z).get()->sendVBO();
            return currBlockType;
        }
        blockType = terrain->getBlockAt(outBlockHit.x+1 , outBlockHit.y, outBlockHit.z);
        if (blockType == EMPTY)
        {
            terrain->setBlockAt(outBlockHit.x+1 , outBlockHit.y, outBlockHit.z, currBlockType);
            terrain->getChunkAt(outBlockHit.x+1, outBlockHit.z).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x+1, outBlockHit.z).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x+1, outBlockHit.z).get()->sendVBO();
            return currBlockType;
        }
        // check up
        blockType = terrain->getBlockAt(outBlockHit.x, outBlockHit.y-1, outBlockHit.z);
        if (blockType == EMPTY)
        {
            terrain->setBlockAt(outBlockHit.x, outBlockHit.y-1, outBlockHit.z, currBlockType);
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->sendVBO();
            return currBlockType;
        }
        blockType = terrain->getBlockAt(outBlockHit.x, outBlockHit.y+1, outBlockHit.z);
        if (blockType == EMPTY)
        {
            terrain->setBlockAt(outBlockHit.x, outBlockHit.y+1, outBlockHit.z, currBlockType);
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->sendVBO();
            return currBlockType;
        }
        // check right
        blockType = terrain->getBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z-1);
        if (blockType == EMPTY)
        {
            terrain->setBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z-1, currBlockType);
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z-1).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z-1).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z-1).get()->sendVBO();
            return currBlockType;
        }
        blockType = terrain->getBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z+1);
        if (blockType == EMPTY)
        {
            terrain->setBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z+1, currBlockType);
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z+1).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z+1).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z+1).get()->sendVBO();
            return currBlockType;
        }
    }
    return EMPTY;
}

BlockType Player::removeBlock(Terrain *terrain)
{
    glm::vec3 rayOrigin = m_camera.mcr_position;
    glm::vec3 rayDirection = 3.f * glm::normalize(this->m_forward);
    float outDist = 0.f;
    glm::ivec3 outBlockHit = glm::ivec3();

    if (gridMarch(rayOrigin, rayDirection, *terrain, &outDist, &outBlockHit))
    {
        BlockType blockType = terrain->getBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z);
        if (blockType != EMPTY && blockType != BEDROCK)
        {
            terrain->setBlockAt(outBlockHit.x, outBlockHit.y, outBlockHit.z, EMPTY);
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->clear_VBO_data();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->createVBOdata();
            terrain->getChunkAt(outBlockHit.x, outBlockHit.z).get()->sendVBO();
        }
        return blockType;
    }
    return EMPTY;
}


void Player::setCameraWidthHeight(unsigned int w, unsigned int h)
{
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir)
{
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount)
{
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount)
{
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount)
{
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount)
{
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount)
{
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount)
{
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees)
{
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees)
{
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees)
{
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees)
{
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees)
{
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees)
{
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const
{
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const
{
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const
{
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const
{
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}

BlockType Player::get_camera_block(const Terrain &terrain) {
    glm::vec3 cam_pos = m_camera.mcr_position;
    if (cam_pos.y > 255 || cam_pos.y < 0) {
        return EMPTY;
    }

    BlockType cam_block = terrain.getBlockAt(floor(cam_pos.x), floor(cam_pos.y), floor(cam_pos.z));

    return cam_block;
}
