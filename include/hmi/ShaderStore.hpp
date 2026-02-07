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

    void init() {
        carProgram = createProgram(vertexCar, fragCar);
        gridProgram = createProgram(vertexGrid, fragGrid);
        uiProgram = createProgram(vertexUI, fragUI);
        pathProgram = createProgram(vertexPath, fragPath);
        glassProgram = createProgram(vertexUI, fragGlass);
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
        layout(location = 0) in vec2 aPos;
        uniform mat4 projection; // Ortho
        uniform mat4 model;      // Transform
        void main() {
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
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
