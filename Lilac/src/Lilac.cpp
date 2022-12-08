////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML3D/Graphics.hpp>
#include <SFML3D/Window.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>


////////////////////////////////////////////////////////////
// Obj model
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \return Application exit code
///
/// Note that the shader code (on line 1000 of RenderTarget.cpp) needs to be modified
/// To be #version 420 to run this with proper shading (for some reason... it's really not clear to me why this would be needed)
////////////////////////////////////////////////////////////
int main()
{
    // Request a 32-bits depth buffer when creating the window
    sf3d::ContextSettings contextSettings;
    contextSettings.depthBits = 32;

    // Create the main window
    sf3d::RenderWindow window(sf3d::VideoMode(800, 600), "SFML3D 3D graphics", sf3d::Style::Default, contextSettings);
    window.setVerticalSyncEnabled(true);

    float aspectRatio = static_cast<float>(window.getSize().x) / static_cast<float>(window.getSize().y);
    sf3d::Vector3f downscaleFactor(1 / static_cast<float>(window.getSize().x) * aspectRatio, -1 / static_cast<float>(window.getSize().y), 1);

    // Set up our 3D camera with a field of view of 90 degrees
    // 1000 units space between the clipping planes
    // and scale it according to the screen aspect ratio
    sf3d::Camera camera(90.f, 0.001f, 1000.f);
    camera.scale(1 / aspectRatio, 1, 1);
    camera.setPosition(0, 0, 10);

    // Set the camera as the window's active view
    window.setView(camera);

    // Create a cube to demonstrate transform and lighting effects
    std::vector<sf3d::Cuboid> cubes{};
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int z = -1; z <= 1; z++)
            {
                cubes.emplace_back(sf3d::Vector3f(1, 1, 1));
                cubes.back().setColor(sf3d::Color::Red);
                cubes.back().setPosition(2 * x, 2 * y, 2 * z);
            }
        }
    }

    // Create a clock for measuring the time elapsed
    sf3d::Clock clock;
    float elapsedSeconds = 0;

    // Create a light to illuminate our scene
    sf3d::Light light;
    light.setColor(sf3d::Color::White);
    light.setAmbientIntensity(0.1f);
    light.setDiffuseIntensity(1.0f);
    light.setLinearAttenuation(0.002f);
    light.setQuadraticAttenuation(0.0005f);
    light.enable();
    sf3d::Light::enableLighting();

    // Enable depth testing so we can draw 3D objects in any order
    window.enableDepthTest(true);

    // Keep the mouse cursor hidden at the center of the window
    sf3d::Mouse::setPosition(sf3d::Vector2i(window.getSize()) / 2, window);
    window.setMouseCursorVisible(false);

    // Variables that keep track of our virtual camera orientation
    float yaw = 3.141592654f / 2.f;
    float pitch = 0.f;
    sf3d::Vector3f direction;
    sf3d::Vector3f rightVector;

    // Start game loop
    while (window.isOpen())
    {
        float delta = clock.restart().asSeconds();
        elapsedSeconds += delta;

        int deltaX = 0;
        int deltaY = 0;

        // Process events
        sf3d::Event event;
        while (window.pollEvent(event))
        {
            // Close window : exit
            if (event.type == sf3d::Event::Closed)
                window.close();

            // Escape key : exit
            if ((event.type == sf3d::Event::KeyPressed) && (event.key.code == sf3d::Keyboard::Escape))
                window.close();

            // Mouse move : rotate camera
            if (event.type == sf3d::Event::MouseMoved)
            {
                deltaX = event.mouseMove.x - window.getSize().x / 2;
                deltaY = event.mouseMove.y - window.getSize().y / 2;
            }
        }

        sf3d::Mouse::setPosition(sf3d::Vector2i(window.getSize()) / 2, window);

        // Update our virtual camera orientation/position based on user input
        yaw -= deltaX / 5.f * delta;
        pitch -= deltaY / 5.f * delta;

        direction.x = std::cos(yaw) * std::cos(pitch);
        direction.y = std::sin(pitch);
        direction.z = -std::sin(yaw) * std::cos(pitch);

        rightVector.x = -direction.z;
        rightVector.y = 0.f;
        rightVector.z = direction.x;

        float rightVectorNorm = std::sqrt(rightVector.x * rightVector.x +
            rightVector.y * rightVector.y +
            rightVector.z * rightVector.z);
        rightVector /= rightVectorNorm;

        camera.setDirection(direction);

        // W key pressed : move forward
        if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::W))
            camera.move(direction * 50.f * delta);

        // A key pressed : strafe left
        if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::A))
            camera.move(rightVector * -50.f * delta);

        // S key pressed : move backward
        if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::S))
            camera.move(direction * -50.f * delta);

        // D key pressed : strafe right
        if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::D))
            camera.move(rightVector * 50.f * delta);

        if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::LAlt))
        {
            sf3d::Texture t;
            t.create(window.getSize().x, window.getSize().y);
            t.update(window);
            t.copyToImage().saveToFile("screenshot.png");
            return EXIT_SUCCESS;
        }
            

        //// Space bar pressed : move upwards
        //if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::Space))
        //    camera.move(0, 50 * delta, 0);

        //// Shift key pressed : move downwards
        //if (sf3d::Keyboard::isKeyPressed(sf3d::Keyboard::LShift))
        //    camera.move(0, -50 * delta, 0);

        // Inform the window to update its view with the new camera data
        window.setView(camera);

        // Clear the window
        window.clear();

        // Make the light source orbit around the scene
        sf3d::Vector3f newOrbitPosition(50 * std::cos(elapsedSeconds / 6),
            30 * std::cos(elapsedSeconds / 6),
            20 * std::sin(elapsedSeconds / 6));
        light.setPosition(sf3d::Vector3f(0, 0, -50) + newOrbitPosition);

        // Disable lighting for the text and the light sphere
        sf3d::Light::disableLighting();

        // Enable lighting again
        sf3d::Light::enableLighting();

        // Draw the cube, sphere and billboard
        for (auto& cube : cubes)
        {
            window.draw(cube);
        }

        // Disable lighting and reset to 2D view to draw information
        sf3d::Light::disableLighting();
        window.setView(window.getDefaultView());

        // Reset view to our camera and enable lighting again
        window.setView(camera);
        sf3d::Light::enableLighting();

        // Finally, display the rendered frame on screen
        window.display();
    }

    return EXIT_SUCCESS;
}
