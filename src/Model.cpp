#include <Model.h>

#include <stdexcept>
#include <iostream>
#include <unordered_map>

#include <tinyobj/tiny_obj_loader.h>

Model::Model(const std::string &path)
{
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string errors;
    std::string warnings;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warnings, &errors, path.c_str()))
    {
        throw std::runtime_error("Could not load model at " + path + ", error: " + errors);
    }
    if (!warnings.empty())
    {
        std::cout << "Warnings loading " << path << ":" << warnings;
    }

    for (const auto& shape : shapes)
    {
        for (auto index : shape.mesh.indices)
        {
            uint32_t base_vertex_id = static_cast<uint32_t>(index.vertex_index * 3);
			glm::vec3 position = {attributes.vertices[base_vertex_id], attributes.vertices[base_vertex_id + 1],
								  attributes.vertices[base_vertex_id + 2]};
			//glm::vec3 color = {attributes.colors[base_vertex_id], attributes.colors[base_vertex_id + 1], 
             //   attributes.colors[base_vertex_id + 2]};
            glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f};

            int32_t base_uv_id = index.texcoord_index * 2;
            glm::vec2 uv = {0.0f, 0.0f};
            if (base_uv_id >= 0)
            {
                uv = {attributes.texcoords[base_uv_id], 1.0f - attributes.texcoords[base_uv_id + 1]};
            }
			m_Vertices.emplace_back(position, color, uv);
			m_Indices.emplace_back(static_cast<uint32_t>(m_Indices.size()));
        }
    }
}

const std::vector<Vertex>& Model::GetVertices() const
{
    return m_Vertices;
}

const std::vector<uint32_t>& Model::GetIndices() const
{
    return m_Indices;
}
