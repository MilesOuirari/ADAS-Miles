#ifndef HMI_SHADER_STORE_HPP
#define HMI_SHADER_STORE_HPP

#include <GL/glew.h>
#include <string>
#include <iostream>

namespace adas::hmi {

class ShaderStore {
public:
    GLuint carProgram;
    GLuint gridProgram;
    GLuint uiProgram;
    GLuint pathProgram;  // NEW: Ribbon Path
    GLuint glassProgram; // NEW: UI Backgrounds
    GLuint barProgram;   // NEW: Velocity Bar
    GLuint signProgram;  // NEW: Traffic Sign Procedural
    GLuint textureProgram; // NEW: Image Rendering
    GLuint bboxProgram; // NEW: Bounding Box Wireframe

    void init() {
        carProgram = createProgram(vertexCar, fragCar);
        gridProgram = createProgram(vertexGrid, fragGrid);
        uiProgram = createProgram(vertexUI, fragUI);
        pathProgram = createProgram(vertexPath, fragPath);
        glassProgram = createProgram(vertexUI, fragGlass);
        barProgram = createProgram(vertexBar, fragBar);
        signProgram = createProgram(vertexBar, fragSign);
        textureProgram = createProgram(vertexBar, fragTexture);
        bboxProgram = createProgram(vertexCar, fragBBox); // Uses 3D vertex + simple frag
    }

private:
    // ... (Existing Car/Grid shaders kept implicitly via merge, but showing new ones below)

    // --- 4. Ribbon Path Shader (Scrolling) ---
    const char* vertexPath = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord; // Used for scrolling
        
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec2 TexCoord;
        
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    const char* fragPath = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        
        uniform float time;
        uniform vec3 color;
        
        void main() {
            // Scrolling pattern
            float scroll = TexCoord.y - time * 2.0;
            float pattern = fract(scroll * 5.0); // 5 dashes
            float alpha = smoothstep(0.0, 0.2, pattern) * (1.0 - smoothstep(0.5, 0.7, pattern));
            
            // Fade out distance
            float distFade = 1.0 - TexCoord.y; // Assume y 0..1 mapping to distance
            
            FragColor = vec4(color, alpha * distFade * 0.8);
        }
    )";

    // --- 5. Glass UI Shader ---
    const char* fragGlass = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec4 uColor;     // Base color (usually dark blue/black)
        uniform float uBlur;     // Fake blur strength (just alpha/noise in this simple version)
        
