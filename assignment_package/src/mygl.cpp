#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_postprog(this), m_prog_sky(this),
      m_quad(this), m_frameBuffer(this, this->width()*this->devicePixelRatio(), this->height()*this->devicePixelRatio(), this->devicePixelRatio()),
      m_terrain(this), m_player(glm::vec3(320.f, 150.f, 320.f), m_terrain), m_time(0),
      m_selectedBlockType(GRASS),
      m_inventoryOpened(false),
      m_GRASSPlacable(true), m_DIRTPlaceable(true), m_STONEPlacable(true),
      m_WATERPlacable(true), m_SNOWPlacable(true), m_LAVAPlacable(true),
      m_BEDROCKPlacable(true), m_WOODPlacable(true), m_LEAFPlacable(true)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL()
{
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter()
{
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Enable alpha blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    m_terrain.create_texture(":/textures/minecraft_textures_all.png");

    m_quad.createVBOdata();
    m_frameBuffer.create();
    m_frameBuffer.bindFrameBuffer();
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Frame buffer did not initialize correctly..." << std::endl;
        printGLErrorLog();
    }

    m_postprog.create(":/glsl/post.vert.glsl", ":/glsl/post.frag.glsl");

    // Create the shaders program with sky shaders
    m_prog_sky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
}

void MyGL::resizeGL(int w, int h)
{
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)
    m_progLambert.setViewProjMatrix(viewproj);

    // Set camera position
    m_progLambert.set_eye(m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);

    m_frameBuffer.resize(w * this->devicePixelRatio(), this->height() * this->devicePixelRatio(), this->devicePixelRatio());

    m_frameBuffer.destroy();
    m_frameBuffer.create();

    // Set the inverse of view-project matrix for mapping pixels back to world points
    m_prog_sky.setViewProjMatrix(glm::inverse(viewproj));
    // Set screen size
    m_prog_sky.set_dimensions(width(), height());
    // Set camera position
    m_prog_sky.set_eye(m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick()
{
    // MM1
    glm::vec3 prevPlayerPos = m_player.mcr_position;
    float dT = (QDateTime::currentMSecsSinceEpoch() - m_currMSecSinceEpoch) / 1000.f;
    m_player.tick(dT, m_inputs);
    m_currMSecSinceEpoch = QDateTime::currentMSecsSinceEpoch();

    // Update terrain based on position of player
    m_terrain.expandZone(m_player.mcr_position, prevPlayerPos);

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
}

void MyGL::sendPlayerDataToGUI() const
{
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    m_frameBuffer.bindFrameBuffer();
    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());

    // Set the inverse of view-project matrix for mapping pixels back to world points
    m_prog_sky.setViewProjMatrix(glm::inverse(m_player.mcr_camera.getViewProj()));
    // Set camera position
    m_prog_sky.set_eye(m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);
    // Set time
    m_prog_sky.set_time(m_time);

    // Set time for water and lava shader
    m_postprog.set_time(m_time);

    // Set camera position
    m_progLambert.set_eye(m_player.mcr_camera.mcr_position.x, m_player.mcr_camera.mcr_position.y, m_player.mcr_camera.mcr_position.z);
    // Increase time
    m_progLambert.set_time(m_time);
    m_time++;

    // Draw the sky box
    m_prog_sky.draw(m_quad);

    renderTerrain();

    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());

    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0, 0, this->width()* this->devicePixelRatio(), this->height() *this->devicePixelRatio());
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_frameBuffer.bindToTextureSlot(1);
    if (m_time > 100) {
        if (m_player.get_camera_block(m_terrain) == WATER) {
            m_postprog.set_post_type(1);
            m_postprog.draw_quad(m_quad);
        } else if (m_player.get_camera_block(m_terrain) == LAVA) {
            m_postprog.set_post_type(2);
            m_postprog.draw_quad(m_quad);
        } else {
            m_postprog.set_post_type(0);
            m_postprog.draw_quad(m_quad);
        }
    }
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    // Render nine zones of generated terrain that surround the player
    int player_x = 16 * static_cast<int>(glm::floor(m_player.mcr_position.x / 16.f));
    int player_z = 16 * static_cast<int>(glm::floor(m_player.mcr_position.z / 16.f));
    m_terrain.bind_texture();
    m_terrain.draw(player_x - 160, player_x + 160, player_z - 160, player_z + 160, &m_progLambert);
}


