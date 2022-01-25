#include "terrain.h"
#include "cube.h"
#include <stdexcept>
#include <iostream>
#include <scene/procedureterrain.h>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), m_geomCube(context), mp_context(context), mp_texture(nullptr)
{}

Terrain::~Terrain() {
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else
    {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram)
{
    // Traverse trunks in range
    BlockTypeMutex.lock();
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);

                shaderProgram->setModelMatrix(glm::translate(glm::mat4(1.f), glm::vec3(x, 0.f, z)));
                shaderProgram->draw_interleaved(*chunk, 0);
            }
        }
    }

    // Traverse transparent trunks in range
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);

                shaderProgram->setModelMatrix(glm::translate(glm::mat4(1.f), glm::vec3(x, 0.f, z)));
                shaderProgram->draw_interleaved_transparent(*chunk, 0);
            }
        }
    }
    BlockTypeMutex.unlock();
}

void Terrain::CreateTestScene()
{
    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    std::vector<Chunk*> chunkList;
    std::vector<glm::ivec2> coordinates;
    for(int x = 0; x < 640; x += 16)
    {
        for(int z = 0; z < 640; z += 16)
        {
            Chunk* newChunk = instantiateChunkAt(x, z);
            chunkList.push_back(newChunk);
            coordinates.push_back(glm::ivec2(x, z));
        }
    }

    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    m_generatedTerrain.insert(toKey(0, 0));

    // Create the basic terrain floor
    for (int i = 0; i < int(chunkList.size()); i++) {
        auto &chunk = chunkList[i];
        int chunk_x = coordinates[i].x;
        int chunk_z = coordinates[i].y;

        createBlocks(chunk_x, chunk_z, chunk);
     }

    // Generate VBOs
    for (int i = 0; i < int(chunkList.size()); i++)
    {
        auto &chunk = chunkList[i];
        chunk->createVBOdata();
        chunk->sendVBO();
    }
}

BlockType Terrain::generateBlockByHeight(int height, int maxHeight, int isGrass, int cur_x, int cur_z) {
    if (height == 0) {
        return BEDROCK;
    } else if (height <= 128) {
//        if (height < 50) {
//            float caveParameter = ProcedureTerrain::fractalNoise3D(glm::vec3(cur_x, height, cur_z) / 4096.f, 10);
//            if (caveParameter < 0) {
//                if (height < 25) {
//                    return LAVA;
//                } else {
//                    return EMPTY;
//                }
//            }
//        }
        return STONE;
    } else {
        if (isGrass) {
            if (height == maxHeight) {
                return GRASS;
            } else {
                return DIRT;
            }
        } else {
            if (maxHeight <= 200) {
                return STONE;
            } else {
                if (height == maxHeight) {
                    return SNOW;
                } else {
                    return STONE;
                }
            }
        }
    }
}

