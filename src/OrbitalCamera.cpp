#include "OrbitalCamera.h"

#include <cmath>

namespace
{
    static float Clamp(float v, float lo, float hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

    static sf::Vector3f operator-(const sf::Vector3f& a, const sf::Vector3f& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    static sf::Vector3f operator+(const sf::Vector3f& a, const sf::Vector3f& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    static sf::Vector3f operator*(const sf::Vector3f& v, float s) {
        return { v.x * s, v.y * s, v.z * s };
    }

    static float Dot(const sf::Vector3f& a, const sf::Vector3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static sf::Vector3f Cross(const sf::Vector3f& a, const sf::Vector3f& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    static float Length(const sf::Vector3f& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    static sf::Vector3f Normalize(const sf::Vector3f& v) {
        float len = Length(v);
        if (len <= 1e-6f) return { 0.f, 0.f, 0.f };
        return { v.x / len, v.y / len, v.z / len };
    }
}

OrbitalCamera::OrbitalCamera()
    : m_target(0.f, 0.f, 0.f)
    , m_distance(10.f)
    , m_azimuth(0.f)
    , m_elevation(0.f)
    , m_fov(3.14159265f / 3.f) // 60 degrees
    , m_near(0.1f)
    , m_far(100.f)
    , m_viewport(800, 600)
{
}

sf::Vector3f OrbitalCamera::ComputePosition() const
{
    // Spherical -> Cartesian relative to target
    float cosElev = std::cos(m_elevation);
    float sinElev = std::sin(m_elevation);
    float cosAzi = std::cos(m_azimuth);
    float sinAzi = std::sin(m_azimuth);

    sf::Vector3f offset;
    offset.x = m_distance * cosElev * sinAzi;      // right
    offset.y = m_distance * sinElev;               // up
    offset.z = m_distance * cosElev * cosAzi;
    
    offset.z = -offset.z;

    return {m_target+offset};
}

void OrbitalCamera::UpdateViewMatrix()
{
    // Compute camera position
    sf::Vector3f eye = ComputePosition();

    // For OpenGL, we look down negative Z
    sf::Vector3f forward = Normalize(m_target - eye);
    sf::Vector3f up(0.f, 1.f, 0.f);

    // Handle the case where forward is parallel to up
    sf::Vector3f right = Normalize(Cross(forward, up));
    if (Length(right) < 1e-6f) {
        // If forward is straight up or down, use a different up reference
        sf::Vector3f altUp(0.f, 0.f, 1.f);
        right = Normalize(Cross(forward, altUp));
        up = Normalize(Cross(right, forward));
    }
    else {
        up = Normalize(Cross(right, forward));
    }

    // Create view matrix (lookAt)
    // [ right.x, up.x, -forward.x, 0 ]
    // [ right.y, up.y, -forward.y, 0 ]
    // [ right.z, up.z, -forward.z, 0 ]
    // [ -dot(eye, right), -dot(eye, up), dot(eye, forward), 1 ]

    m_viewMatrix[0] = right.x; m_viewMatrix[1] = up.x; m_viewMatrix[2] = -forward.x; m_viewMatrix[3] = 0.f;
    m_viewMatrix[4] = right.y; m_viewMatrix[5] = up.y; m_viewMatrix[6] = -forward.y; m_viewMatrix[7] = 0.f;
    m_viewMatrix[8] = right.z; m_viewMatrix[9] = up.z; m_viewMatrix[10] = -forward.z; m_viewMatrix[11] = 0.f;
    m_viewMatrix[12] = -Dot(eye, right);
    m_viewMatrix[13] = -Dot(eye, up);
    m_viewMatrix[14] = Dot(eye, forward);
    m_viewMatrix[15] = 1.f;
}

void OrbitalCamera::UpdateProjectionMatrix()
{
    float aspect = static_cast<float>(m_viewport.x) / static_cast<float>(m_viewport.y);
    float tanHalfFov = std::tan(m_fov * 0.5f);

    m_projectionMatrix[0] = 1.f / (aspect * tanHalfFov);
    m_projectionMatrix[1] = 0.f;
    m_projectionMatrix[2] = 0.f;
    m_projectionMatrix[3] = 0.f;

    m_projectionMatrix[4] = 0.f;
    m_projectionMatrix[5] = 1.f / tanHalfFov;
    m_projectionMatrix[6] = 0.f;
    m_projectionMatrix[7] = 0.f;

    m_projectionMatrix[8] = 0.f;
    m_projectionMatrix[9] = 0.f;
    m_projectionMatrix[10] = -(m_far + m_near) / (m_far - m_near);
    m_projectionMatrix[11] = -1.f;

    m_projectionMatrix[12] = 0.f;
    m_projectionMatrix[13] = 0.f;
    m_projectionMatrix[14] = -(2.f * m_far * m_near) / (m_far - m_near);
    m_projectionMatrix[15] = 0.f;
}

void OrbitalCamera::SetViewport(sf::Vector2u size)
{
    if (size.x > 0u && size.y > 0u)
        m_viewport = size;
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void OrbitalCamera::SetTarget(const sf::Vector3f& target)
{
    m_target = target;
    UpdateViewMatrix();
}

void OrbitalCamera::SetDistance(float distance)
{
    m_distance = (distance <= 0.001f) ? 0.001f : distance;
    UpdateViewMatrix();
}

void OrbitalCamera::SetFovDegrees(float degrees)
{
    const float deg = (degrees <= 0.1f) ? 0.1f : degrees;
    m_fov = deg * 3.14159265358979323846f / 180.f;
    UpdateProjectionMatrix();
}

void OrbitalCamera::SetNearFar(float nearPlane, float farPlane)
{
    if (nearPlane <= 0.f) nearPlane = 0.001f;
    if (farPlane <= nearPlane) farPlane = nearPlane + 1.f;
    m_near = nearPlane;
    m_far = farPlane;
    UpdateProjectionMatrix();
}

void OrbitalCamera::SetAnglesRadians(float azimuth, float elevation)
{
    m_azimuth = azimuth;
    // Keep elevation away from exact +-90 degrees to avoid gimbal singularity.
    const float eps = 1e-3f;
    m_elevation = Clamp(elevation, -3.14159265f / 2.f + eps, 3.14159265f / 2.f - eps);
    UpdateViewMatrix();
}

void OrbitalCamera::Rotate(float deltaAzimuth, float deltaElevation)
{
    SetAnglesRadians(m_azimuth + deltaAzimuth, m_elevation + deltaElevation);
}

void OrbitalCamera::Zoom(float delta)
{
    SetDistance(m_distance + delta);
}

sf::Vector2f OrbitalCamera::Project(const sf::Vector3f& worldPoint) const
{
    // Transform point to view space
    float x = worldPoint.x * m_viewMatrix[0] + worldPoint.y * m_viewMatrix[4] +
        worldPoint.z * m_viewMatrix[8] + m_viewMatrix[12];
    float y = worldPoint.x * m_viewMatrix[1] + worldPoint.y * m_viewMatrix[5] +
        worldPoint.z * m_viewMatrix[9] + m_viewMatrix[13];
    float z = worldPoint.x * m_viewMatrix[2] + worldPoint.y * m_viewMatrix[6] +
        worldPoint.z * m_viewMatrix[10] + m_viewMatrix[14];
    float w = worldPoint.x * m_viewMatrix[3] + worldPoint.y * m_viewMatrix[7] +
        worldPoint.z * m_viewMatrix[11] + m_viewMatrix[15];

    // Perspective division
    if (w == 0.f) w = 1.f;
    x /= w; y /= w; z /= w;

    // Check if behind camera
    if (z >= 0.f) { // In OpenGL, positive Z is behind camera after projection
        return { -10000.f, -10000.f };
    }

    // Apply projection
    float projX = x * m_projectionMatrix[0];
    float projY = y * m_projectionMatrix[5];

    // Convert to NDC [-1, 1]
    projX /= -z;  // Divide by -z (since z is negative in view space for visible objects)
    projY /= -z;

    // Convert to screen coordinates
    float sx = (projX + 1.f) * 0.5f * m_viewport.x;
    float sy = (1.f - projY) * 0.5f * m_viewport.y; // Flip Y

    return { sx, sy };
}

bool OrbitalCamera::IsBehind(const sf::Vector3f& worldPoint) const
{
    // Simplified check - transform Z coordinate to view space
    float viewZ = worldPoint.x * m_viewMatrix[2] + worldPoint.y * m_viewMatrix[6] +
        worldPoint.z * m_viewMatrix[10] + m_viewMatrix[14];
    return viewZ >= 0.f; // In OpenGL, positive Z in view space is behind camera
}