## Mini Minecraft
This project made attempts to recover a similair but smaller scenery like Minecraft. Although this one is a mini version of Minecraft, we still get all basic features realized in this project. Detailed features and implementation are included in following parts.
### [Demo Video](https://www.youtube.com/watch?v=-H92uWlCeQw)
### Building Involves
OpenGL <br/>
GLSL <br/>
Qt Creator <br/>
### Feature List
* Efficient terrain rendering and chunking
* Texturing and texture animation
* Water waves
* Day and night cycle
* Distance fog
* Game engine tick function and player physics
* Multithreaded terrain generation
* Inventory and onscreen GUI 
* Procedural terrain
* Cave systems
* Procedurally placed assets
* Procedural grass color
* Post-process camera overlay

### Procedural Terrain
#### Get Height Function
I created the procedural terrain in the class procedural_terrain class. From the given x and z coordinate, it generated the mountain and the grassland biome with the noise function. Also, I set up generate block function in the terrain class to determine the boundary between each biome. This function decides what block should be filled in.  

#### Noise Function
I used the fractal brownian motion to create the noise for the mountain biome. I used the factual brownian motion and Worley to create the noise for the grassland biome. Besides the Perlin noise, I also used a worley noise with the Voronoi-based hill effects to create a difference between the neighbor block. I applied the glm::smoothstep function to create the transition between the mountain and grassland transition. I faced difficulty when setting up the parameters for the fractal brownian motion noise. When the octave is too high, there exists an entire empty block on a x-z coordinate. There are also some fluctuation when generating the grassland biome.

#### Create Test Scene Function
In the terrain class, I instantiated a chunk list to store the chunk and get the coordinate from each chunk in order to pass it to the get height function. Then, I created the test scene in the terrain class by getting the height of each block from the given x and z coordinates. I was also a little unclear about how to set up the water block. So I decided to manually set up the block into the specific water position.  

### Efficient Terrain Rendering and Chunking

#### Chunk-based Rendering
To achieve efficient rendering, we draw our terrain on a per-chunk basis rather than a per-block basis. Firstly we make `Chunk` class inherit from `Drawable`, which needs to modify the constructor function to take a `OpenGLContext` as input. Also, the function `createVBOdata` is overridden to populate correct VBO data for one chunk. In detail, we traverse all the blocks in the chunk to see if they have neighbors in each direction (`XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG`). If the neighbor block in any direction is `EMPTY`, we consider the block side on this direction is visible, and so store the data of four vertices for this side. The per-vertex data, including position, normal and color, is stored in an interleaved format, like `pos0 norm0 col0 pos1 norm1 col1 pos2 norm2 col2 pos3 norm3 col3`.

The `ShaderProgram` class now has a new drawing function called `draw_interleaved()`, which renders a `Drawable` that has been set up with interleaved VBOs as mentioned above. The `glVertexAttribPointer` extracts data for `vs_Pos`, `vs_Nor`, `vs_Col` respectively with a stride of `3 * sizeof(glm::vec4)`.

#### Update New Terrain

A function called `update_terrain` is added to the `Terrain` class to check whether a new chunk is needed each frame. It takes the current position of player as input. Since the requirement asks for a 16-blocks distance test, which is the width of one chunk. I decide to check all 8 neighbor chunks surrounding the player and generate terrain for empty ones using the terrain generator function. Notice that even though we do not de-load old VBO data in this milestone, only 9 surrounding zones of player will be rendered in `paintGL()`.

#### Shader Program

The Lambertian shader is used to render the chunks. The vertex shader takes position, normal, and color of each vertex. And the fragment shader performs a basic Lambertian shading.