// MM2
uPtr<Chunk> Terrain::instantiateChunkAt0(int x, int z)
{
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context);
    Chunk *cPtr = chunk.get();
    // MM2
    cPtr->setChunkPos(x, z);

    return chunk;
}
void Terrain::VBOWorker(uPtr<Chunk> chunk)
{
    chunk->createVBOdata();
    VBOMutex.lock();
    VBOChunks[toKey(chunk->getChunkPos().x, chunk->getChunkPos().y)] = move(chunk);
    VBOMutex.unlock();
}
void Terrain::BlockTypeWorker(uPtr<Chunk> chunk)
{
    glm::ivec2 chunkPos = chunk->getChunkPos();

    // Create the basic terrain floor
    createBlocks(chunkPos.x, chunkPos.y, chunk.get());

    BlockTypeMutex.lock();
    BlockTypeChunks[toKey(chunk->getChunkPos().x, chunk->getChunkPos().y)] = move(chunk);
    BlockTypeMutex.unlock();
}
bool Terrain::hasZoneAt(glm::ivec2 zonePos) const
{
    return m_generatedTerrain.find(toKey(zonePos.x, zonePos.y)) != m_generatedTerrain.end();
}
std::vector<glm::ivec2> Terrain::getSurroundingZones(glm::vec2 pos, int n)
{
    std::vector<glm::ivec2> surroundingZones = {};
    int halfWidth = glm::floor(n / 2.f) * 64;
    for (int i = pos.x - halfWidth; i <= pos.x + halfWidth; i += 64)
    {
        for (int j = pos.y - halfWidth; j <= pos.y + halfWidth; j+= 64)
        {
            surroundingZones.push_back(glm::ivec2(i, j));
        }
    }
    return surroundingZones;
}
std::vector<glm::ivec2> Terrain::diffVectors(std::vector<glm::ivec2> a,
                                             std::vector<glm::ivec2> b)
{
    std::vector<glm::ivec2> diff;

    for (glm::ivec2 vecB : b)
    {
        bool match = false;
        for (glm::ivec2 vecA : a)
        {
            if (vecA.x == vecB.x && vecA.y == vecB.y)
            {
                match = true;
                break;
            }
        }

        if (!match)
        {
            diff.push_back(vecB);
        }
    }

    return diff;
}
bool Terrain::hasNewChunkAt(int x, int z) const
{
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return newChunks.find(toKey(16 * xFloor, 16 * zFloor)) != newChunks.end();
}
uPtr<Chunk>& Terrain::getNewChunkAt(int x, int z)
{
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));

    return newChunks[toKey(16 * xFloor, 16 * zFloor)];
}
void Terrain::expandZone(glm::vec3 currPlayerPos, glm::vec3 prevPlayerPos)
{
    glm::ivec2 currZone = glm::ivec2(glm::floor(currPlayerPos.x / 64.f) * 64.f,
                                     glm::floor(currPlayerPos.z / 64.f) * 64.f);
    glm::ivec2 prevZone = glm::ivec2(glm::floor(prevPlayerPos.x / 64.f) * 64.f,
                                     glm::floor(prevPlayerPos.z / 64.f) * 64.f);
    std::vector<glm::ivec2> currSourrondingZones = getSurroundingZones(currZone, 5);
    std::vector<glm::ivec2> prevSourrondingZones = getSurroundingZones(prevZone, 5);

    std::vector<glm::ivec2> newZones = firstTick ? currSourrondingZones : diffVectors(prevSourrondingZones, currSourrondingZones);
    std::vector<glm::ivec2> oldZones = diffVectors(currSourrondingZones,
                                                   prevSourrondingZones);
    for (glm::ivec2 newZone : newZones)
    {
        if (!hasZoneAt(newZone))
        {
            m_generatedTerrain.insert(toKey(newZone.x, newZone.y));

            for (int x = 0; x < 64; x += 16)
            {
                for (int z = 0; z < 64; z += 16)
                {
                    newChunks[toKey(newZone.x + x, newZone.y + z)] = instantiateChunkAt0(newZone.x + x, newZone.y + z);
                }
            }
        }
    }

    for (auto & [ key, chunk ]: newChunks)
    {
        int x = chunk->getChunkPos().x;
        int z = chunk->getChunkPos().y;

        if (hasChunkAt(x, z + 16))
        {
            auto &chunkNorth = getChunkAt(x, z + 16);
            chunk.get()->linkNeighbor(chunkNorth, ZPOS);
        }
        if (hasChunkAt(x, z - 16))
        {
            auto &chunkSouth = getChunkAt(x, z - 16);
            chunk.get()->linkNeighbor(chunkSouth, ZNEG);
        }
        if (hasChunkAt(x + 16, z))
        {
            auto &chunkEast = getChunkAt(x + 16, z);
            chunk.get()->linkNeighbor(chunkEast, XPOS);
        }
        if (hasChunkAt(x - 16, z))
        {
            auto &chunkWest = getChunkAt(x - 16, z);
            chunk.get()->linkNeighbor(chunkWest, XNEG);
        }

        if (hasNewChunkAt(x, z + 16))
        {
            auto &chunkNorth = getNewChunkAt(x, z + 16);
            chunk.get()->linkNeighbor(chunkNorth, ZPOS);
        }
        if (hasNewChunkAt(x, z - 16))
        {
            auto &chunkSouth = getNewChunkAt(x, z - 16);
            chunk.get()->linkNeighbor(chunkSouth, ZNEG);
        }
        if (hasNewChunkAt(x + 16, z))
        {
            auto &chunkEast = getNewChunkAt(x + 16, z);
            chunk.get()->linkNeighbor(chunkEast, XPOS);
        }
        if (hasNewChunkAt(x - 16, z))
        {
            auto &chunkWest = getNewChunkAt(x - 16, z);
            chunk.get()->linkNeighbor(chunkWest, XNEG);
        }
    }

    for (auto & [ key, chunk ]: newChunks)
    {
        BlockTypeThreads.push_back(std::thread(&Terrain::BlockTypeWorker, this, move(chunk)));
    }

    newChunks.clear();

    for (auto &BlockTypeThread : BlockTypeThreads)
    {
        if(BlockTypeThread.joinable())
        {
//            std::cout << "joinable" << std::endl;
            BlockTypeThread.join();
        }
    }


    BlockTypeMutex.lock();
    for (auto & [ key, chunk ] : BlockTypeChunks)
    {
        VBOThreads.push_back(std::thread(&Terrain::VBOWorker, this, move(chunk)));
    }
    BlockTypeChunks.clear();
    BlockTypeMutex.unlock();

    VBOMutex.lock();
    for (auto & [ key, chunk ] : VBOChunks)
    {
        chunk.get()->sendVBO();
        m_chunks[key] = move(chunk);
    }

//    for (auto & [ key, chunk ] : VBOChunks)
//    {
//        chunk->update_neighbor_chunks();
//    }
    VBOChunks.clear();
    VBOMutex.unlock();
    firstTick = false;
}

