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
    // Wireframe Bounding Box (Unit Cube 0-1)
    static Mesh createBoundingBox() {
        std::vector<Vertex> verts;
        // 12 lines = 24 verts
        // Bottom square
        verts.push_back({0,0,0, 0,0,0}); verts.push_back({1,0,0, 0,0,0});
        verts.push_back({1,0,0, 0,0,0}); verts.push_back({1,0,1, 0,0,0});
        verts.push_back({1,0,1, 0,0,0}); verts.push_back({0,0,1, 0,0,0});
        verts.push_back({0,0,1, 0,0,0}); verts.push_back({0,0,0, 0,0,0});
        // Top square
        verts.push_back({0,1,0, 0,0,0}); verts.push_back({1,1,0, 0,0,0});
        verts.push_back({1,1,0, 0,0,0}); verts.push_back({1,1,1, 0,0,0});
        verts.push_back({1,1,1, 0,0,0}); verts.push_back({0,1,1, 0,0,0});
        verts.push_back({0,1,1, 0,0,0}); verts.push_back({0,1,0, 0,0,0});
        // Vertical edges
        verts.push_back({0,0,0, 0,0,0}); verts.push_back({0,1,0, 0,0,0});
        verts.push_back({1,0,0, 0,0,0}); verts.push_back({1,1,0, 0,0,0});
        verts.push_back({1,0,1, 0,0,0}); verts.push_back({1,1,1, 0,0,0});
        verts.push_back({0,0,1, 0,0,0}); verts.push_back({0,1,1, 0,0,0});
        
        return upload(verts, GL_LINES);
    }
    
    // Lane Area Mesh (Triangle Strip for filled lane polygon)
    static Mesh createLaneArea(int maxPoints) {
        Mesh m;
        m.vertexCount = 0;
        m.primitiveType = GL_TRIANGLE_STRIP;
        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        glBindVertexArray(m.vao);
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        // 2 verts per point (left + right edge)
        glBufferData(GL_ARRAY_BUFFER, maxPoints * 2 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        return m;
    }
    
    static void updateLaneArea(Mesh& m, const std::vector<float>& leftPts, const std::vector<float>& rightPts) {
        // Interleave left and right points to form triangle strip
        std::vector<Vertex> verts;
        size_t count = std::min(leftPts.size(), rightPts.size()) / 3;
        for(size_t i = 0; i < count; ++i) {
            size_t idx = i * 3;
            verts.push_back({leftPts[idx], leftPts[idx+1], leftPts[idx+2], 0,1,0});
            verts.push_back({rightPts[idx], rightPts[idx+1], rightPts[idx+2], 0,1,0});
        }
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(Vertex), verts.data());
        m.vertexCount = verts.size();
    }
    
    // Turn Arrow (Chevron) Mesh
    static Mesh createChevron() {
        std::vector<Vertex> verts;
        // Chevron pointing forward (Z+) - flat on ground
        float w = 0.5f; // half width
        float l = 1.0f; // length
        float t = 0.15f; // thickness
        
        // Left arm of chevron
        verts.push_back({-w, 0.1f, 0,  0,1,0});
        verts.push_back({-w+t, 0.1f, 0,  0,1,0});
        verts.push_back({0, 0.1f, l,  0,1,0});
        
        verts.push_back({-w, 0.1f, 0,  0,1,0});
        verts.push_back({0, 0.1f, l,  0,1,0});
        verts.push_back({0, 0.1f, l-t*2,  0,1,0});
        
        // Right arm
        verts.push_back({w, 0.1f, 0,  0,1,0});
        verts.push_back({0, 0.1f, l,  0,1,0});
        verts.push_back({w-t, 0.1f, 0,  0,1,0});
        
        verts.push_back({w, 0.1f, 0,  0,1,0});
        verts.push_back({0, 0.1f, l-t*2,  0,1,0});
        verts.push_back({0, 0.1f, l,  0,1,0});
        
        return upload(verts, GL_TRIANGLES);
    }
    
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

    // Create a realistic road surface (asphalt quad)
    // Extends from 0 to -length (forward in camera space)
    static Mesh createRoadSurface(float width, float length) {
        std::vector<Vertex> verts;
        float hw = width / 2.0f;
        // Road surface quad - extends in -Z direction
        verts.push_back({-hw, -0.01f, 0, 0, 1, 0});
        verts.push_back({ hw, -0.01f, 0, 0, 1, 0});
        verts.push_back({ hw, -0.01f, -length, 0, 1, 0});
        
        verts.push_back({-hw, -0.01f, 0, 0, 1, 0});
        verts.push_back({ hw, -0.01f, -length, 0, 1, 0});
        verts.push_back({-hw, -0.01f, -length, 0, 1, 0});
        
        return upload(verts, GL_TRIANGLES);
    }
    
    // Create dashed lane markings (realistic road lines)
    // dashLength: length of each dash (e.g., 3m)
    // gapLength: gap between dashes (e.g., 6m)
    // markingWidth: width of the marking (e.g., 0.15m)
    static Mesh createDashedLaneMarking(float totalLength, float dashLength, float gapLength, float markingWidth) {
        std::vector<Vertex> verts;
        float hw = markingWidth / 2.0f;
        float z = 0.0f;
        float y = 0.02f; // Slightly above road
        
        while (z < totalLength) {
            float dashEnd = std::min(z + dashLength, totalLength);
            
            // Create dash quad (two triangles) - extends in -Z
            verts.push_back({-hw, y, -z, 0, 1, 0});
            verts.push_back({ hw, y, -z, 0, 1, 0});
            verts.push_back({ hw, y, -dashEnd, 0, 1, 0});
            
            verts.push_back({-hw, y, -z, 0, 1, 0});
            verts.push_back({ hw, y, -dashEnd, 0, 1, 0});
            verts.push_back({-hw, y, -dashEnd, 0, 1, 0});
            
            z += dashLength + gapLength;
        }
        
        return upload(verts, GL_TRIANGLES);
    }
    
    // Create solid lane marking (continuous line) - extends in -Z
    static Mesh createSolidLaneMarking(float totalLength, float markingWidth) {
        std::vector<Vertex> verts;
        float hw = markingWidth / 2.0f;
        float y = 0.02f;
        
        // One long quad - extends in -Z
        verts.push_back({-hw, y, 0, 0, 1, 0});
        verts.push_back({ hw, y, 0, 0, 1, 0});
        verts.push_back({ hw, y, -totalLength, 0, 1, 0});
        
        verts.push_back({-hw, y, 0, 0, 1, 0});
        verts.push_back({ hw, y, -totalLength, 0, 1, 0});
        verts.push_back({-hw, y, -totalLength, 0, 1, 0});
        
        return upload(verts, GL_TRIANGLES);
    }
    
    // Create double yellow center line (for opposite traffic)
    static Mesh createDoubleCenterLine(float totalLength, float markingWidth, float gap) {
        std::vector<Vertex> verts;
        float hw = markingWidth / 2.0f;
        float y = 0.015f;
        
        // Left line
        float leftX = -gap/2.0f - hw;
        verts.push_back({leftX - hw, y, 0, 0, 1, 0});
        verts.push_back({leftX + hw, y, 0, 0, 1, 0});
        verts.push_back({leftX + hw, y, totalLength, 0, 1, 0});
        verts.push_back({leftX - hw, y, 0, 0, 1, 0});
        verts.push_back({leftX + hw, y, totalLength, 0, 1, 0});
        verts.push_back({leftX - hw, y, totalLength, 0, 1, 0});
        
        // Right line  
        float rightX = gap/2.0f + hw;
        verts.push_back({rightX - hw, y, 0, 0, 1, 0});
        verts.push_back({rightX + hw, y, 0, 0, 1, 0});
        verts.push_back({rightX + hw, y, totalLength, 0, 1, 0});
        verts.push_back({rightX - hw, y, 0, 0, 1, 0});
        verts.push_back({rightX + hw, y, totalLength, 0, 1, 0});
        verts.push_back({rightX - hw, y, totalLength, 0, 1, 0});
        
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
        float w=0.4f; float h=0.4f;
        verts.push_back({-w, 0, 0, 0,0,1}); verts.push_back({w, 0, 0, 0,0,1}); verts.push_back({w, h*2, 0, 0,0,1});
        verts.push_back({-w, 0, 0, 0,0,1}); verts.push_back({w, h*2, 0, 0,0,1}); verts.push_back({-w, h*2, 0, 0,0,1});
        return upload(verts, GL_TRIANGLES);
    }

    // Standard Unit Quad [0,0] to [1,1] with UVs in Normal.xy
    static Mesh createQuad() {
        std::vector<Vertex> verts;
        // Pos(x,y,z), Norm(u,v,dummy)
        // 0,0
        verts.push_back({0, 0, 0, 0,0,0}); 
        // 1,0
        verts.push_back({1, 0, 0, 1,0,0}); 
        // 1,1
        verts.push_back({1, 1, 0, 1,1,0});
        
        // 0,0
        verts.push_back({0, 0, 0, 0,0,0}); 
        // 1,1
        verts.push_back({1, 1, 0, 1,1,0}); 
        // 0,1
        verts.push_back({0, 1, 0, 0,1,0});
        
        return upload(verts, GL_TRIANGLES);
    }

    // --- NEW PROCEDURAL MESHES (Phase 10) ---

    static Mesh createBus() {
        std::vector<Vertex> verts;
        float w = 1.2f; float l = 6.0f; float h = 3.0f;
        
        // Helper to add quad
        auto addQuad = [&](float x1, float y1, float z1, float x2, float y2, float z2,
                           float x3, float y3, float z3, float x4, float y4, float z4,
                           float nx, float ny, float nz) {
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x2, y2, z2, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz});
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz}); verts.push_back({x4, y4, z4, nx, ny, nz});
        };
        
        // Simple Box Bus
        // Sides
        addQuad(-w, 0, -l, w, 0, -l, w, h, -l, -w, h, -l, 0,0,-1); // Front
        addQuad(w, 0, l, -w, 0, l, -w, h, l, w, h, l, 0,0,1); // Back
        addQuad(-w, 0, l, -w, 0, -l, -w, h, -l, -w, h, l, -1,0,0); // Left
        addQuad(w, 0, -l, w, 0, l, w, h, l, w, h, -l, 1,0,0); // Right
        addQuad(-w, h, -l, w, h, -l, w, h, l, -w, h, l, 0,1,0); // Top
        
        return upload(verts, GL_TRIANGLES);
    }

    static Mesh createTruck() {
        std::vector<Vertex> verts;
        float w = 1.2f; 
        
        // Helper
        auto addQuad = [&](float x1, float y1, float z1, float x2, float y2, float z2,
                           float x3, float y3, float z3, float x4, float y4, float z4,
                           float nx, float ny, float nz) {
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x2, y2, z2, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz});
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz}); verts.push_back({x4, y4, z4, nx, ny, nz});
        };
        
        // 1. Cabin (Front)
        float c_l = 1.5f; float c_h = 2.5f; float offset_z = -3.0f;
        addQuad(-w, 0, offset_z-c_l, w, 0, offset_z-c_l, w, c_h, offset_z-c_l, -w, c_h, offset_z-c_l, 0,0,-1);
        addQuad(-w, 0, offset_z, -w, 0, offset_z-c_l, -w, c_h, offset_z-c_l, -w, c_h, offset_z, -1,0,0);
        addQuad(w, 0, offset_z-c_l, w, 0, offset_z, w, c_h, offset_z, w, c_h, offset_z-c_l, 1,0,0);
        addQuad(-w, c_h, offset_z-c_l, w, c_h, offset_z-c_l, w, c_h, offset_z, -w, c_h, offset_z, 0,1,0);
        
        // 2. Trailer (Rear)
        float t_l = 4.0f; float t_h = 3.5f; float t_z = 2.0f;
        addQuad(-w, 0, t_z-t_l, w, 0, t_z-t_l, w, t_h, t_z-t_l, -w, t_h, t_z-t_l, 0,0,-1); // Trailer Front
        addQuad(w, 0, t_z+t_l, -w, 0, t_z+t_l, -w, t_h, t_z+t_l, w, t_h, t_z+t_l, 0,0,1); // Trailer Back
        addQuad(-w, 0, t_z+t_l, -w, 0, t_z-t_l, -w, t_h, t_z-t_l, -w, t_h, t_z+t_l, -1,0,0); // Left
        addQuad(w, 0, t_z-t_l, w, 0, t_z+t_l, w, t_h, t_z+t_l, w, t_h, t_z-t_l, 1,0,0); // Right
        addQuad(-w, t_h, t_z-t_l, w, t_h, t_z-t_l, w, t_h, t_z+t_l, -w, t_h, t_z+t_l, 0,1,0); // Top
        
        return upload(verts, GL_TRIANGLES);
    }
    
    static Mesh createMotorcycle() {
        std::vector<Vertex> verts;
        float w = 0.3f; float l = 1.0f; float h = 1.2f;
        
        // Helper
        auto addQuad = [&](float x1, float y1, float z1, float x2, float y2, float z2,
                           float x3, float y3, float z3, float x4, float y4, float z4,
                           float nx, float ny, float nz) {
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x2, y2, z2, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz});
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz}); verts.push_back({x4, y4, z4, nx, ny, nz});
        };
        
        // Body
        addQuad(-w, 0, -l, w, 0, -l, w, h*0.6, -l, -w, h*0.6, -l, 0,0,-1);
        addQuad(w, 0, l, -w, 0, l, -w, h*0.6, l, w, h*0.6, l, 0,0,1);
        addQuad(-w, 0, l, -w, 0, -l, -w, h*0.6, -l, -w, h*0.6, l, -1,0,0);
        addQuad(w, 0, -l, w, 0, l, w, h*0.6, l, w, h*0.6, -l, 1,0,0);
        
        // Rider/Screen
        addQuad(-w, h*0.6, -0.5, w, h*0.6, -0.5, w, h, -0.2, -w, h, -0.2, 0,1,0);
        
        return upload(verts, GL_TRIANGLES);
    }
    
    static Mesh createBuilding() {
         std::vector<Vertex> verts;
         float w = 5.0f; float h = 20.0f; float d = 5.0f;
         
        auto addQuad = [&](float x1, float y1, float z1, float x2, float y2, float z2,
                           float x3, float y3, float z3, float x4, float y4, float z4,
                           float nx, float ny, float nz) {
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x2, y2, z2, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz});
            verts.push_back({x1, y1, z1, nx, ny, nz}); verts.push_back({x3, y3, z3, nx, ny, nz}); verts.push_back({x4, y4, z4, nx, ny, nz});
        };
        
        // Simple Tall Box
        addQuad(-w, 0, d, w, 0, d, w, h, d, -w, h, d, 0,0,1); // Front
        addQuad(w, 0, -d, -w, 0, -d, -w, h, -d, w, h, -d, 0,0,-1); // Back
        addQuad(-w, 0, -d, -w, 0, d, -w, h, d, -w, h, -d, -1,0,0); // Left
        addQuad(w, 0, d, w, 0, -d, w, h, -d, w, h, d, 1,0,0); // Right
        addQuad(-w, h, d, w, h, d, w, h, -d, -w, h, -d, 0,1,0); // Top
        
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

public:
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