### Game Engine Tick Function and Player Physics
#### Tick Function
Tick function exists both in `mygl.cpp` and `player.cpp`. `mygl::tick()` responds to a `QTimer` with a `timeout()` signal every 16ms (1/60 seconds), then passes down a `m_input` which contains all the information of key hitting status and mode info (flight_mode/ non_flight_mode) into `player::tick()`. Basically, this `m_input` is a `InputBundle` defined in `entity.cpp` that collects all necessary status info for character to perform further movements in scenery. To make player move smoothly like in reality, I compute the delta-time by using `QDateTime::currentMSecsSinceEpoch()` and store the previous frame's `currentMSecsSinceEpoch` in mygl in order to compute this delta, and initialize this variable in MyGL's constructor using `QDateTime::currentMSecsSinceEpoch()`. With this delta-time(`dT`), I manage to imitate a final velocity with acceleration that obeys physics laws. In addition, `player::tick()` simply calls 2 functions: processInputs and computePhysics, these are 2 important functions to determine the real movement of player and would be covered in later parts.

#### KeyPressEvent/ MouseClickEvent/ MouseMoveEvent
The key hitting status mentioned above is a bool value that turns to "true" once mygl receives a KeyPressEvent for specific keys in mygl.m_input. That is, every 1/60s, updated bool values would be passed down inside m_input to player and when a release event for key is received, the bool value turns back to "false" directly. The same with MousePressEvent and MouseMoveEvent. In detail, the specific tasks completed by each key and mouse events are listed as follows: <br />
(* note that our default mode is flight mode, and in both movement modes, the player's velocity is reduced to less than 100% of its current value by add a 0.95f on current velocity.) <br />
* When flight mode is active: <br />
W -> Accelerate positively along forward vector <br />
S -> Accelerate negatively along forward vector <br />
D -> Accelerate positively along right vector <br />
A -> Accelerate negatively along right vector <br />
E -> Accelerate positively along up vector <br />
Q -> Accelerate negatively along up vector <br />
F -> Toggle flight mode OFF <br />
* When flight mode is inactive: <br />
W -> Accelerate positively along forward vector <br />
S -> Accelerate negatively along forward vector <br />
D -> Accelerate positively along right vector <br />
A -> Accelerate negatively along right vector <br />
Spacebar -> Add a vertical component to the player's velocity to make them jump <br />
F -> Toggle flight mode ON <br />
* On both modes: <br />
Right Click on Mouse: place a block adjacent to the block face they are looking at (within 3 units' distance) <br />
Left Click on Mouse: remove the block currently overlapping the center of the screen <br />
Vertical Movement of Mouse: rotate camera up and down (according to delta distance mouse moves vertically) <br />
Horizontal Movement of Mouse: rotate camera left and right (according to delta distance mouse moves horizontally) <br />

#### Add/ Remove Block
When adding blocks, I applied `player:gridMarch()` to get the OutHitBlocks with current rayDirection and examine each OutHitBlocks whether they have EMPTY positions beside them to add a block given a BlockType. In Milestone, we always add one more GRASS block when add function called since the BlockType is not explicitly stated in assignment. When removing blocks, we did pretty the same procedure to check whether there's overlapping of block and current camera center with gridMarch, and set the overlapping block as EMPTY. 

#### Collision Detection
This part starts from `player::tick()`. Recall that processInputs and computePhysics are called when `player::tick()` operates. processInputs updates m_velocity and m_acceleration of player, while computePhysics computes the real displacement of player after time accumulation dT. After we get the displacement(which names rayDirection) with a distance and direction, we further perform collision detection based on current terrain. Our strategy is to perform detection in 3 axises separately of all 12 vertexes along rayDirections 3 axises. For every axis, we fetch the minimum distance every axis can move and updates each axises for rayDirection. This creates a smoothie effect when facing obstacles and "slides" along facing blocks. Note that, when doing collision detection for xyz axis, we spared a epsilon value of 0.001f for xz axis to avoid z-buffer like problem and ignores bottom 4 corners when checking xz axis. While for y axis checking, we only checked bottom 4 corners and top 4 corners, ignore the intermediate 4 corners, and leave no epsilon value for this axis, considering the purpose to let character overlap with ground.

### Cave Systems
#### 3D Noise Function
I added 3D noise function to generate the Cave System. The 3D noise function consists the general 3D Perlin noise with a `surflet3d()` function and a `random3()` function. Using this method, we are able to sample this 3D Perlin noise at every block whose height is under 128. I met a problem of getting the data set with both negative and positive values from the noise. The data only consists negative values. So I tested the parameter in the function and figured out that I should use a higher persistence as well as some extra divisor in order to perceive both negative and positive noise values. When we were trying to merge the cave system with our current biome, the program became very slow, so we commented out the cave part in the most current commit for our recorded video. 
#### Generate Block by Height Function
I revised the `generateBlockByHeight()` function. The biome is divided into three parts. The first part is when the height is zero, all the blocks should be set to Bedrock. The second part is when the height is smaller than 129. In this area, it consists lava and stone where the stone is generated with the 3D noise function. The cave is formed here. And at last when the height is higher than 128, we applied the noise from milestone one to form the grassland and the mountain. There was a problem when I try to generate the cave system with stone, the previous method consists dirt in the system, so I have to revise the logic in this function. 
#### Post Process Rendering
The water and lava are set to a state where they will not cause the player stop moving in the collision detection function. The moving speed is set at 2/3 normal speed.

### Texturing and Texture Animation
#### Rendering with Texture
To render blocks with texture instead of simple colors, we first store the texture image and set a file called `textures.qrc` to tell QT where it is. A unique pointer, `mp_texture`, is created for the `Terrain` class, which points to the texture image. Also there are two functions `Terrain::create_texture()` and `Terrain::bind_texture()` for creating and binding. The texture will be sampled by `u_texture` in the fragment shader, which takes uv coordinates as input and samples the corresponding color in texture image. Such uv coordinates are created when creating VBOs for chunks. Instead of directly setting a color for each block type, now we are setting uv coordinates. For example, if a block is stone, we find its texture is on row 1, colume 2, and so set its uv as `glm::vec2(1.f / 16.f, 15.f / 16.f)` (inverse v coord).
#### Transparent Blocks
For blocks with transparent texture, we need to enable alpha blending in `MyGL::initializeGL()`. After that OpenGL can apply transparency according to the fourth parameter of the color. However, the result may still be incorrect due the order of rendering. To resolve this problem, we split the interleaved VBO data into two VBOs, one for opaque blocks and another for transparent blocks (`WATER` for now). When performing rendering, the opaque VBO is rendered first, then the transparent VBO.
#### Animation for Water and Lava
The animated water and lava can be achieved by offseting their uv coordinates according to time, but before that we need to mark which block types need animation. Notice that we use uv coordinates instead of color now, which makes the third component unused. We utilize this component as a flag to indicate whether animation is needed. It will be extracted as `fs_animation` in vertex shader. Also, a timer `m_time` is added to the `MyGL` class, which increases by 1 every frame. This time is sent to fragment shader through `u_time`. Now for blocks requiring animation, an offset based on time, as `(u_time % 50 / 50.f) * (1.f / 16.f)`, is added to the u coord. The result is an illusion of running water or lava.

### Multithreaded Terrain Generation
#### BlockTypeWorker and VBOWorker
The main function of multithreading named expandZone in terrain.cpp is called in `mygl::tick()` and during this process expandZone takes in previous and updated positions of player. With current position of the player, it is able to check whether new terrain has to be generated. Multithreading is then used when player reaches the edge at a center zone of current terrain. When implementing multithreading, I created a `BlockTypeWorker` to push back filled Chunks into a collection of `Chunk*`s stored in Terrain and a VBOWorker to send completed VBO data to the GPU. Meanwhile, I used `std::mutex` to lock shared memory, one for BlockTypeMutex and another for VBOMutex. To store the created threads that operates with workers, I applied `std::vector<std::thread>` to store threads of workers respectively.
#### Initialize Terrain and Threads
Since our terrain is generated one chunk per time, I modified the initial size generated to a complete zone at very beginning, and then after every tick, determine whether to generate a new zone. Before, I forget to modify the initial size of terrain, which causes a large blank between newly generated terrain and initial one. This bug has been fixed. The difficulty for me of this part is understanding the relationship between threads and workers, also when to lock my shared memory. After I go through the examples given on website, I figured this out and things become a bit easier. Another problem realtes to the double rendering condition of transparent river layer. Although we have deleted the x and z coordinates edge of river, this bug still exist. Fortunately, We find out it was the problem of conflicts between `BlockTypeWorker` and `VBOWorker`, we have to make sure that `VBOWorker` always starts after all `BlockTypeWorker` finishes. To solve this, we add `thread.join()` for each `BlockTypeWorker`. 

### Procedurally placed assets
I add some extra blocks in the `createBlocks` function to create the tree assets and randomly place them throughout the world. The function will now construct trees in a range of height and only on the grassland around the terrain. At first, there are some of the trees in the river, I just set more restrictions to make the location of the trees more reasonable. As the map expands its size, there will be more trees shown in the map.  

### Procedural grass color
I created two base grass colors in the lambert fragment shader. By getting the x and z position and using the `fbm()` noise function, I just mixed the colors in the biome to create differently colored grass. I am having some problem creating the biome-like distribution. Instead of showing it in the final version, we create a branch to show this feature. I have attached a screenshot below. 
<img width="1229" alt="Screen Shot 2021-12-07 at 11 20 01 PM" src="https://user-images.githubusercontent.com/36288772/145231739-b1c5fe2d-9e9b-4594-8c2c-bead7e5e1d37.png">

### Post-process Camera Overlay
In the post fragment shader, I applied a wave-like screen distortion and coloration. I distort the `uv` as a function of time. I was trying to add some 2D noise, but there are some line errors on the boundary of the shader. Instead, I use the cos and sin function to animate the noise in order to distort the player’s view under water and lava. 

### Water waves
To get the effect as waves on water, firstly we need to modify the vertex position of the water block in the vertex shader. A sine function is used to change the y coordinate of a vertex, which takes its x coordinate as the input. The time is also a parameter here to achieve animated effect. Then the normal of each vertex is modified according to the position displacement. The new normal is computed as the normal of the sine function. Further, for better visualizing, we apply the Blinn-Phong model to our fragment shader by adding a specular term.

### Day and night cycle
The sample code from our course website is used for this feature. After merging the sample code, we have a sky box at the sunset time. First we change the position of the sun over time to rotate it around the world. A new color palette is added for the sky color during daytime. And the colors of both the sky and the sun are changed from sunrise to daytime to sunset to night, where the sunrise and the sunset share the same color palette. Also, in the fragment shader, the color and direction of the light are changed according to those of the sun. For sunrise and sunset, the color near the sun is changed, and dusky-colored farther away from the sun.

### Distance fog
The fog effect is implemented by interpolating the alpha value of the final color based on the distance. In detail, the distance of a pixel is computed by taking it back to world position, and calculate its distance from camera. Finally, the alpha channel of color output from fragment shader is interpolated based on this distance.

### Inventory and onscreen GUI
For inventory part, all block types have a default inventory number of 10 and every time the player breaks a block by left click mouse, the inventory of respective block type would increase by 1. The same thing happens for placing a new block. When the inventory for one block type drops to 0, this block type cannot be chosen anymore. For onscreen GUI, the inventory window pops out when pressing `I` and pressing `I` again to hide inventory window. For specific details on inventory window, each block type would have their model image and inventory number on it, along with a `QRadioButton` to indicate if they are chosen or not. As mentioned before, if inventory is dropped to 0, the respective `QRadioButton` is not be able to chosen anymore until the player breaks another block that belongs to this type. At the beginning of this game, every time add blocks, the default type is grass and can be changed via selecting radio button for each block type.

## Authors
Yuxuan Huang <br/>
Bowen Deng <br/>
Zechen Zhou <br/>