void Terrain::createBlocks(int target_x, int target_z, Chunk* chunk) {
    int biome = -1;
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            int height = ProcedureTerrain::getHeight(target_x + x, target_z + z, &biome);
            if (height <= 138 && height >= 128) {
                for (int w = height + 1; w <= 138; w++) {
                    chunk->setBlockAt(x, w, z, WATER);
                }
            }
            for (int k = 0; k <= height; k++) {
                chunk->setBlockAt(x, k, z, generateBlockByHeight(k, height, biome, target_x + x, target_z + z));
                if (rand() % 500 == 0) {
                    if (height < 142 && (target_x > 333 || target_x < 305) && (target_z > 333 || target_z < 305) && chunk->getBlockAt(x, k, z) == GRASS && chunk->getBlockAt(x, k + 1, z) != WATER) {
                        if (x < 12 && z < 12 && x > 4 && z > 4) {
                            for (int y = height; y < height + 3; y++) {
                                chunk->setBlockAt(x, y, z, WOOD);
                            }
                            for (int y = height + 3; y < height + 8; y++) {
                                for (int i = x - 1; i <= x + 1; i++) {
                                    for (int j = z - 1; j <= z + 1; j++) {
                                        chunk->setBlockAt(i, y, j, LEAF);
                                    }
                                }
                            }
                            for (int y = height + 4; y < height + 5; y++) {
                                for (int i = x - 2; i <= x + 2; i++) {
                                    for (int j = z - 2; j <= z + 2; j++) {
                                        chunk->setBlockAt(i, y, j, LEAF);
                                    }
                                }
                            }
                            for (int y = height + 5; y < height + 6; y++) {
                                for (int i = x - 3; i <= x + 3; i++) {
                                    for (int j = z - 3; j <= z + 3; j++) {
                                        chunk->setBlockAt(i, y, j, LEAF);
                                    }
                                }
                            }
                            for (int y = height + 6; y < height + 7; y++) {
                                for (int i = x - 2; i <= x + 2; i++) {
                                    for (int j = z - 2; j <= z + 2; j++) {
                                        chunk->setBlockAt(i, y, j, LEAF);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Terrain::create_texture(const char *textureFile) {
    mp_texture = std::unique_ptr<Texture>(new Texture(mp_context));
    mp_texture->create(textureFile);
    mp_texture->load(0);
}

void Terrain::bind_texture() {
    mp_texture->bind(0);
}
