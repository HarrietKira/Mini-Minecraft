#include "chunk.h"
#include <iostream>


Chunk::Chunk(OpenGLContext *context)
    : Drawable(context), m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::createVBOdata() {
    std::vector<GLuint> vec_idx;
    // Store interleaved data as pos-norm-col
    std::vector<glm::vec4> vec_data;

    // Store interleaved data for transparent blocks
    std::vector<GLuint> vec_idx_transparent;
    std::vector<glm::vec4> vec_data_transparent;

    // Record offset of vertex index for each face
    int index_offset = 0;

    int index_offset_transparent = 0;

    // Traverse all blocks
    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 16; x++) {
                BlockType t = getBlockAt(x, y, z);

                // Position offset of vertex
                glm::vec4 block_offset(x, y, z, 0);

                // Offset of uv coords for 4 corners
                glm::vec2 uv_offset[4] = {glm::vec2(0, 0),
                                          glm::vec2(1.f / 16.f, 0),
                                          glm::vec2(1.f / 16.f, 1.f / 16.f),
                                          glm::vec2(0, 1.f / 16.f)};

                if (t == EMPTY) {
                    continue;
                }
                if (!is_transparent(t)) {
                    // Check all neighbors for opaque blocks

                    // YPOS
                    //////////////////////////////////////////////////////////////////
                    BlockType t_y_pos;
                    if (y == 255) {
                        // Border case
                        t_y_pos = EMPTY;
                    } else {
                        t_y_pos = getBlockAt(x, y + 1, z);
                    }

                    if (t_y_pos == EMPTY || is_transparent(t_y_pos)) {
                        // Ceiling
                        glm::vec4 ceiling[4] = {glm::vec4(0, 1, 1, 1),
                                                glm::vec4(1, 1, 1, 1),
                                                glm::vec4(1, 1, 0, 1),
                                                glm::vec4(0, 1, 0, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data.push_back(ceiling[i] + block_offset);
                            // Normal
                            vec_data.push_back(glm::vec4(0, 1, 0, 0));

                            // UV
                            switch(t) {
                                case GRASS:
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 13.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case DIRT:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case STONE:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case WOOD:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LEAF:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 12.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LAVA:
                                    vec_data.push_back(glm::vec4(glm::vec2(13.f / 16.f, 1.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                case BEDROCK:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case SNOW:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 11.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 1);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset + 3);

                        index_offset += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // YNEG
                    //////////////////////////////////////////////////////////////////
                    BlockType t_y_neg;
                    if (y == 0) {
                        // Border case
                        t_y_neg = EMPTY;
                    } else {
                        t_y_neg = getBlockAt(x, y - 1, z);
                    }

                    if (t_y_neg == EMPTY || is_transparent(t_y_neg)) {
                        // Floor
                        glm::vec4 floor[4] = {glm::vec4(0, 0, 0, 1),
                                              glm::vec4(1, 0, 0, 1),
                                              glm::vec4(1, 0, 1, 1),
                                              glm::vec4(0, 0, 1, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data.push_back(floor[i] + block_offset);
                            // Normal
                            vec_data.push_back(glm::vec4(0, -1, 0, 0));

                            // UV
                            switch(t) {
                                case GRASS:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case DIRT:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case STONE:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case WOOD:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LEAF:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 12.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LAVA:
                                    vec_data.push_back(glm::vec4(glm::vec2(13.f / 16.f, 1.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                case BEDROCK:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case SNOW:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 11.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 1);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset + 3);

                        index_offset += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // XPOS
                    //////////////////////////////////////////////////////////////////
                    BlockType t_x_pos;
                    if (x == 15) {
                        // Border case
                        if (m_neighbors[XPOS] == nullptr) {
                            t_x_pos = EMPTY;
                        } else {
                            t_x_pos = m_neighbors[XPOS]->getBlockAt(0, y, z);
                        }
                    } else {
                        t_x_pos = getBlockAt(x + 1, y, z);
                    }

                    if (t_x_pos == EMPTY || is_transparent(t_x_pos)) {
                        // Forward
                        glm::vec4 forward[4] = {glm::vec4(1, 0, 1, 1),
                                                glm::vec4(1, 0, 0, 1),
                                                glm::vec4(1, 1, 0, 1),
                                                glm::vec4(1, 1, 1, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data.push_back(forward[i] + block_offset);
                            // Normal
                            vec_data.push_back(glm::vec4(1, 0, 0, 0));

                            // UV
                            switch(t) {
                                case GRASS:
                                    vec_data.push_back(glm::vec4(glm::vec2(3.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case DIRT:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case STONE:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case WOOD:
                                    vec_data.push_back(glm::vec4(glm::vec2(4.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LEAF:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 12.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LAVA:
                                    vec_data.push_back(glm::vec4(glm::vec2(13.f / 16.f, 1.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                case BEDROCK:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case SNOW:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 11.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 1);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset + 3);

                        index_offset += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // XNEG
                    //////////////////////////////////////////////////////////////////
                    BlockType t_x_neg;
                    if (x == 0) {
                        // Border case
                        if (m_neighbors[XNEG] == nullptr) {
                            t_x_neg = EMPTY;
                        } else {
                            t_x_neg = m_neighbors[XNEG]->getBlockAt(15, y, z);
                        }
                    } else {
                        t_x_neg = getBlockAt(x - 1, y, z);
                    }

                    if (t_x_neg == EMPTY || is_transparent(t_x_neg)) {
                        // Back
                        glm::vec4 back[4] = {glm::vec4(0, 0, 0, 1),
                                             glm::vec4(0, 0, 1, 1),
                                             glm::vec4(0, 1, 1, 1),
                                             glm::vec4(0, 1, 0, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data.push_back(back[i] + block_offset);
                            // Normal
                            vec_data.push_back(glm::vec4(-1, 0, 0, 0));

                            // UV
                            switch(t) {
                                case GRASS:
                                    vec_data.push_back(glm::vec4(glm::vec2(3.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case DIRT:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case STONE:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case WOOD:
                                    vec_data.push_back(glm::vec4(glm::vec2(4.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LEAF:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 12.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LAVA:
                                    vec_data.push_back(glm::vec4(glm::vec2(13.f / 16.f, 1.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                case BEDROCK:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case SNOW:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 11.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 1);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset + 3);

                        index_offset += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // ZPOS
                    //////////////////////////////////////////////////////////////////
                    BlockType t_z_pos;
                    if (z == 15) {
                        // Border case
                        if (m_neighbors[ZPOS] == nullptr) {
                            t_z_pos = EMPTY;
                        } else {
                            t_z_pos = m_neighbors[ZPOS]->getBlockAt(x, y, 0);
                        }
                    } else {
                        t_z_pos = getBlockAt(x, y, z + 1);
                    }

                    if (t_z_pos == EMPTY || is_transparent(t_z_pos)) {
                        // Right
                        glm::vec4 right[4] = {glm::vec4(0, 0, 1, 1),
                                              glm::vec4(1, 0, 1, 1),
                                              glm::vec4(1, 1, 1, 1),
                                              glm::vec4(0, 1, 1, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data.push_back(right[i] + block_offset);
                            // Normal
                            vec_data.push_back(glm::vec4(0, 0, 1, 0));

                            // UV
                            switch(t) {
                                case GRASS:
                                    vec_data.push_back(glm::vec4(glm::vec2(3.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case DIRT:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case STONE:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case WOOD:
                                    vec_data.push_back(glm::vec4(glm::vec2(4.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LEAF:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 12.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LAVA:
                                    vec_data.push_back(glm::vec4(glm::vec2(13.f / 16.f, 1.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                case BEDROCK:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case SNOW:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 11.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 1);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset + 3);

                        index_offset += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // ZNEG
                    //////////////////////////////////////////////////////////////////
                    BlockType t_z_neg;
                    if (z == 0) {
                        // Border case
                        if (m_neighbors[ZNEG] == nullptr) {
                            t_z_neg = EMPTY;
                        } else {
                            t_z_neg = m_neighbors[ZNEG]->getBlockAt(x, y, 15);
                        }
                    } else {
                        t_z_neg = getBlockAt(x, y, z - 1);
                    }

                    if (t_z_neg == EMPTY || is_transparent(t_z_neg)) {
                        // Left
                        glm::vec4 left[4] = {glm::vec4(1, 0, 0, 1),
                                             glm::vec4(0, 0, 0, 1),
                                             glm::vec4(0, 1, 0, 1),
                                             glm::vec4(1, 1, 0, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data.push_back(left[i] + block_offset);
                            // Normal
                            vec_data.push_back(glm::vec4(0, 0, -1, 0));

                            // UV
                            switch(t) {
                                case GRASS:
                                    vec_data.push_back(glm::vec4(glm::vec2(3.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case DIRT:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case STONE:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 15.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case WOOD:
                                    vec_data.push_back(glm::vec4(glm::vec2(4.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LEAF:
                                    vec_data.push_back(glm::vec4(glm::vec2(5.f / 16.f, 12.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case LAVA:
                                    vec_data.push_back(glm::vec4(glm::vec2(13.f / 16.f, 1.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                case BEDROCK:
                                    vec_data.push_back(glm::vec4(glm::vec2(1.f / 16.f, 14.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                case SNOW:
                                    vec_data.push_back(glm::vec4(glm::vec2(2.f / 16.f, 11.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 1);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset);
                        vec_idx.push_back(index_offset + 2);
                        vec_idx.push_back(index_offset + 3);

                        index_offset += 4;
                    }
                    //////////////////////////////////////////////////////////////////
                } else {
                    // Check all neighbors for transparent blocks

                    // YPOS
                    //////////////////////////////////////////////////////////////////
                    BlockType t_y_pos;
                    if (y == 255) {
                        // Border case
                        t_y_pos = EMPTY;
                    } else {
                        t_y_pos = getBlockAt(x, y + 1, z);
                    }

                    if (t_y_pos == EMPTY) {
                        // Ceiling
                        glm::vec4 ceiling[4] = {glm::vec4(0, 1, 1, 1),
                                                glm::vec4(1, 1, 1, 1),
                                                glm::vec4(1, 1, 0, 1),
                                                glm::vec4(0, 1, 0, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data_transparent.push_back(ceiling[i] + block_offset);
                            // Normal
                            vec_data_transparent.push_back(glm::vec4(0, 1, 0, 0));

                            // UV
                            switch(t) {
                                case WATER:
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(13.f / 16.f, 3.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 1);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent + 3);

                        index_offset_transparent += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // YNEG
                    //////////////////////////////////////////////////////////////////
                    BlockType t_y_neg;
                    if (y == 0) {
                        // Border case
                        t_y_neg = EMPTY;
                    } else {
                        t_y_neg = getBlockAt(x, y - 1, z);
                    }

                    if (t_y_neg == EMPTY) {
                        // Floor
                        glm::vec4 floor[4] = {glm::vec4(0, 0, 0, 1),
                                              glm::vec4(1, 0, 0, 1),
                                              glm::vec4(1, 0, 1, 1),
                                              glm::vec4(0, 0, 1, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data_transparent.push_back(floor[i] + block_offset);
                            // Normal
                            vec_data_transparent.push_back(glm::vec4(0, -1, 0, 0));

                            // UV
                            switch(t) {
                                case WATER:
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(13.f / 16.f, 3.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 1);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent + 3);

                        index_offset_transparent += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // XPOS
                    //////////////////////////////////////////////////////////////////
                    BlockType t_x_pos;
                    if (x == 15) {
                        // Border case
                        if (m_neighbors[XPOS] == nullptr) {
                            t_x_pos = BEDROCK;
                        } else {
                            t_x_pos = m_neighbors[XPOS]->getBlockAt(0, y, z);
                        }
                    } else {
                        t_x_pos = getBlockAt(x + 1, y, z);
                    }

                    if (t_x_pos == EMPTY) {
                        // Forward
                        glm::vec4 forward[4] = {glm::vec4(1, 0, 1, 1),
                                                glm::vec4(1, 0, 0, 1),
                                                glm::vec4(1, 1, 0, 1),
                                                glm::vec4(1, 1, 1, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data_transparent.push_back(forward[i] + block_offset);
                            // Normal
                            vec_data_transparent.push_back(glm::vec4(1, 0, 0, 0));

                            // UV
                            switch(t) {
                                case WATER:
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(13.f / 16.f, 3.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 1);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent + 3);

                        index_offset_transparent += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // XNEG
                    //////////////////////////////////////////////////////////////////
                    BlockType t_x_neg;
                    if (x == 0) {
                        // Border case
                        if (m_neighbors[XNEG] == nullptr) {
                            t_x_neg = BEDROCK;
                        } else {
                            t_x_neg = m_neighbors[XNEG]->getBlockAt(15, y, z);
                        }
                    } else {
                        t_x_neg = getBlockAt(x - 1, y, z);
                    }

                    if (t_x_neg == EMPTY) {
                        // Back
                        glm::vec4 back[4] = {glm::vec4(0, 0, 0, 1),
                                             glm::vec4(0, 0, 1, 1),
                                             glm::vec4(0, 1, 1, 1),
                                             glm::vec4(0, 1, 0, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data_transparent.push_back(back[i] + block_offset);
                            // Normal
                            vec_data_transparent.push_back(glm::vec4(-1, 0, 0, 0));

                            // UV
                            switch(t) {
                                case WATER:
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(13.f / 16.f, 3.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 1);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent + 3);

                        index_offset_transparent += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // ZPOS
                    //////////////////////////////////////////////////////////////////
                    BlockType t_z_pos;
                    if (z == 15) {
                        // Border case
                        if (m_neighbors[ZPOS] == nullptr) {
                            t_z_pos = BEDROCK;
                        } else {
                            t_z_pos = m_neighbors[ZPOS]->getBlockAt(x, y, 0);
                        }
                    } else {
                        t_z_pos = getBlockAt(x, y, z + 1);
                    }

                    if (t_z_pos == EMPTY) {
                        // Right
                        glm::vec4 right[4] = {glm::vec4(0, 0, 1, 1),
                                              glm::vec4(1, 0, 1, 1),
                                              glm::vec4(1, 1, 1, 1),
                                              glm::vec4(0, 1, 1, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data_transparent.push_back(right[i] + block_offset);
                            // Normal
                            vec_data_transparent.push_back(glm::vec4(0, 0, 1, 0));

                            // UV
                            switch(t) {
                                case WATER:
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(13.f / 16.f, 3.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 1);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent + 3);

                        index_offset_transparent += 4;
                    }
                    //////////////////////////////////////////////////////////////////

                    // ZNEG
                    //////////////////////////////////////////////////////////////////
                    BlockType t_z_neg;
                    if (z == 0) {
                        // Border case
                        if (m_neighbors[ZNEG] == nullptr) {
                            t_z_neg = BEDROCK;
                        } else {
                            t_z_neg = m_neighbors[ZNEG]->getBlockAt(x, y, 15);
                        }
                    } else {
                        t_z_neg = getBlockAt(x, y, z - 1);
                    }

                    if (t_z_neg == EMPTY) {
                        // Left
                        glm::vec4 left[4] = {glm::vec4(1, 0, 0, 1),
                                             glm::vec4(0, 0, 0, 1),
                                             glm::vec4(0, 1, 0, 1),
                                             glm::vec4(1, 1, 0, 1)};

                        for (int i = 0; i < 4; i++) {
                            // Position
                            vec_data_transparent.push_back(left[i] + block_offset);
                            // Normal
                            vec_data_transparent.push_back(glm::vec4(0, 0, -1, 0));

                            // UV
                            switch(t) {
                                case WATER:
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(13.f / 16.f, 3.f / 16.f) + uv_offset[i], 1, 1));
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    vec_data_transparent.push_back(glm::vec4(glm::vec2(8.f / 16.f, 1.f / 16.f) + uv_offset[i], 0, 1));
                                    break;
                            }
                        }

                        // Index
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 1);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent);
                        vec_idx_transparent.push_back(index_offset_transparent + 2);
                        vec_idx_transparent.push_back(index_offset_transparent + 3);

                        index_offset_transparent += 4;
                    }
                    //////////////////////////////////////////////////////////////////
                }
            }
        }
    }

    // MM2
    chunkVBOData.vec_data = vec_data;
    chunkVBOData.vec_data_trans = vec_data_transparent;

    chunkVBOData.vec_id = vec_idx;
    chunkVBOData.vec_id_trans = vec_idx_transparent;
}
void Chunk::sendVBO()
{
    // send data to GPU
    // Set counter for indices buffer
    m_count = chunkVBOData.vec_id.size();
    m_count_transparent = chunkVBOData.vec_id_trans.size();

    // Send index data to VBO
    generateIdx();
    bindIdx();
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunkVBOData.vec_id.size() * sizeof(GLuint), chunkVBOData.vec_id.data(), GL_STATIC_DRAW);

    generate_idx_transparent();
    bind_idx_transparent();
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunkVBOData.vec_id_trans.size() * sizeof(GLuint), chunkVBOData.vec_id_trans.data(), GL_STATIC_DRAW);

    // Send position/color data to VBO
    generatePos();
    bindPos();
    mp_context->glBufferData(GL_ARRAY_BUFFER, chunkVBOData.vec_data.size() * sizeof(glm::vec4), chunkVBOData.vec_data.data(), GL_STATIC_DRAW);

    generate_data_transparent();
    bind_data_transparent();
    mp_context->glBufferData(GL_ARRAY_BUFFER, chunkVBOData.vec_data_trans.size() * sizeof(glm::vec4), chunkVBOData.vec_data_trans.data(), GL_STATIC_DRAW);
}

void Chunk::clear_VBO_data() {
    chunkVBOData.vec_data.clear();
    chunkVBOData.vec_data_trans.clear();

    chunkVBOData.vec_id.clear();
    chunkVBOData.vec_id_trans.clear();

    destroyVBOdata();
}

void Chunk::update_neighbor_chunks() {
    // Check 4 horizontal neighbors
    if (m_neighbors[XPOS] != nullptr) {
        m_neighbors[XPOS]->destroyVBOdata();
        m_neighbors[XPOS]->createVBOdata();
        m_neighbors[XPOS]->sendVBO();
    }
    if (m_neighbors[XNEG] != nullptr) {
        m_neighbors[XNEG]->destroyVBOdata();
        m_neighbors[XNEG]->createVBOdata();
        m_neighbors[XNEG]->sendVBO();
    }
    if (m_neighbors[ZPOS] != nullptr) {
        m_neighbors[ZPOS]->destroyVBOdata();
        m_neighbors[ZPOS]->createVBOdata();
        m_neighbors[ZPOS]->sendVBO();
    }
    if (m_neighbors[ZNEG] != nullptr) {
        m_neighbors[ZNEG]->destroyVBOdata();
        m_neighbors[ZNEG]->createVBOdata();
        m_neighbors[ZNEG]->sendVBO();
    }
}

bool Chunk::is_transparent(BlockType t) {
    return t == WATER;
}

void Chunk::setChunkPos(int x, int z)
{
    int x_floor = static_cast<int>(glm::floor(x / 16.f));
    int z_floor = static_cast<int>(glm::floor(z / 16.f));
    chunkPos = glm::ivec2(16 * x_floor, 16 * z_floor);
}
glm::ivec2 Chunk::getChunkPos()
{
    return chunkPos;
}
