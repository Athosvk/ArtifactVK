#pragma once
#include <string>
#include <vector>

#include <Vertex.h>

class Model
{
public:
    Model(const std::string &path);

    const std::vector<Vertex> &GetVertices() const;
    const std::vector<uint32_t> &GetIndices() const;

private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
};