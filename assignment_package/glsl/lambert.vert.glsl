#version 150
// ^ Change this to version 130 if you have compatibility issues

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_Model;       // The matrix that defines the transformation of the
                            // object we're rendering. In this assignment,
                            // this will be the result of traversing your scene graph.

uniform mat4 u_ModelInvTr;  // The inverse transpose of the model matrix.
                            // This allows us to transform the object's normals properly
                            // if the object has been non-uniformly scaled.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

// Time counter
uniform int u_time;

in vec4 vs_Pos;             // The array of vertex positions passed to the shader

in vec4 vs_Nor;             // The array of vertex normals passed to the shader

in vec4 vs_Col;             // The array of vertex colors passed to the shader.

out vec4 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.

out vec2 fs_uv;
out float fs_animation;

const float PI = 3.14159265359;


vec4 distort_surface_pos(vec4 ori_pos) {
    vec4 res = ori_pos;
    res.y += sin(ori_pos.x * 2.f + u_time % 50 / 50.f * 2.f * PI) * 0.07;
    return res;
}

vec4 distort_surface_norm(vec4 ori_pos) {
    vec3 grad_x = vec3(1, 2.f * cos(ori_pos.x * 2.f + u_time % 50 / 50.f * 2.f * PI) * 0.07, 0);
    vec3 grad_z = vec3(0, 0, 1);
    vec3 res = cross(grad_x, grad_z);
    return vec4(-normalize(res), 0);
}

void main()
{
    // Extract uv and animation flag from color
    fs_uv = vs_Col.xy;
    fs_animation = vs_Col.z;

    // Distort position for surface of water
    vec4 distort_pos;
    if (fs_animation == 1.f) {
        distort_pos = distort_surface_pos(u_Model * vs_Pos);
    }
    else {
        distort_pos = u_Model * vs_Pos;
    }

    fs_Pos = distort_pos;

    mat3 invTranspose = mat3(u_ModelInvTr);

    // Distort normal for surface of water
    if (fs_animation == 1.f) {
        fs_Nor = distort_surface_norm(distort_pos);
    }
    else {
        fs_Nor = vec4(invTranspose * vec3(vs_Nor), 0);          // Pass the vertex normals to the fragment shader for interpolation.
                                                                // Transform the geometry's normals by the inverse transpose of the
                                                                // model matrix. This is necessary to ensure the normals remain
                                                                // perpendicular to the surface after the surface is transformed by
                                                                // the model matrix.
    }

    vec4 modelposition = distort_pos;   // Temporarily store the transformed vertex positions for use below

    gl_Position = u_ViewProj * modelposition;// gl_Position is a built-in variable of OpenGL which is
                                             // used to render the final positions of the geometry's vertices
}
