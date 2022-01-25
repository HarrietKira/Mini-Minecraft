#version 150

uniform mat4 u_ViewProj;    // We're actually passing the inverse of the viewproj
                            // from our CPU, but it's named u_ViewProj so we don't
                            // have to bother rewriting our ShaderProgram class

uniform ivec2 u_dimensions; // Screen dimensions

uniform vec3 u_eye; // Camera pos

uniform int u_time;

out vec4 outColor;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

const bool ADD_CLOUD = true;

const float SUNSET_THRESHOLD = 0.75;
const float DUSK_THRESHOLD = -0.1;

const float SUN_VELOCITY = 1 / 200.f;
const float SUNSET_LEN = 0.4;

// Daytime palette
const vec3 daytime[5] = vec3[](vec3(194, 225, 247) / 255.0,
                               vec3(171, 214, 244) / 255.0,
                               vec3(149, 203, 241) / 255.0,
                               vec3(103, 181, 235) / 255.0,
                               vec3(81, 170, 232) / 255.0);

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                               vec3(254, 192, 81) / 255.0,
                               vec3(255, 137, 103) / 255.0,
                               vec3(253, 96, 81) / 255.0,
                               vec3(57, 32, 51) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(107, 73, 132) / 255.0,
                            vec3(72, 52, 117) / 255.0,
                            vec3(43, 47, 119) / 255.0,
                            vec3(20, 24, 82) / 255.0,
                            vec3(7, 11, 52) / 255.0);

// Sun palette
const vec3 sun[3] = vec3[](vec3(255, 255, 245) / 255.0,
                            vec3(255, 255, 190) / 255.0,
                            vec3(107, 73, 132) / 255.0);

const vec3 cloudColor = sunset[3];

// Map a 3d coordinate on a unit sphere to a 2d uv coordinate
// Compute for phi and theta in polar coordinates with radius = 1
vec2 sphereToUV(vec3 p) {
    // Horizontal angle, mapped to [0, 2pi]
    float phi = atan(p.z, p.x);
    if(phi < 0) {
        phi += TWO_PI;
    }
    // Vertical angle
    float theta = acos(p.y);
    // Map to [0, 1]
    return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}

// Map a 2d uv coordinate to a color in the daytime palette
// The palette is placed as bars on x direction with different colors
// For bars between the lowest and highest bars, the color is linearly interpolated from two adjacent colors
vec3 uvToDaytime(vec2 uv) {
    // Bar 0
    if(uv.y < 0.5) {
        return daytime[0];
    }
    // Bar 1
    else if(uv.y < 0.55) {
        return mix(daytime[0], daytime[1], (uv.y - 0.5) / 0.05);
    }
    // Bar 2
    else if(uv.y < 0.6) {
        return mix(daytime[1], daytime[2], (uv.y - 0.55) / 0.05);
    }
    // Bar 3
    else if(uv.y < 0.65) {
        return mix(daytime[2], daytime[3], (uv.y - 0.6) / 0.05);
    }
    // Bar 4
    else if(uv.y < 0.75) {
        return mix(daytime[3], daytime[4], (uv.y - 0.65) / 0.1);
    }
    // Bar 5
    return daytime[4];
}

// Map a 2d uv coordinate to a color in the sunset palette
// The palette is placed as bars on x direction with different colors
// For bars between the lowest and highest bars, the color is linearly interpolated from two adjacent colors
vec3 uvToSunset(vec2 uv) {
    // Bar 0
    if(uv.y < 0.5) {
        return sunset[0];
    }
    // Bar 1
    else if(uv.y < 0.55) {
        return mix(sunset[0], sunset[1], (uv.y - 0.5) / 0.05);
    }
    // Bar 2
    else if(uv.y < 0.6) {
        return mix(sunset[1], sunset[2], (uv.y - 0.55) / 0.05);
    }
    // Bar 3
    else if(uv.y < 0.65) {
        return mix(sunset[2], sunset[3], (uv.y - 0.6) / 0.05);
    }
    // Bar 4
    else if(uv.y < 0.75) {
        return mix(sunset[3], sunset[4], (uv.y - 0.65) / 0.1);
    }
    // Bar 5
    return sunset[4];
}