        void main() {
            // Simple glass effect: solid color with low alpha + border
            // Real blur requires multi-pass. We simulate "frosted" look via color + noise if needed.
            // For now, sleek semi-transparent.
            FragColor = uColor; 
        }
    )";
    // --- 1. Car Shader (Tech/Cyberpunk Look) ---
    // Rim lighting + metallic gloss
    const char* vertexCar = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 Normal;
        out vec3 FragPos;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
        }
    )";

    const char* fragCar = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 Normal;
        in vec3 FragPos;
        
        uniform vec3 viewPos;
        uniform vec3 color;
        
        void main() {
            // Material properties
            vec3 ambient = 0.5 * color;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3)); // Fixed overhead light
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * color;
            
            // Specular
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = vec3(1.0) * spec; // White highlight
            
            // Rim Light (Fresnel-ish) -> "Tech" look
            float rim = 1.0 - max(dot(viewDir, norm), 0.0);
            rim = pow(rim, 3.0) * 0.8;
            vec3 rimColor = vec3(0.0, 1.0, 1.0) * rim; // Cyan rim
            
            vec3 result = ambient + diffuse + specular + rimColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    // --- 2. Infinite Grid/Road Shader ---
    const char* vertexGrid = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in float aAlpha; // Vertex specific alpha/fade
        
        uniform mat4 view;
        uniform mat4 projection;
        
        out float Alpha;
        out vec3 WorldPos;
        
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            WorldPos = aPos;
            Alpha = aAlpha;
        }
    )";

    const char* fragGrid = R"(
        #version 330 core
        out vec4 FragColor;
        in float Alpha;
        in vec3 WorldPos;

        uniform vec3 color;
        
        void main() {
            // Distance fade fog
            float dist = length(WorldPos.xz); // Fog from center? Or depth?
            // Simple logic: Use vertex alpha
            FragColor = vec4(color, Alpha);
        }
    )";

    // --- 3. UI Shader ---
    const char* vertexUI = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos; 
        uniform mat4 projection; 
        uniform mat4 model;      
        void main() {
            gl_Position = projection * model * vec4(aPos, 1.0);
        }
    )";
    
    // --- 6. High-Tech Bar Shader (NEW) ---
    const char* vertexBar = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal; // Used as UV
        
        uniform mat4 projection;
        uniform mat4 model;
        
        out vec2 UV;
        
        void main() {
            gl_Position = projection * model * vec4(aPos, 1.0);
            UV = aNormal.xy;
        }
    )";

    const char* fragBar = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 UV;
        
        uniform float uProgress; // 0.0 to 1.0
        uniform float uTime;
        
        void main() {
            // Skew UV for speed look?
            float x = UV.x;
            
            // Segments (20 blocks)
            float segments = 20.0;
            float gap = 0.1; // 10% gap
            float block = fract(x * segments);
            if(block > (1.0 - gap)) discard; // Gap
            
            // Active vs Inactive
            float isOn = step(x, uProgress);
            
            // Colors
            vec3 cActive = mix(vec3(0.0, 1.0, 1.0), vec3(1.0, 0.0, 1.0), x); // Cyan -> Magenta
            vec3 cInactive = vec3(0.1, 0.1, 0.2);
            
            vec3 color = mix(cInactive, cActive, isOn);
            
            // Glow active segments
            if(isOn > 0.5) {
                // Pulse or Shimmer
                float shimmer = smoothstep(0.0, 0.2, abs(fract(UV.x - uTime) - 0.5));
                color += vec3(0.2) * shimmer;
                color *= 1.5; // Brightness boost (HDR-like)
            }
            
            FragColor = vec4(color, 0.9);
        }
    )";
    
    // --- 7. Traffic Sign Shader (Professional) ---
    const char* fragSign = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 UV;
        
        uniform int uType;   // 0=Limit, 1=Stop, 2=Yield, 5=TrafficLight
        uniform int uValue;  // 30, 50, 80, 100, 120
        uniform float uTime;
        
        // --- SDF Primitives ---
        float sdCircle(vec2 p, float r) { return length(p) - r; }
        float sdBox(vec2 p, vec2 b) { vec2 d = abs(p)-b; return length(max(d,0.0)) + min(max(d.x,d.y),0.0); }
        float sdOctagon(vec2 p, float r) {
            const vec3 k = vec3(-0.9238795325, 0.3826834323, 0.4142135623 );
            p = abs(p);
            p -= 2.0*min(dot(vec2( k.x,k.y),p),0.0)*vec2( k.x,k.y);
            p -= 2.0*min(dot(vec2(-k.x,k.y),p),0.0)*vec2(-k.x,k.y);
            return length(p - vec2(clamp(p.x,-k.z*r,k.z*r), r))*sign(p.y - r);
        }
        
        float DrawDigit(vec2 p, int n) {
            float d = 1.0;
            // Simplified Segment Logic for 3, 5, 8, 0, 1, 2
            // 3x5 Grid roughly
            // Return < 0 inside
            
            if (n == 0) { // O
                float outer = sdCircle(p, 0.35);
                float inner = sdCircle(p, 0.20);
                d = max(outer, -inner);
                // Cut middle to make it look like 0 not O? No loop is fine.
            }
            else if (n == 1) { // I
                 d = sdBox(p, vec2(0.08, 0.35));
            }
            else if (n == 3) { // 3
                // Two circles trimmed?
                // Construct from bars
                // |
                // -
                // |
                // -
                // Use SDF union of boxes
                // This is getting complex for GLSL string.
                // Fallback: Just squares for segments
            }
            else if (n == 5) {
                // Top Box
                float b1 = sdBox(p - vec2(0.0, 0.3), vec2(0.25, 0.05));
                float b2 = sdBox(p - vec2(-0.2, 0.15), vec2(0.05, 0.2));
                float b3 = sdBox(p - vec2(0.0, 0.0), vec2(0.25, 0.05)); 
                float b4 = sdBox(p - vec2(0.2, -0.15), vec2(0.05, 0.2));
                float b5 = sdBox(p - vec2(0.0, -0.3), vec2(0.25, 0.05));
                d = min(min(min(min(b1, b2), b3), b4), b5);
            }
            else if (n == 8) {
               float outer = sdBox(p, vec2(0.25, 0.35));
               float inner1 = sdBox(p-vec2(0, 0.18), vec2(0.15, 0.1));
               float inner2 = sdBox(p-vec2(0, -0.18), vec2(0.15, 0.1));
               d = max(outer, -min(inner1, inner2));
            }
            
            // Default block for unknown
            if(d > 0.9) d = sdBox(p, vec2(0.2, 0.3));
            
            return d;
        }

        void main() {
            vec2 p = UV * 2.0 - 1.0; // -1 to 1
            // Aspect Ratio correction? wrapper handles it via scale usually.
            
            vec4 col = vec4(0.0);
            
            if (uType == 0) { // Speed Limit
                float dist = sdCircle(p, 0.9);
                float borderResult = smoothstep(0.02, 0.0, abs(dist + 0.1) - 0.1); 
                float fill = smoothstep(0.01, 0.0, dist);
                
                // White Base
                col = mix(col, vec4(1.0), fill);
                // Red Border
                col = mix(col, vec4(0.8, 0.0, 0.0, 1.0), smoothstep(0.02, 0.0, abs(dist) - 0.1));
                
                // Digits
                // Shift p for digits
                vec2 p1 = p - vec2(-0.35, 0.0);
                vec2 p2 = p - vec2(0.35, 0.0);
                
                int d1 = uValue / 10;
                int d2 = uValue % 10;
                
                float dig1 = DrawDigit(p1*1.5, d1); // Scale up
                float dig2 = DrawDigit(p2*1.5, d2);
                
                float text = min(dig1, dig2);
                col = mix(col, vec4(0.0, 0.0, 0.0, 1.0), smoothstep(0.01, -0.01, text));
                
                // Cut outside circle
                col.a *= smoothstep(0.01, 0.0, dist); 
            }
            else if (uType == 1) { // STOP
                float dist = sdOctagon(p, 0.9);
                float fill = smoothstep(0.01, 0.0, dist);
                col = mix(col, vec4(0.8, 0.0, 0.0, 1.0), fill);
                
                float border = smoothstep(0.02, 0.0, abs(dist) - 0.03);
                col = mix(col, vec4(1.0), border);
                
                // "STOP" Text - Simplified to a Bar for now
                float bar = sdBox(p, vec2(0.6, 0.15));
                col = mix(col, vec4(1.0), smoothstep(0.01, 0.0, bar));
            }
            else if (uType == 5) { // Traffic Light
                // Box
                float box = sdBox(p, vec2(0.4, 0.9));
                float fill = smoothstep(0.01, 0.0, box);
                col = mix(col, vec4(0.1, 0.1, 0.1, 1.0), fill);
                
                // Lights
                float r = sdCircle(p - vec2(0, 0.6), 0.2);
                float y = sdCircle(p - vec2(0, 0.0), 0.2); // Yellow/Amb
                float g = sdCircle(p - vec2(0, -0.6), 0.2);
                
                vec3 lightCol = vec3(0.2); // Off
                if (uValue == 0) lightCol = vec3(1.0, 0.0, 0.0); // Red
                
                // Glow
                float gR = smoothstep(0.02, 0.0, r);
                col = mix(col, vec4(lightCol, 1.0), gR);
            }
            
            FragColor = col;
        }
    )";
    
    // --- 8. Texture Shader (Image Support) ---
    const char* fragTexture = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 UV;
        
        uniform sampler2D uTex;
        uniform vec4 uTint;
        
        void main() {
            // Flip Y for STB Image (Top-Left origin vs OpenGL Bottom-Left)
            vec4 texColor = texture(uTex, vec2(UV.x, 1.0 - UV.y));
            FragColor = texColor * uTint;
        }
    )";
    
    const char* fragUI = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 uColor;
        void main() {
            FragColor = uColor;
        }
    )";

    // Bounding Box Fragment Shader (Simple solid color)
    const char* fragBBox = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";

    GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        
        GLint success;
        glGetShaderiv(s, GL_COMPILE_STATUS, &success);
        if(!success) {
            char infoLog[512];
            glGetShaderInfoLog(s, 512, nullptr, infoLog);
            std::cerr << "SHADER ERROR: " << infoLog << std::endl;
        }
        return s;
    }

    GLuint createProgram(const char* vSrc, const char* fSrc) {
        GLuint vs = compile(GL_VERTEX_SHADER, vSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fSrc);
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        
        GLint success;
        glGetProgramiv(p, GL_LINK_STATUS, &success);
        if(!success) {
            char infoLog[512];
            glGetProgramInfoLog(p, 512, nullptr, infoLog);
            std::cerr << "PROGRAM ERROR: " << infoLog << std::endl;
        }
        
        glDeleteShader(vs);
        glDeleteShader(fs);
        return p;
    }
};

} // namespace adas::hmi

#endif // HMI_SHADER_STORE_HPP
