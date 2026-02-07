#ifndef HUD_RENDERER_HPP
#define HUD_RENDERER_HPP

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include <array>

#include "DataModels.hpp"

namespace adas::hud {

constexpr size_t kMaxVertices = 4096; // Increased buffer

class Renderer {
public:
    Renderer() : shader_program_(0), vao_(0), vbo_(0) {}

    ~Renderer() {
        if (shader_program_) glDeleteProgram(shader_program_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
        if (vbo_) glDeleteBuffers(1, &vbo_);
    }

    bool init() {
        if (!createShaders()) return false;
        createBuffers();
        return true;
    }

    // --- HUD Elements ---

    void drawSpeedometer(float speed_kph) {
        // Draw "SPEED" label placeholder (static text hard to do proc, stick to number)
        // Draw 3 Digits centered bottom
        float center_x = 0.0f;
        float bottom_y = -0.8f;
        float digit_size = 0.15f;
        
        int speed = static_cast<int>(speed_kph);
        if (speed > 999) speed = 999;
        
        setColor(0.0f, 1.0f, 0.8f, 1.0f); // Cyan
        
        // Hundreds
        if (speed >= 100) drawDigit(speed / 100, center_x - 1.2f * digit_size, bottom_y, digit_size);
        // Tens
        if (speed >= 10) drawDigit((speed / 10) % 10, center_x, bottom_y, digit_size);
        else drawDigit(0, center_x, bottom_y, digit_size); // Always show 00 or 0
        // Units
        drawDigit(speed % 10, center_x + 1.2f * digit_size, bottom_y, digit_size);
        
        // Unit "km/h" - simplified as a small underline
        drawRect(center_x - 1.5f * digit_size, bottom_y - 0.05f, center_x + 1.5f * digit_size, bottom_y - 0.04f);
    }

    void drawObjects(const ObjectList& objects) {
        if (!objects.is_valid) return;
        
        for (size_t i = 0; i < objects.count; ++i) {
            const auto& obj = objects.objects[i];
            drawBoundingBox3D(obj.x, obj.y, obj.width, obj.height);
        }
    }

    void drawTTCWarning(float ttc_seconds) {
        if (ttc_seconds < 2.5f && ttc_seconds > 0.0f) {
            // Flash effect based on time (caller handles flash logic or we use static)
            setColor(1.0f, 0.0f, 0.0f, 0.6f); 
            // Big red brackets around center
            float w = 0.3f; float h = 0.2f;
            drawRect(-w, -h, -w+0.02f, h); // Left
            drawRect(w-0.02f, -h, w, h);   // Right
            
            // "STOP" or "!" - simplify to "!"
            drawRect(-0.02f, -h + 0.1f, 0.02f, h - 0.05f); // Top part
            drawRect(-0.02f, -h, 0.02f, -h + 0.05f);       // Dot
        }
    }

    void drawLaneGlow(const LaneData& lane) {
        if (!lane.is_valid || lane.point_count < 2) return;

        setColor(0.2f, 0.6f, 1.0f, 0.5f); // Cyan transparent

        // We use a simple perspective projection helper locally
        // to map World (x, y) -> NDC (x, y)
        // Camera assumption: 1.5m height, looking straight ahead
        
        std::vector<float> verts;
        verts.reserve(lane.point_count * 4); // Line strip

        for (size_t i = 0; i < lane.point_count; ++i) {
             float sx, sy;
             if (projectWorldToScreen(lane.points[i].x, lane.points[i].y, sx, sy)) {
                 verts.push_back(sx);
                 verts.push_back(sy);
             }
        }
        
        if (verts.size() < 4) return;

        uploadVertices(verts.data(), verts.size());
        glDrawArrays(GL_LINE_STRIP, 0, verts.size() / 2);
    }

private:
    GLuint shader_program_;
    GLuint vao_;
    GLuint vbo_;
    float current_color_[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // --- Helpers ---

    bool projectWorldToScreen(float world_x, float world_y, float& screen_x, float& screen_y) {
        // Simple manual perspective:
        // x' = x / z, y' = y / z (roughly)
        // Camera at (0, 0, 0), Looking +Y (or +Z).
        // Let's assume World Y is depth (forward), World X is lateral.
        // Camera height offset?
        
        float depth = world_y;
        if (depth < 1.0f) return false; // Behind or too close

        float fov_scale = 1.5f; // Field of View factor
        
        // X projection:
        screen_x = (world_x / depth) * fov_scale;
        
        // Y projection (Height):
        // Assume road is flat at Y_height = -1.5m relative to cam
        float cam_height = 1.5f; 
        // We project the "ground" y
        screen_y = (-cam_height / depth) * fov_scale; 

        // Adjust screen_y to center horizon (Horizon is at 0.0 roughly if looking straight)
        // In this formula, infinite depth -> 0. Close depth -> negative.
        // So horizon is 0.0. Screen bottom is -1.0.
        
        return true;
    }

    void drawBoundingBox3D(float x, float y, float w, float h) {
        // Project 4 corners of the back face (at distance y)
        // Bounding box: Center (x, y), Width w, Height h
        
        float left = x - w/2;
        float right = x + w/2;
        float bottom = 0.0f; // On road
        float top = h;       // Object height

        // 4 Points in world space
        // BL: left, y, 0 (ground)
        // BR: right, y, 0
        // TL: left, y, top
        // TR: right, y, top
        // (Simplified: we ignore depth thickness for now, just a face plate)

        float sx_bl, sy_bl, sx_br, sy_br, sx_tl, sy_tl, sx_tr, sy_tr;
        
        // For ground points, use standard projection
        // For top points, we need to handle "height" in the projection
        // Re-using logic: screen_y = (world_height - cam_height) / depth * scale
        
        auto project = [&](float wx, float wy, float wz, float& sx, float& sy) {
             if (wy < 1.0f) return false;
             float fov = 1.5f;
             sx = (wx / wy) * fov;
             sy = ((wz - 1.5f) / wy) * fov; // 1.5f is cam height
             return true;
        };

        bool p1 = project(left, y, 0, sx_bl, sy_bl);
        bool p2 = project(right, y, 0, sx_br, sy_br);
        bool p3 = project(left, y, h, sx_tl, sy_tl);
        bool p4 = project(right, y, h, sx_tr, sy_tr);

        if (!p1 || !p2 || !p3 || !p4) return;

        setColor(1.0f, 0.0f, 1.0f, 0.8f); // Magenta
        
        // Draw Box lines
        float lines[] = {
            sx_bl, sy_bl, sx_br, sy_br,
            sx_br, sy_br, sx_tr, sy_tr,
            sx_tr, sy_tr, sx_tl, sy_tl,
            sx_tl, sy_tl, sx_bl, sy_bl
        };
        
        uploadVertices(lines, 8);
        glDrawArrays(GL_LINES, 0, 8);
        
        // Add distance text (simulated as small dots/bar above)
        // ...
    }

    void drawDigit(int digit, float x, float y, float size) {
        // 7-segment representation
        //   A
        // F   B
        //   G
        // E   C
        //   D
        
        bool segs[7]; // A, B, C, D, E, F, G
        // defaults
        for(int i=0; i<7; ++i) segs[i] = false;

        switch(digit) {
            case 0: segs[0]=1; segs[1]=1; segs[2]=1; segs[3]=1; segs[4]=1; segs[5]=1; break;
            case 1: segs[1]=1; segs[2]=1; break;
            case 2: segs[0]=1; segs[1]=1; segs[6]=1; segs[4]=1; segs[3]=1; break;
            case 3: segs[0]=1; segs[1]=1; segs[6]=1; segs[2]=1; segs[3]=1; break;
            case 4: segs[5]=1; segs[6]=1; segs[1]=1; segs[2]=1; break;
            case 5: segs[0]=1; segs[5]=1; segs[6]=1; segs[2]=1; segs[3]=1; break;
            case 6: segs[0]=1; segs[5]=1; segs[4]=1; segs[3]=1; segs[2]=1; segs[6]=1; break;
            case 7: segs[0]=1; segs[1]=1; segs[2]=1; break;
            case 8: for(int i=0; i<7; ++i) segs[i]=1; break;
            case 9: segs[0]=1; segs[1]=1; segs[2]=1; segs[3]=1; segs[5]=1; segs[6]=1; break;
        }

        // Coords relative to x,y (bottom-left)
        // Width/Height logic
        float w = size * 0.5f;
        float h = size; 
        float t = size * 0.1f; // thickness? No using lines.

        std::vector<float> lines;
        
        auto addLine = [&](float x1, float y1, float x2, float y2) {
            lines.push_back(x + x1); lines.push_back(y + y1);
            lines.push_back(x + x2); lines.push_back(y + y2);
        };

        if (segs[0]) addLine(0, h, w, h);       // A
        if (segs[1]) addLine(w, h, w, h/2);     // B
        if (segs[2]) addLine(w, h/2, w, 0);     // C
        if (segs[3]) addLine(0, 0, w, 0);       // D
        if (segs[4]) addLine(0, 0, 0, h/2);     // E
        if (segs[5]) addLine(0, h/2, 0, h);     // F
        if (segs[6]) addLine(0, h/2, w, h/2);   // G

        if (lines.empty()) return;
        uploadVertices(lines.data(), lines.size());
        glDrawArrays(GL_LINES, 0, lines.size()/2);
    }

    // --- Base Graphics ---

    void setColor(float r, float g, float b, float a) {
        current_color_[0] = r; current_color_[1] = g; current_color_[2] = b; current_color_[3] = a;
        glUseProgram(shader_program_);
        GLint color_loc = glGetUniformLocation(shader_program_, "uColor");
        glUniform4f(color_loc, r, g, b, a);
    }

    void drawRect(float x1, float y1, float x2, float y2) {
        float vertices[] = { x1, y1, x2, y1, x1, y2, x1, y2, x2, y1, x2, y2 };
        uploadVertices(vertices, 12);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void uploadVertices(const float* data, size_t count) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        // In production, we'd map buffer or use subdata properly with tracking.
        // For here, strict overwrite for immediate draw.
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(float), data);
        
        glUseProgram(shader_program_);
        GLint pos_loc = glGetAttribLocation(shader_program_, "aPos");
        glEnableVertexAttribArray(pos_loc);
        glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    }

    bool createShaders() {
        const char* vertex_src = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* fragment_src = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main() {
                FragColor = uColor;
            }
        )";

        GLuint vs = compileShader(GL_VERTEX_SHADER, vertex_src);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment_src);
        if (!vs || !fs) return false;

        shader_program_ = glCreateProgram();
        glAttachShader(shader_program_, vs);
        glAttachShader(shader_program_, fs);
        glLinkProgram(shader_program_);
        
        glDeleteShader(vs);
        glDeleteShader(fs);
        return true;
    }

    GLuint compileShader(GLenum type, const char* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        return shader;
    }

    void createBuffers() {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, kMaxVertices * sizeof(float), nullptr, GL_DYNAMIC_DRAW); 
    }
};

} // namespace adas::hud

#endif // HUD_RENDERER_HPP
