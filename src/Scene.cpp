#include "Scene.h"
#include "OrbitalCamera.h"

#include <iostream>

bool Scene::LoadModel(const std::string& filepath, ModelLoader& loader)
{
    auto opt = loader.LoadModel(filepath);
    if (!opt)
    {
        std::cerr << "Failed to load model from '" << filepath << "'\n";
        return false;
    }

    m_model = std::move(*opt);

    std::cout << "Loaded model '" << filepath << "' with " << m_model.meshes.size() << " mesh(es) and " << m_model.textures.size() << " texture(s).\n";
    return true;
}

void Scene::SetProjection(sf::Vector2f offset, float scale)
{
    m_offset = offset;
    m_scale = scale;
}

void Scene::SetCamera(const OrbitalCamera* camera)
{
    m_camera = camera;
}

void Scene::Clear()
{
    m_model = Model{};
}

sf::Vector2f Scene::Project(const sf::Vector3f& v) const
{
    if (m_camera)
    {
        return m_camera->Project(v);
    }

    // Simple orthographic projection: X -> X, Y -> -Y (flip to match screen Y-down)
    return {v.x * m_scale + m_offset.x, -v.y * m_scale + m_offset.y};
}

void Scene::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    // Build and draw each mesh every frame so projection reflects current camera state.
    for (const auto& mesh : m_model.meshes)
    {
        // Determine texture size if present (used to convert UVs to tex coords in pixels)
        sf::Vector2u texSize{1, 1};
        if (mesh.textureIndex >= 0 && static_cast<size_t>(mesh.textureIndex) < m_model.textures.size())
        {
            texSize = m_model.textures[mesh.textureIndex].getSize();
            if (texSize.x == 0) texSize.x = 1;
            if (texSize.y == 0) texSize.y = 1;
        }

        sf::VertexArray va(sf::PrimitiveType::Triangles, mesh.indices.size());

        for (size_t i = 0; i < mesh.indices.size(); ++i)
        {
            unsigned int idx = mesh.indices[i];
            const Vertex& v = mesh.vertices[idx];

            sf::Vertex vert;
            vert.position = Project(v.position);
            vert.texCoords = sf::Vector2f(v.texCoords.x * static_cast<float>(texSize.x),
                                          v.texCoords.y * static_cast<float>(texSize.y));
            va[i] = vert;
        }

        if (mesh.textureIndex >= 0 && static_cast<size_t>(mesh.textureIndex) < m_model.textures.size())
        {
            states.texture = &m_model.textures[mesh.textureIndex];
        }
        else
        {
            states.texture = nullptr;
            // don't spam stderr every frame; keep to developer-time messages
        }

        target.draw(va, states);
    }
}