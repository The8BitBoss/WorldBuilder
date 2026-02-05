#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include "Scene.h"
#include "OrbitalCamera.h"
#include "ModelLoader.h"
#include <iostream>

sf::Vector2u WorldSize{ 1920u/2, 1080u/2 };

int main()
{
    std::cout << "SFML version: "
        << SFML_VERSION_MAJOR << "."
        << SFML_VERSION_MINOR << "."
        << SFML_VERSION_PATCH << std::endl;
    Scene world;
    ModelLoader loader;
	OrbitalCamera camera;

	world.SetCamera(&camera);
    world.SetProjection({960.f, 540.f}, 1.f);

	camera.SetViewport(WorldSize);
	camera.SetTarget({ 0.f, 0.f, 0.f });
	camera.SetDistance(3.f);

    if (!world.LoadModel("assets\\models\\Earth\\earth.fbx", loader))
    {
        std::cerr << "Failed to load model\n";
        return 1;
    }

    // Request a depth buffer when creating the OpenGL context
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;

    auto window = sf::RenderWindow(sf::VideoMode(WorldSize), "CMake SFML Project", sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(144);


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Interaction state
    bool dragging = false;
    sf::Vector2i lastMousePos{0,0};
    const float rotationSpeed = 0.005f; // radians per pixel
    const float wheelZoomSpeed = 0.5f;  // world units per wheel step
    const float keyRotateSpeed = 0.03f; // radians per key press
    const float keyZoomSpeed = 0.1f;    // world units per key press

    while (window.isOpen())
    {
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left)
                {
                    dragging = true;
                    lastMousePos = mouseButtonPressed->position;
                }
            }
            else if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouseButtonReleased->button == sf::Mouse::Button::Left)
                {
                    dragging = false;
                }
            }
            else if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
            {
                if (dragging)
                {
                    sf::Vector2i cur = mouseMoved->position;
                    sf::Vector2i delta = cur - lastMousePos;
                    // Horizontal mouse movement -> azimuth, vertical -> elevation (invert Y to feel natural)
                    camera.Rotate(-static_cast<float>(delta.x) * rotationSpeed,
                        -static_cast<float>(delta.y) * rotationSpeed);
                    lastMousePos = cur;
                }
            }
            else if (const auto* mouseWheelScrolled = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                // Positive delta typically means wheel up (zoom in)
                float delta = mouseWheelScrolled->delta;
                camera.Zoom(-delta * wheelZoomSpeed); // negative so wheel up reduces distance
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                // Keyboard controls: arrows to rotate, +/- to zoom
                switch (keyPressed->scancode)
                {
                case sf::Keyboard::Scan::Left:
                    camera.Rotate(-keyRotateSpeed, 0.f);
                    break;
                case sf::Keyboard::Scan::Right:
                    camera.Rotate(keyRotateSpeed, 0.f);
                    break;
                case sf::Keyboard::Scan::Up:
                    camera.Rotate(0.f, -keyRotateSpeed);
                    break;
                case sf::Keyboard::Scan::Down:
                    camera.Rotate(0.f, keyRotateSpeed);
                    break;
                case sf::Keyboard::Scan::Equal:
                    camera.Zoom(-keyZoomSpeed);
                    break;
                case sf::Keyboard::Scan::Hyphen:
                    camera.Zoom(keyZoomSpeed);
                    break;
                default:
                    break;
                }
            }
        }

        // Clear color (SFML) and depth (OpenGL) buffers
        window.clear(sf::Color::Black);
        glClear(GL_DEPTH_BUFFER_BIT);

        window.draw(world);
        window.display();
    }
}
