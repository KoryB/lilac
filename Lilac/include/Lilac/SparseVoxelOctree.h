#ifndef LILAC_SPARSE_VOXEL_OCTREE_H
#define LILAC_SPARSE_VOXEL_OCTREE_H

#include <Lilac/OpenGL.h>
#include <glm/vec3.hpp>

#include <stdint.h>
#include <vector>
#include <functional>

namespace Lilac
{
class SparseVoxelOctree
{
public:
	struct Voxel
	{
		uint16_t x, y, z;
		uint16_t materialId;
	};

	SparseVoxelOctree(glm::vec3 min, const std::vector<Voxel>& voxels);

	// Vector of indices, min, scale, materialId
	void walk(std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)> func);

private:
	struct Node
	{
		glm::vec3 min;
		uint16_t scale;
		uint16_t materialId;
		Node* children[8];

	public:
		Node(glm::vec3 min, uint16_t scale, uint16_t materialId);
		~Node();

		bool isLeaf();
		bool isChildrenHomogenous();
	};

	void walk_internal(std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)> func, Node* node, std::vector<size_t> indices);


	void addVoxels(const std::vector<Voxel>& voxels);
	void addVoxel(Node* parent, size_t leafIndex, Node* node, Voxel voxel);

	void collapseNodes();
	void collapseNode(Node* parent, size_t leafIndex, Node* node);

	void splitLeaf(Node* parent, size_t leafIndex, Node* leaf);
	glm::vec3 octentIndexToOffset(size_t i);
	size_t globalPositionToChildIndex(Node* node, uint16_t x, uint16_t y, uint16_t z);


	Node* m_head; // TODO: Figure some way to not recompute the glbuffer for every modification?
	GLuint m_buffer;
};
}

#endif // LILAC_SPARSE_VOXEL_OCTREE_H