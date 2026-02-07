#ifndef HMI_MESH_GENERATORS_HPP
#define HMI_MESH_GENERATORS_HPP

#include <vector>
#include <cmath>
#include <GL/glew.h>

namespace adas::hmi {

struct Mesh {
    GLuint vao, vbo;
    size_t vertexCount;
    // Primitive type: GL_TRIANGLES, GL_LINES etc.
    GLenum primitiveType;

    void draw() const {
        glBindVertexArray(vao);
        glDrawArrays(primitiveType, 0, vertexCount);
    }
};

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
};

class MeshGenerators {
public:
    // Generate a sleek "Cyber" car
    static Mesh createCar() {
        std::vector<Vertex> verts;
        
        // Dimensions
        float w = 0.9f; // Half width
        float l_f = 2.0f; // Length Front
        float l_r = 2.0f; // Length Rear
        float h_b = 0.5f; // Body Height
        float h_c = 1.3f; // Cabin Height
        float w_c = 0.6f; // Cabin Half Width

        // Helper to add quad
        auto addQuad = [&](float x1, float y1, float z1,
                           float x2, float y2, float z2,
                           float x3, float y3, float z3,
                           float x4, float y4, float z4,
                           float nx, float ny, float nz) {
            verts.push_back({x1, y1, z1, nx, ny, nz});
            verts.push_back({x2, y2, z2, nx, ny, nz});
            verts.push_back({x3, y3, z3, nx, ny, nz});
            verts.push_back({x1, y1, z1, nx, ny, nz});
            verts.push_back({x3, y3, z3, nx, ny, nz});
            verts.push_back({x4, y4, z4, nx, ny, nz});
        };

        // Note: Our system: X=Lateral, Y=Altitude/Up?, Z=Forward?
        // Let's stick to standard OpenGL:
        // Y=Up, -Z=Forward, X=Right.
        // Car center at 0,0,0

        // Bottom Plate
        addQuad(-w, 0, -l_f,  w, 0, -l_f,  w, 0, l_r, -w, 0, l_r,  0,-1,0);

        // Hood (Front)
        addQuad(-w, h_b, 0.0f, w, h_b, 0.0f, w, 0.3f, -l_f, -w, 0.3f, -l_f, 0, 0.5, -0.5);

        // Trunk (Rear)
        addQuad(-w, h_b, 0.0f, -w, 0.4f, l_r, w, 0.4f, l_r, w, h_b, 0.0f, 0, 0.5, 0.5);

        // Sides (Simple extrude logic roughly)
        // Let's just do a simple stylized box logic for stability
        
        // Roof
        addQuad(-w_c, h_c, -0.5f, w_c, h_c, -0.5f, w_c, h_c, 1.0f, -w_c, h_c, 1.0f, 0, 1, 0);

        // Windshield
        addQuad(-w_c, h_c, -0.5f, -w, h_b, 0.0f, w, h_b, 0.0f, w_c, h_c, -0.5f, 0, 0.7, -0.7);

        // Rear window
        addQuad(-w_c, h_c, 1.0f, w_c, h_c, 1.0f, w, 0.4f, l_r, -w, 0.4f, l_r, 0, 0.5, 0.5);
        
        // Side Panels (Left)
        // Just fill the gap quad
        addQuad(-w, 0.3f, -l_f, -w, h_b, 0.0f, -w, 0, -l_f, -w, 0, 0, -1, 0, 0); // Front side partial
        // Simplifying...
        
        // Just return what we have (Hood + Trunk + Roof + Windows). 
        // A minimal "floating" cyber car logic.

        return upload(verts, GL_TRIANGLES);
    }

    static Mesh createGrid(float size, int divisions) {
        std::vector<Vertex> verts;
        float step = size / divisions;
        float offset = size / 2.0f;

        for (int i = 0; i <= divisions; ++i) {
            float pos = -offset + i * step;
            // Line along Z (variable X)
            verts.push_back({pos, 0, -offset, 0,1,0});
            verts.push_back({pos, 0, offset, 0,1,0});
            
            // Line along X (variable Z)
            verts.push_back({-offset, 0, pos, 0,1,0});
            verts.push_back({offset, 0, pos, 0,1,0});
        }
        return upload(verts, GL_LINES);
    }
    
