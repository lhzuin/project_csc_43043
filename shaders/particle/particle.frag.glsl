#version 330 core

in vec3 world_position;
in vec3 world_normal;

out vec4 FragColor;

// Uniforms (sent from C++)
uniform vec3 light;                // world‐space light position
uniform mat4 view;                 // camera view (to reconstruct camera pos)

uniform vec3 particle_color;       // base color
uniform float alpha;               // transparency

uniform vec3 fog_color;            // fog mixture
uniform float fog_distance_max;    // distance for full fog

void main()
{
    // Simple Lambert + ambient
    vec3 N = normalize(world_normal);
    // Choose point light: direction from fragment to light
    vec3 L = normalize(light - world_position);
    float diff = max(dot(N, L), 0.0);
    float Ka = 0.3;    // ambient coefficient
    float Kd = 0.7;    // diffuse coefficient
    vec3 baseColor = particle_color * (Ka + Kd * diff);

    // Compute camera position from view matrix:
    mat3 O = transpose(mat3(view));
    vec3 camPos = -O * (view * vec4(0,0,0,1)).xyz;
    float dist = length(camPos - world_position);
    float fogFactor = clamp(dist / fog_distance_max, 0.0, 1.0);
    vec3 finalColor = mix(baseColor, fog_color, fogFactor);

    FragColor = vec4(finalColor, alpha);
}