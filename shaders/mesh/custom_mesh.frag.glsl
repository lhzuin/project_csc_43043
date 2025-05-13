#version 330 core

in struct fragment_data {
    vec3 position;  
    vec3 normal;    
    vec3 color;     
    vec2 uv;        
} fragment;

layout(location=0) out vec4 FragColor;

// ——— your existing uniforms ———
uniform sampler2D image_texture;
uniform mat4 view;
uniform vec3 light;             // point-light position
uniform vec3 lightColor;        // point-light color
uniform material_structure {
    vec3 color;
    float alpha;
    phong_structure {
        float ambient;
        float diffuse;
        float specular;
        float specular_exponent;
    } phong;
    texture_settings_structure {
        bool use_texture;
        bool texture_inverse_v;
        bool two_sided;
    } texture_settings;
} material;

// ——— new uniforms ———
uniform vec3 sunDir;            // normalized direction *towards* sun (e.g. vec3(0,1,1))
uniform vec3 sunColor;          // color/intensity of sun
uniform sampler2D causticMap;   // animated caustic texture (e.g. 128×128 loop)
uniform sampler2D flowMap;      // noise/flow texture
uniform float time;            

// scale & strength parameters
uniform float flowScale;        
uniform float flowSpeed;        
uniform float flowAmplitude;    
uniform float causticScale;     
uniform float causticIntensity; 
uniform float fogDensity;       
uniform vec3  fogColor;         

void main()
{
    // ——— camera position in world space ———
    mat3 O = transpose(mat3(view));
    vec3 last_col = vec3(view * vec4(0,0,0,1));
    vec3 camera_position = -O * last_col;

    // ——— normal & view vector ———
    vec3 N = normalize(fragment.normal);
    if (material.texture_settings.two_sided && !gl_FrontFacing)
        N = -N;
    vec3 V = normalize(camera_position - fragment.position);

    // ——— base color from vertex, material & texture ———
    vec2 uv_image = fragment.uv;
    if (!material.texture_settings.use_texture) {
        // ignore texture
    } else {
        if (material.texture_settings.texture_inverse_v)
            uv_image.y = 1.0 - uv_image.y;
    }
    vec4 tex = material.texture_settings.use_texture
        ? texture(image_texture, uv_image)
        : vec4(1.0);
    vec3 baseColor = fragment.color * material.color * tex.rgb;

    // ——— POINT LIGHT (your existing “light” uniform) ———
    vec3 Lp = normalize(light - fragment.position);
    float diff_p = max(dot(N, Lp), 0.0);
    float spec_p = 0.0;
    if (diff_p > 0.0) {
        vec3 R = reflect(-Lp, N);
        spec_p = pow(max(dot(R, V), 0.0), material.phong.specular_exponent);
    }
    vec3 amb_p  = material.phong.ambient * baseColor * lightColor;
    vec3 dif_p  = material.phong.diffuse * diff_p * baseColor * lightColor;
    vec3 spc_p  = material.phong.specular * spec_p * lightColor;

    // ——— DIRECTIONAL “SUN” LIGHT ———
    vec3 Ld = normalize(-sunDir);
    float diff_d = max(dot(N, Ld), 0.0);
    float spec_d = 0.0;
    if (diff_d > 0.0) {
        vec3 R = reflect(sunDir, N);
        spec_d = pow(max(dot(R, V), 0.0), material.phong.specular_exponent);
    }
    vec3 amb_d = material.phong.ambient * baseColor * sunColor;
    vec3 dif_d = material.phong.diffuse * diff_d * baseColor * sunColor;
    vec3 spc_d = material.phong.specular * spec_d * sunColor;

    // ——— COMBINED SHADING ———
    vec3 color_shading = amb_p + dif_p + spc_p
                       + amb_d + dif_d + spc_d;

    // ——— PROJECTED CAUSTICS ———
    vec2 causticUV = fragment.position.xz * causticScale 
                     + vec2(time * flowSpeed * 0.5);
    float caustic = texture(causticMap, fract(causticUV)).r
                    * causticIntensity;
    color_shading += caustic * baseColor;

    // ——— FLOW-BASED DISTORTION (“current”) ———
    vec2 flowUV = fragment.position.xz * flowScale 
                  + vec2(time * flowSpeed);
    vec2 flowRG = texture(flowMap, fract(flowUV)).rg * 2.0 - 1.0;
    vec3 displacedPos = fragment.position
                        + vec3(flowRG.x, 0.0, flowRG.y) * flowAmplitude;

    // ——— EXPONENTIAL FOG ———
    float dist = length(camera_position - displacedPos);
    float fogFactor = 1.0 - exp(-fogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(color_shading, fogColor, fogFactor);
    FragColor = vec4(finalColor, material.alpha * tex.a);
}