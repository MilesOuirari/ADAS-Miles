#ifndef HMI_OBJ_LOADER_HPP
#define HMI_OBJ_LOADER_HPP

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"
#include "hmi/MeshGenerators.hpp"
#include <iostream>
#include <string>

namespace adas::hmi {

/**
 * Load an OBJ file and convert to our Mesh format.
 * Returns a mesh with GL_TRIANGLES rendering mode.
 */
inline Mesh loadOBJ(const std::string& filepath) {
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.vertex_color = false;
    
    if (!reader.ParseFromFile(filepath, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "OBJ Load Error: " << reader.Error() << std::endl;
        }
        // Return empty mesh
        Mesh m;
        m.vao = 0;
        m.vbo = 0;
        m.vertexCount = 0;
        m.primitiveType = GL_TRIANGLES;
        return m;
    }
    
    if (!reader.Warning().empty()) {
        std::cout << "OBJ Warning: " << reader.Warning() << std::endl;
    }
    
    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    
    std::vector<Vertex> verts;
    
    // Iterate over shapes
    for (const auto& shape : shapes) {
        // Iterate over faces (triangles)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fv = shape.mesh.num_face_vertices[f];
            
            // Should be 3 (triangles) since we set triangulate=true
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                
                float nx = 0.0f, ny = 1.0f, nz = 0.0f;
                if (idx.normal_index >= 0) {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }
                
                verts.push_back({vx, vy, vz, nx, ny, nz});
            }
            index_offset += fv;
        }
    }
    
    std::cout << "Loaded OBJ: " << filepath << " (" << verts.size() << " vertices)" << std::endl;
    
    // Upload to GPU
    return MeshGenerators::upload(verts, GL_TRIANGLES);
}

} // namespace adas::hmi

#endif // HMI_OBJ_LOADER_HPP
