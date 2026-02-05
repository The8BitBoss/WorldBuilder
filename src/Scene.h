#pragma once

#include <SFML/Graphics.hpp>
#include "ModelLoader.h"
#include <vector>

class OrbitalCamera; // forward-declare

class Scene : public sf::Drawable
{
public:
    Scene() = default;

    // Load a model using the provided loader. Returns false on failure.
    bool LoadModel(const std::string& filepath, ModelLoader& loader);

    // Set a simple orthographic projection: screen offset (in pixels) and uniform scale.
    void SetProjection(sf::Vector2f offset, float scale);

    // Use an orbital 3D camera for projection. If set, camera projection overrides orthographic projection.
    void SetCamera(const OrbitalCamera* camera);

    // Clear loaded model/meshes.
    void Clear();

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::Vector2f Project(const sf::Vector3f& v) const;

    Model m_model;
    sf::Vector2f m_offset{0.f, 0.f};
    float m_scale{1.f};

    const OrbitalCamera* m_camera{nullptr};
};