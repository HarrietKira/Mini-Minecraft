#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

// Texture image
uniform sampler2D u_texture;

// Time counter
uniform int u_time;

// Camera pos
uniform vec3 u_eye;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_Col;

in vec2 fs_uv;
in float fs_animation;

out vec4 out_Col; // This is the final output color that you will see on your
                  // screen for the pixel that is currently being processed.

const float SUN_VELOCITY = 1 / 200.f;
const float SUNSET_LEN = 0.3;

// Sun palette
const vec3 sun[3] = vec3[](vec3(255, 255, 245) / 255.0,
                            vec3(255, 137, 103) / 255.0,
                            vec3(107, 73, 132) / 255.0);


void main()
{
    // Material base color (before shading)
    vec4 diffuseColor;
    if (fs_animation == 1.f) {
        // Animation for WATER and LAVA
        float offset = (u_time % 50 / 50.f) * (1.f / 16.f);
        diffuseColor = texture(u_texture, vec2(fs_uv.x + offset, fs_uv.y));
    } else {
        diffuseColor = texture(u_texture, fs_uv);
    }

    // Direction of sun light
    vec3 sunDir = normalize(vec3(cos(u_time * SUN_VELOCITY), sin(u_time * SUN_VELOCITY), 0.f));

    // Indicator for day:1 or night:-1
    float day_night_ratio = sin(u_time * SUN_VELOCITY);

    // Color of sun light
    vec3 sun_color;
    if (day_night_ratio > SUNSET_LEN) {
        sun_color = sun[0];
    }
    else if (day_night_ratio < -SUNSET_LEN) {
        sun_color = sun[2];
    }
    else {
        if (day_night_ratio > 0) {
            float smooth_t = smoothstep(0.f, 1.f, abs(day_night_ratio) / SUNSET_LEN);
            sun_color = mix(sun[1], sun[0], smooth_t);
        }
        else {
            float smooth_t = smoothstep(0.f, 1.f, abs(day_night_ratio) / (SUNSET_LEN));
            sun_color = mix(sun[1], sun[2], smooth_t);
        }
    }

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(vec3(fs_Nor)), normalize(sunDir));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    // Higher the shininess, smaller and brighter the specular highlight
    float shininess = 32;
    // Calculate the specular term
    vec3 view_dir = u_eye - vec3(fs_Pos);
    vec3 H = (normalize(view_dir) + normalize(sunDir)) / 2;
    float specular = pow(dot(normalize(vec3(fs_Nor)), normalize(H)), shininess);
    // Avoid negative lighting values
    specular = clamp(specular, 0, 1);

    if (day_night_ratio < -SUNSET_LEN) {
        diffuseTerm = 0.f;
        specular = 0.f;
    }
    else if (day_night_ratio >= -SUNSET_LEN && day_night_ratio < SUNSET_LEN) {
        diffuseTerm = mix(0.f, diffuseTerm, (day_night_ratio + SUNSET_LEN) / (2.f * SUNSET_LEN));
        specular = mix(0.f, specular, (day_night_ratio + SUNSET_LEN) / (2.f * SUNSET_LEN));
    }

    //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.
    float ambientTerm = 0.3;

    float lightIntensity = 0.5 * diffuseTerm + ambientTerm + specular;

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb * lightIntensity * sun_color, diffuseColor.a);

    float dist = max(abs(fs_Pos.x - u_eye.x), abs(fs_Pos.z - u_eye.z)) * 0.008;

    float alpha = mix(out_Col.a, 0.f, clamp(pow(dist, 10), 0.f, 1.f));
    out_Col = vec4(out_Col.rgb, alpha);
}
