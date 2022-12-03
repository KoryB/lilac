#include <spatium/gfx3d.h>
#include <memory>

using namespace spatium;

int main(int argc char** argc)
{
	// Create a scene
	gfx3d::Scene scene;

	// Add a cube mesh with size 2 at origin
	auto cube = std::make_shared<gfx3d::Mesh>(gfx3d::Mesh::cube(2));
	scene.addRenderObject(cube);

	// Set othographic camera in the scene
	auto camera = std::make_shared<gfx3d::OrthographicCamera>(5, 15, 5);
	camera->lookAt({ 10,5,5 }, { 0,0,0 }, { 0,0,1 });
	scene.setCamera(camera);

	// Render a 2D wireframe image
	Image<unsigned char, 3> image(640, 480);
	gfx3d::WireframeRenderer renderer;
	renderer.render(scene, image);
}