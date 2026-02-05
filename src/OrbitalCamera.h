#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>
#include <array>

class OrbitalCamera
{
public:
    OrbitalCamera();

    void SetViewport(sf::Vector2u size);
    void SetTarget(const sf::Vector3f& target);
    void SetDistance(float distance);
    void SetFovDegrees(float degrees);
    void SetNearFar(float nearPlane, float farPlane);
    void SetAnglesRadians(float azimuth, float elevation);

    void Rotate(float deltaAzimuth, float deltaElevation);
    void Zoom(float delta); // adjusts distance

    sf::Vector3f GetPosition() const { return ComputePosition(); }
    const std::array<float, 16>& GetViewMatrix() const { return m_viewMatrix; }
    const std::array<float, 16>& GetProjectionMatrix() const { return m_projectionMatrix; }

    sf::Vector2f Project(const sf::Vector3f& worldPoint) const;
    bool IsBehind(const sf::Vector3f& worldPoint) const;

private:
    sf::Vector3f m_target;
    float m_distance;
    float m_azimuth;   // radians
    float m_elevation; // radians
    float m_fov; // radians
    float m_near;
    float m_far;
    sf::Vector2u m_viewport;

    std::array<float, 16> m_viewMatrix;
    std::array<float, 16> m_projectionMatrix;

    sf::Vector3f ComputePosition() const;
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    
};