void MyGL::keyPressEvent(QKeyEvent *e)
{
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier)
    {
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape)
    {
        QApplication::quit();
    }
    else if (e->key() == Qt::Key_Right)
    {
        m_player.rotateOnUpGlobal(-amount);
    }
    else if (e->key() == Qt::Key_Left)
    {
        m_player.rotateOnUpGlobal(amount);
    }
    else if (e->key() == Qt::Key_Up)
    {
        m_player.rotateOnRightLocal(-amount);
    }
    else if (e->key() == Qt::Key_Down)
    {
        m_player.rotateOnRightLocal(amount);
    }
    else if (e->key() == Qt::Key_W)
    {
        m_inputs.wPressed = true;
    }
    else if (e->key() == Qt::Key_S)
    {
        m_inputs.sPressed = true;
    }
    else if (e->key() == Qt::Key_D)
    {
        m_inputs.dPressed = true;
    }
    else if (e->key() == Qt::Key_A)
    {
        m_inputs.aPressed = true;
    }
    else if (e->key() == Qt::Key_Q)
    {
        m_inputs.qPressed = true;
    }
    else if (e->key() == Qt::Key_E)
    {
        m_inputs.ePressed = true;
    }
    else if (e->key() == Qt::Key_F)
    {
        m_inputs.fPressed = true;
        m_inputs.flight_mode = !m_inputs.flight_mode;
    }
    else if (e->key() == Qt::Key_Space)
    {
        m_inputs.spacePressed = true;
    }
}

// MM1
void MyGL::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_W)
    {
        m_inputs.wPressed = false;
    }
    else if (e->key() == Qt::Key_S)
    {
        m_inputs.sPressed = false;
    }
    else if (e->key() == Qt::Key_D)
    {
        m_inputs.dPressed = false;
    }
    else if (e->key() == Qt::Key_A)
    {
        m_inputs.aPressed = false;
    }
    else if (e->key() == Qt::Key_Q)
    {
        m_inputs.qPressed = false;
    }
    else if (e->key() == Qt::Key_E)
    {
        m_inputs.ePressed = false;
    }
    else if (e->key() == Qt::Key_F)
    {
        m_inputs.fPressed = false;
    }
    else if (e->key() == Qt::Key_Space)
    {
        m_inputs.spacePressed = false;
    }
    else if (e->key() == Qt::Key_I)
    {
        m_inventoryOpened = !m_inventoryOpened;
        emit sig_inventoryWindow(m_inventoryOpened);
    }
}


void MyGL::mouseMoveEvent(QMouseEvent *e)
{
    // MM1, rotate camera
    float dx = (width() * 0.5 - e->pos().x()) / width();
    float dy = (height() * 0.5 - e->pos().y()) / height();
    m_player.rotateOnUpGlobal(dx * 360 * 0.05f);
    m_player.rotateOnRightLocal(dy * 360 * 0.05f);
    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e)
{
    // TODO
    if (e->button() == Qt::LeftButton)
    {
        BlockType removedBlock = m_player.removeBlock(&m_terrain);
        if(removedBlock != EMPTY) emit sig_updateInventory(removedBlock, 1);
    }
    else if (e->button() == Qt::RightButton)
    {
        BlockType curr = m_selectedBlockType;
        bool placable = false;
        if(curr == GRASS)
        {
            placable = m_GRASSPlacable;
        }
        else if(curr == DIRT)
        {
            placable = m_DIRTPlaceable;
        }
        else if(curr == STONE)
        {
            placable = m_STONEPlacable;
        }
        else if(curr == WATER)
        {
            placable = m_WATERPlacable;
        }
        else if(curr == SNOW)
        {
            placable = m_SNOWPlacable;
        }
        else if(curr == LAVA)
        {
            placable = m_LAVAPlacable;
        }
        else if(curr == BEDROCK)
        {
            placable = m_BEDROCKPlacable;
        }
        else if(curr == WOOD)
        {
            placable = m_WOODPlacable;
        }
        else if(curr == LEAF)
        {
            placable = m_LEAFPlacable;
        }
        BlockType addedBlock = EMPTY;
        if(placable) addedBlock = m_player.addBlock(&m_terrain, m_selectedBlockType);
        if(addedBlock != EMPTY) emit sig_updateInventory(addedBlock, -1);
    }
}