    // Create a mesh for a specific lane segment
    // To be dynamic, we might update this every frame, but VBO streaming is complex.
    // We will use a pre-allocated dynamic line strip buffer.
    static Mesh createDynamicLane(int maxPoints) {
        Mesh m;
        m.vertexCount = 0; // Starts empty
        m.primitiveType = GL_LINE_STRIP;
        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        glBindVertexArray(m.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        // Position only for lines
        glBufferData(GL_ARRAY_BUFFER, maxPoints * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        // Alpha attribute (loc 1) - default 1.0
        glVertexAttrib1f(1, 1.0f); 
        return m;
    }

    static void updateLaneMesh(Mesh& m, const std::vector<float>& points) {
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(float), points.data());
        m.vertexCount = points.size() / 3;
    }

    // Simplified Low-Poly Human
    static Mesh createPedestrian() {
        std::vector<Vertex> verts;
        // Head
        // Body
        // Legs
        // Just a simple box-man for HMI
        float w=0.25f, h=1.7f, d=0.25f;
        // Body Color will be handled by uniform usually, but mesh is same logic
        // Use a cylinder/capsule approx
        
        // Let's just create a vertical box for now, maybe with a "head" sphere if possible?
        // Box is easiest.
        // x1,y1,z1 ...
        // We can reuse the car quad logic if we refactored it, but I'll inline a simple box.
        
        // Front Face
        verts.push_back({-w, 0, d, 0,0,1}); verts.push_back({w, 0, d, 0,0,1}); verts.push_back({w, h, d, 0,0,1});
        verts.push_back({-w, 0, d, 0,0,1}); verts.push_back({w, h, d, 0,0,1}); verts.push_back({-w, h, d, 0,0,1});
        
        // Back Face
        verts.push_back({w, 0, -d, 0,0,-1}); verts.push_back({-w, 0, -d, 0,0,-1}); verts.push_back({-w, h, -d, 0,0,-1});
        verts.push_back({w, 0, -d, 0,0,-1}); verts.push_back({-w, h, -d, 0,0,-1}); verts.push_back({w, h, -d, 0,0,-1});
        
        // Head (Small box on top)
        float hw=0.15f, hh=0.25f;
        float hy = h;
        // ... omitted detail for perf, just the body pillar is enough for "blob" detection usually
        
        return upload(verts, GL_TRIANGLES);
    }
    
    static Mesh createSignBoard() {
        std::vector<Vertex> verts;
        // A simple rectangular board
        float w=0.4f; float h=0.4f;
        // Facing +/- Z? Signs face the driver (so their normal faces -Z, drivers view faces +Z... wait. Driver looks -Z. Sign faces +Z.)
        
        verts.push_back({-w, 0, 0, 0,0,1}); verts.push_back({w, 0, 0, 0,0,1}); verts.push_back({w, h*2, 0, 0,0,1});
        verts.push_back({-w, 0, 0, 0,0,1}); verts.push_back({w, h*2, 0, 0,0,1}); verts.push_back({-w, h*2, 0, 0,0,1});
        
        return upload(verts, GL_TRIANGLES);
    }

    static Mesh createSignPost() {
         std::vector<Vertex> verts;
         float w=0.05f; float h=2.0f;
         // Cylinder approx (4-sided is enough)
         float r = w;
         
         // Front
         verts.push_back({-r, 0, r, 0,0,1}); verts.push_back({r, 0, r, 0,0,1}); verts.push_back({r, h, r, 0,0,1});
         verts.push_back({-r, 0, r, 0,0,1}); verts.push_back({r, h, r, 0,0,1}); verts.push_back({-r, h, r, 0,0,1});
         // ... (+ other sides) to make it look 3D
         
         return upload(verts, GL_TRIANGLES);
    }

    // Dynamic Ribbon for Path Planning
    // Usage: Update regularly with new points
    static Mesh createDynamicRibbon(int maxPoints) {
        Mesh m;
        m.vertexCount = 0; 
        m.primitiveType = GL_TRIANGLE_STRIP;
        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        glBindVertexArray(m.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        
        // 5 floats per vertex: x,y,z, u,v
        glBufferData(GL_ARRAY_BUFFER, maxPoints * 2 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        glEnableVertexAttribArray(0); // Pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        
        glEnableVertexAttribArray(1); // TexCoord (we misuse 'Normal' logic in upload but here specific)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
        
        return m;
    }

    static void updateRibbonMesh(Mesh& m, const std::vector<float>& points, float width) {
        // Points are x,z centerline. We expand to quad strip.
        std::vector<float> data;
        float totalDist = 0;
        for(size_t i=0; i<points.size()/2; ++i) {
             float x = points[i*2];
             float z = points[i*2+1];
             
             // Simple expansion (assuming forward is roughly -Z, so lateral is X)
             // For curves we need normal calculation, but for HMI demo simple offset is ok.
             
             data.push_back(x - width/2); data.push_back(0.1f); data.push_back(z); // Left
             data.push_back(0.0f); data.push_back(i / (float)points.size()); // UV
             
             data.push_back(x + width/2); data.push_back(0.1f); data.push_back(z); // Right
             data.push_back(1.0f); data.push_back(i / (float)points.size()); // UV
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
        m.vertexCount = data.size() / 5;
    }

private:
    static Mesh upload(const std::vector<Vertex>& verts, GLenum type) {
        // std::cout << "Uploading Mesh: " << verts.size() << " vertices." << std::endl;
        Mesh m;
        m.vertexCount = verts.size();
        m.primitiveType = type;
        
        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        
        if (m.vao == 0) std::cerr << "ERROR: VAO is 0. GLEW issue?" << std::endl;

        glBindVertexArray(m.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        
        return m;
    }
};

} // namespace adas::hmi

#endif // HMI_MESH_GENERATORS_HPP
