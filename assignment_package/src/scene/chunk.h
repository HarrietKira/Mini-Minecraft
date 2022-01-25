#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>


//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, SNOW, LAVA, BEDROCK, WOOD, LEAF
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

// MM2
class Chunk;
struct ChunkVBOData
{
    Chunk* chunk;
    std::vector<glm::vec4> vec_data, vec_data_trans;
    std::vector<GLuint> vec_id, vec_id_trans;
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// Have Chunk inherit from Drawable
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    // MM2
    glm::ivec2 chunkPos;
    ChunkVBOData chunkVBOData;
  
public:
    Chunk(OpenGLContext *context);
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    // Creates VBO data for only visible block faces
    void createVBOdata() override;

    // Update VBOs of neighbor chunks when a new chunk is generated
    void update_neighbor_chunks();

    // Determine if a block is transparent
    bool is_transparent(BlockType t);
    // MM2
    void setChunkPos(int x, int z);
    glm::ivec2 getChunkPos();
    void sendVBO();

    void clear_VBO_data();
};

