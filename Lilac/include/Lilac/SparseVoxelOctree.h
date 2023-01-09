#ifndef LILAC_SPARSE_VOXEL_OCTREE_H
#define LILAC_SPARSE_VOXEL_OCTREE_H

#include <Lilac/OpenGL.h>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
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
	// Do we want this walk to hit non-leaf nodes as well
	void walk(const std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)>& func);

	// Buffer format is
	// [Header: uint subtree_count, uint voxel_count, uint scale16u, uint padding, vec4 min]
	// [Subtree[subtree_count]: vec4 min_scale16u, uint[8] children_indices] // vec4*3
	// [Voxel[voxel_count]: vec4 min_materialId16u_scale16u] // vec4*1
	[[nodiscard]] std::vector<std::byte> flatten() const; // TODO: Figure out if we need to do endianness stuff here, maybe use unique pointer here?

private:
	struct Node
	{
		glm::vec3 min;
		uint16_t scale;
		uint16_t materialId;
		Node* children[8]{};

	public:
		Node(glm::vec3 min, uint16_t scale, uint16_t materialId);
		~Node();

		[[nodiscard]] bool isLeaf() const;
		[[nodiscard]] bool isChildrenHomogenous() const;
	};

	void walk_internal(const std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)>& func, Node* node, std::vector<size_t> indices);

	void addVoxels(const std::vector<Voxel>& voxels);
	void addVoxel(Node* parent, size_t leafIndex, Node* node, Voxel voxel);
	void splitLeaf(Node* parent, size_t leafIndex, Node* leaf);

	bool tryCollapseNodes();
	bool tryCollapseNode(Node* parent, size_t childIndex, Node* node);
	void collapseNode(Node* parent, size_t childIndex, Node* node);

	static glm::vec3 octantIndexToOffset(size_t i);
	static size_t globalPositionToChildIndex(Node* node, uint16_t x, uint16_t y, uint16_t z);

	void gatherNodes(
		Node* current,
		std::map<Node*, size_t>& parent_to_index,
		std::map<Node*, size_t>& leaf_to_index,
		std::vector<Node*>& parents,
		std::vector<Node*>& leaves) const;


	void flattenedWriteHeader(
		std::vector<std::byte>& vec,
		const std::vector<Node*>& parents,
		const std::vector<Node*>& leaves) const;

	static void flattenedWriteParents(
		std::vector<std::byte>& vec,
		const std::map<Node*, size_t>& parent_to_index,
		const std::map<Node*, size_t>& leaf_to_index,
		const std::vector<Node*>& parents);

	static void flattenedWriteLeaves(
		std::vector<std::byte>& vec,
		const std::vector<Node*>& leaves);

	static void pushFloat(std::vector<std::byte>& vec, GLfloat x);
	static void pushGLuint(std::vector<std::byte>& vec, GLuint x);
	static void pushUint16(std::vector<std::byte>& vec, uint16_t x);
	static void pushVec3AsVec4(std::vector<std::byte>& vec, glm::vec3 x);
	static void pushBytes(std::vector<std::byte>& vec, std::byte const* x, size_t byte_count);

	Node* m_head; // TODO: Figure some way to not recompute the gl buffer for every modification?
};
}

#endif // LILAC_SPARSE_VOXEL_OCTREE_H