// Map a 2d uv coordinate to a color in the dusk palette
// The palette is placed as bars on x direction with different colors
// For bars between the lowest and highest bars, the color is linearly interpolated from two adjacent colors
vec3 uvToDusk(vec2 uv) {
    // Bar 0
    if(uv.y < 0.5) {
        return dusk[0];
    }
    // Bar 1
    else if(uv.y < 0.55) {
        return mix(dusk[0], dusk[1], (uv.y - 0.5) / 0.05);
    }
    // Bar 2
    else if(uv.y < 0.6) {
        return mix(dusk[1], dusk[2], (uv.y - 0.55) / 0.05);
    }
    // Bar 3
    else if(uv.y < 0.65) {
        return mix(dusk[2], dusk[3], (uv.y - 0.6) / 0.05);
    }
    // Bar 4
    else if(uv.y < 0.75) {
        return mix(dusk[3], dusk[4], (uv.y - 0.65) / 0.1);
    }
    // Bar 5
    return dusk[4];
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int z = -1; z <= 1; z++)
    {
        for(int y = -1; y <= 1; y++)
        {
            for(int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_time * 0.01 + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

float WorleyNoise(vec2 uv)
{
    // Tile the space
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            // Random point inside current neighboring cell
            vec2 point = random2(uvInt + neighbor);

            // Animate the point
            point = 0.5 + 0.5 * sin(u_time * 0.01 + 6.2831 * point); // 0 to 1 range

            // Compute the distance b/t the point and the fragment
            // Store the min dist thus far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float worleyFBM(vec3 uv) {
    float sum = 0;
    float freq = 4;
    float amp = 0.5;
    for(int i = 0; i < 8; i++) {
        sum += WorleyNoise3D(uv * freq) * amp;
        freq *= 2;
        amp *= 0.5;
    }
    return sum;
}



void main()
{
    // From pixel space to world space
    vec2 ndc = (gl_FragCoord.xy / vec2(u_dimensions)) * 2.0 - 1.0; // -1 to 1 NDC
    vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
    p *= 1000.0; // Times far clip plane value
    p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world

    // Direction from camera to this point
    vec3 rayDir = normalize(p.xyz - u_eye);

    // Compute corresponding uv
    vec2 uv = sphereToUV(rayDir);

    // Generate a disturbance based on time to simulate an effect of moving clouds
    vec2 offset = vec2(0.0);
    // Get a noise value in the range [-1, 1]
    // by using Worley noise as the noise basis of FBM
    if (ADD_CLOUD) {
        offset = vec2(worleyFBM(rayDir));
        offset *= 2.0;
        offset -= vec2(1.0);
    }

    // Compute a gradient from the bottom of the sky-sphere to the top
    vec3 daytimeColor = uvToDaytime(uv + offset * 0.1);
    vec3 sunsetColor = uvToSunset(uv + offset * 0.1);
    vec3 duskColor = uvToDusk(uv);

    // Add a glowing sun in the sky
    // Move around the world
    vec3 sunDir = normalize(vec3(cos(u_time * SUN_VELOCITY), sin(u_time * SUN_VELOCITY), 0.f));

    // Indicator for day:1 or night:-1
    float day_night_ratio = sin(u_time * SUN_VELOCITY);

    vec3 sunColor;

    // Day and night color cycle
    vec3 sky_color;
    // Daytime
    if (day_night_ratio > SUNSET_LEN) {
        sky_color = daytimeColor;

        sunColor = sun[0];
    }
    // Night
    else if (day_night_ratio < -SUNSET_LEN) {
        sky_color = duskColor;

        sunColor = sun[2];
    }
    // Sunrise or sunset
    else {
        sky_color = mix(duskColor, daytimeColor, (day_night_ratio + SUNSET_LEN) / (SUNSET_LEN * 2.f));

        if (day_night_ratio > 0) {
            float smooth_t = smoothstep(0.f, 1.f, abs(day_night_ratio) / SUNSET_LEN);
            sunColor = mix(sun[1], sun[0], smooth_t);
        }
        else {
            float smooth_t = smoothstep(0.f, 1.f, abs(day_night_ratio) / (SUNSET_LEN));
            sunColor = mix(sun[1], sun[2], smooth_t);
        }
    }

    vec3 out_color_sunset;

    float sunSize = 30;
    float angle = acos(dot(rayDir, sunDir)) * 360.0 / PI;
    // If the angle between our ray dir and vector to center of sun
    // is less than the threshold, then we're looking at the sun
    vec3 out_col;

    if(angle < sunSize) {
        // Full center of sun
        if(angle < 7.5) {
            out_col = sunColor;
        }
        // Corona of sun, mix with sky color
        else {
            out_col = mix(sunColor, sky_color, (angle - 7.5) / 22.5);

            // Sunrise or sunset
            if (day_night_ratio <= SUNSET_LEN && day_night_ratio >= -SUNSET_LEN) {
                out_color_sunset = mix(sunColor, sunsetColor, (angle - 7.5) / 22.5);

                float smooth_t = smoothstep(0.f, 1.f, 1.f - abs(day_night_ratio) / SUNSET_LEN);
                out_col = mix(out_col, out_color_sunset, smooth_t);
            }
        }
    }
    // Otherwise our ray is looking into just the sky
    else {
        out_col = sky_color;

        // Sunrise or sunset
        if (day_night_ratio <= SUNSET_LEN && day_night_ratio >= -SUNSET_LEN) {
            float raySunDot = dot(rayDir, sunDir);

            if(raySunDot > SUNSET_THRESHOLD) {
                out_color_sunset = sunsetColor;
            }
            // Any dot product between 0.75 and -0.1 is a LERP b/t sunset and sky color
            else if(raySunDot > DUSK_THRESHOLD) {
                float t = (raySunDot - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
                out_color_sunset = mix(sunsetColor, sky_color, t);
            }
            // Any dot product <= -0.1 are pure sky color
            else {
                out_color_sunset = sky_color;
            }

            float smooth_t = smoothstep(0.f, 1.f, 1.f - abs(day_night_ratio) / SUNSET_LEN);
            out_col = mix(out_col, out_color_sunset, smooth_t);
        }
    }

    outColor = vec4(out_col, 1);
}
