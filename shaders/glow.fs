#version 330

#define MAX_LINES 100

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec2 lineA[MAX_LINES];
uniform vec2 lineB[MAX_LINES];
uniform int numLines;
uniform float time; // Add time uniform for animation

// Simple 2D noise function
float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p.x) * 43758.5453123) * 2.0 - 1.0;
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(mix(hash(i + vec2(0.0, 0.0)), 
                   hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), 
                   hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    float glowIntensity = 0.0;
    float lineThickness = 2.0; // Adjust this value to change the line thickness
    float fadeDistance = 30.0; // Adjust this value to change the fade distance

    for (int i = 0; i < numLines; i++) {
        vec2 pointA = lineA[i] * 2.0;
        vec2 pointB = lineB[i] * 2.0;

        // Calculate the distance from the fragment to the line
        vec2 lineDir = pointB - pointA;
        float lineLength = length(lineDir);
        lineDir = normalize(lineDir);

        vec2 fragToLineStart = fragCoord - pointA;
        float projection = dot(fragToLineStart, lineDir);
        projection = clamp(projection, 0.0, lineLength);

        vec2 closestPoint = pointA + lineDir * projection;
        float distanceToLine = length(fragCoord - closestPoint);

        // Accumulate glow intensity
        float glow = smoothstep(lineThickness + fadeDistance, lineThickness, distanceToLine);
        glowIntensity += glow;
    }

    // Normalize and colorize the glow
    glowIntensity = clamp(glowIntensity, 0.0, 10.0);
    
    // Adding trippy color effects
    float noiseValue = noise(fragCoord * 0.5 + time * 0.1);
    vec3 baseColor = vec3(1.0, 0.5, 1.0);
    vec3 trippyColor = vec3(sin(time + fragCoord.x * 0.1), sin(time + fragCoord.y * 0.1), cos(time));
    
    vec4 glowColor = vec4(mix(baseColor, trippyColor, noiseValue), 1.0) * glowIntensity;

    finalColor = glowColor;
}
