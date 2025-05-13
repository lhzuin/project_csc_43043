#version 330 core

// ————— Inputs from your vertex shader —————
in struct fragment_data {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} fragment;

// ————— Output —————
layout(location = 0) out vec4 FragColor;

// ————— Original uniforms —————
uniform sampler2D image_texture;
uniform mat4 view;
uniform vec3 light;               // your point-light position

// ————— Flattened “material” parameters —————
uniform vec3   mat_color;
uniform float  mat_alpha;
uniform float  mat_ambient;
uniform float  mat_diffuse;
uniform float  mat_specular;
uniform float  mat_specular_exponent;
uniform bool   use_texture;
uniform bool   texture_inverse_v;
uniform bool   two_sided;

// ————— New “sun” (directional) light —————
uniform vec3   sunDir;            // normalized, points _toward_ the sun
uniform vec3   sunColor;          // RGB intensity of the sun

// ————— Caustics & flow maps —————
uniform sampler2D causticMap;     // e.g. a looping caustic sequence
uniform sampler2D flowMap;        // noise or flow-vector texture

// ————— Effect‐control uniforms —————
uniform float time;               // pass your global time
uniform float flowScale;          // scale of the flow look-ups
uniform float flowSpeed;          // how fast the flow scrolls
uniform float flowAmplitude;      // how much it displaces
uniform float causticScale;       // tiling of the caustics
uniform float causticIntensity;   // strength of the caustic bright spots
uniform float fogDensity;         // controls fog “murkiness”
uniform vec3  fogColor;           // the water color to fog-blend to

void main()
{
    // ——— compute camera position from view matrix ———
    mat3 M = transpose(mat3(view));
    vec3 camPos = -M * vec3(view * vec4(0,0,0,1));

    // ——— normalize & (optionally) flip the normal ———
    vec3 N = normalize(fragment.normal);
    if (two_sided && !gl_FrontFacing) N = -N;
    vec3 V = normalize(camPos - fragment.position);

    // ——— sample (or skip) the texture ———
    vec2 uv = fragment.uv;
    if (texture_inverse_v) uv.y = 1.0 - uv.y;
    vec4 texel = use_texture 
        ? texture(image_texture, uv) 
        : vec4(1.0);
    vec3 baseColor = fragment.color * mat_color * texel.rgb;

    // ——— point‐light (your original “light”) ———
    vec3 Lp = normalize(light - fragment.position);
    float diff_p = max(dot(N,Lp),0.0);
    float spec_p = (diff_p>0.0)
      ? pow(max(dot(reflect(-Lp,N), V),0.0), mat_specular_exponent)
      : 0.0;
    vec3 shade_p = mat_ambient * baseColor
                 + mat_diffuse * diff_p * baseColor
                 + mat_specular * spec_p * vec3(1.0);

    // ——— directional “sun” light ———
    vec3 Ld = normalize(-sunDir);
    float diff_d = max(dot(N,Ld),0.0);
    float spec_d = (diff_d>0.0)
      ? pow(max(dot(reflect(sunDir,N), V),0.0), mat_specular_exponent)
      : 0.0;
    vec3 shade_d = mat_ambient * baseColor * sunColor
                 + mat_diffuse * diff_d   * baseColor * sunColor
                 + mat_specular * spec_d  * sunColor;

    // ——— combine both lights ———
    vec3 color_shading = shade_p + shade_d;

    // ——— add projected caustics ———
    vec2 cuv = fragment.position.xz * causticScale 
             + vec2(time * flowSpeed * 0.5);
    float caustic = texture(causticMap, fract(cuv)).r 
                  * causticIntensity;
    color_shading += caustic * baseColor;

    // ——— distort sample‐position for fog by the flow field ———
    vec2 fuv = fragment.position.xz * flowScale 
             + vec2(time * flowSpeed);
    vec2 flow = texture(flowMap, fract(fuv)).rg * 2.0 - 1.0;
    vec3 displaced = fragment.position 
                   + vec3(flow.x, 0.0, flow.y) * flowAmplitude;

    // ——— exponential fog ———
    float dist = length(camPos - displaced);
    float fogFactor = clamp(1.0 - exp(-fogDensity * dist), 0.0, 1.0);

    vec3 finalColor = mix(color_shading, fogColor, fogFactor);
    FragColor = vec4(finalColor, mat_alpha * texel.a);
}