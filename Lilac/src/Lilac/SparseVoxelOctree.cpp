#include <Lilac/SparseVoxelOctree.h>

#include <glm/vec3.hpp>

#include <cstddef>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <functional>


Lilac::SparseVoxelOctree::SparseVoxelOctree(glm::vec3 min, const std::vector<Voxel>& voxels)
	: m_head(nullptr)
{
	uint16_t scale = 1;

	for (const auto& voxel : voxels)
	{
		// TODO: This runs into issues in a 2x2x2 cuboid for some reason
		scale = std::max(std::max(std::max(scale, voxel.x), voxel.y), voxel.z);
	}

	uint16_t powerOfTwoScale = 1;
	while (powerOfTwoScale < scale)
	{
		powerOfTwoScale *= 2;
	}

	m_head = new Node(min, powerOfTwoScale, 0);

	addVoxels(voxels);
}

void Lilac::SparseVoxelOctree::walk(const std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)>& func)
{
	walk_internal(func, m_head, { });
}

std::vector<std::byte> Lilac::SparseVoxelOctree::flatten() const
{
	std::vector<std::byte> flattened;
	std::map<Node*, size_t> parentToIndex;
	std::map<Node*, size_t> leafToIndex;
	std::vector<Node*> parents;
	std::vector<Node*> leaves;

	gatherNodes(m_head, parentToIndex, leafToIndex, parents, leaves);

	flattenedWriteHeader(flattened, parents, leaves);
	flattenedWriteParents(flattened, parentToIndex, leafToIndex, parents);
	flattenedWriteLeaves(flattened, leaves);

	return flattened;
}

void Lilac::SparseVoxelOctree::flattenedWriteHeader(
	std::vector<std::byte>& vec, 
	const std::vector<Node*>& parents, 
	const std::vector<Node*>& leaves) const
{
	pushGLuint(vec, parents.size());  // parent_count
	pushGLuint(vec, leaves.size());   // leaf_count
	pushGLuint(vec, m_head->scale);   // scale16u
	pushGLuint(vec, 0);               // padding
	pushVec3AsVec4(vec, m_head->min); // min
}

void Lilac::SparseVoxelOctree::flattenedWriteParents(
	std::vector<std::byte>& vec, 
	const std::map<Node*, size_t>& parentToIndex,
	const std::map<Node*, size_t>& leafToIndex,
	const std::vector<Node*>& parents)
{
	size_t parent_count = parents.size();

	for (const Node* parent : parents)
	{
		pushFloat(vec, parent->min.x);
		pushFloat(vec, parent->min.y);
		pushFloat(vec, parent->min.z);
		pushGLuint(vec, parent->scale);

		for (Node* child : parent->children)
		{
			size_t index = child->isLeaf()
				? parent_count + leafToIndex.at(child)
				: parentToIndex.at(child);

			pushGLuint(vec, index);
		}
	}
}

void Lilac::SparseVoxelOctree::flattenedWriteLeaves(
	std::vector<std::byte>& vec, 
	const std::vector<Node*>& leaves)
{

	for (Node* leaf : leaves)
	{
		pushFloat(vec, leaf->min.x);
		pushFloat(vec, leaf->min.y);
		pushFloat(vec, leaf->min.z);
		pushUint16(vec, leaf->materialId);
		pushUint16(vec, leaf->scale);
	}
}

void Lilac::SparseVoxelOctree::gatherNodes(Node* current,
	std::map<Node*, size_t>& parentToIndex,
	std::map<Node*, size_t>& leafToIndex,
	std::vector<Node*>& parents,
	std::vector<Node*>& leaves) const
{
	if (current->isLeaf())
	{
		leafToIndex[current] = leaves.size();
		leaves.push_back(current);
	}
	else
	{
		parentToIndex[current] = parents.size();
		parents.push_back(current);

		for (auto child : current->children)
		{
			gatherNodes(child, parentToIndex, leafToIndex, parents, leaves);
		}
	}
}

void Lilac::SparseVoxelOctree::pushFloat(std::vector<std::byte>& vec, GLfloat x)
{
	auto x_p = reinterpret_cast<std::byte const*>(&x);

	pushBytes(vec, x_p, 4);
}

void Lilac::SparseVoxelOctree::pushGLuint(std::vector<std::byte>& vec, GLuint x)
{
    auto x_p = reinterpret_cast<std::byte const*>(&x);

	pushBytes(vec, x_p, 4);
}

void Lilac::SparseVoxelOctree::pushUint16(std::vector<std::byte>& vec, uint16_t x)
{
    auto x_p = reinterpret_cast<std::byte const*>(&x);

	pushBytes(vec, x_p, 2);
}

void Lilac::SparseVoxelOctree::pushVec3AsVec4(std::vector<std::byte>& vec, glm::vec3 x)
{
	pushFloat(vec, x.x);
	pushFloat(vec, x.y);
	pushFloat(vec, x.z);
	pushFloat(vec, 0.0f);
}

void Lilac::SparseVoxelOctree::pushBytes(std::vector<std::byte>& vec, std::byte const* x, size_t byte_count)
{
	for (int i = 0; i < byte_count; i++)
	{
		vec.push_back(*(x + i));
	}
}


void Lilac::SparseVoxelOctree::walk_internal(const std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)>& func, Node* node, std::vector<size_t> indices)
{
	if (node->isLeaf())
	{
		func(indices, node->min, node->scale, node->materialId);
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			indices.push_back(i);
			walk_internal(func, node->children[i], indices);
			indices.pop_back();
		}
	}
}

void Lilac::SparseVoxelOctree::addVoxels(const std::vector<Voxel>& voxels)
{
	for (const auto& voxel : voxels)
	{
		addVoxel(nullptr, 0, m_head, voxel);
	}

	tryCollapseNodes();
}

//TODO: This assumes the voxel is inside the node
// This also overwrites voxels which are already there, which seems fine
void Lilac::SparseVoxelOctree::addVoxel(Node* parent, size_t leafIndex, Node* node, Voxel voxel)
{
	if (node->isLeaf())
	{
		if (node->scale == 1)
		{
			node->materialId = voxel.materialId;
			return;
		}
		else
		{
			splitLeaf(parent, leafIndex, node);

			if (parent == nullptr)
			{
				node = m_head;
			}
			else
			{
				node = parent->children[leafIndex];
			}
		}
	}

	auto childIndex = globalPositionToChildIndex(node, voxel.x, voxel.y, voxel.z);
	addVoxel(node, childIndex, node->children[childIndex], voxel);
}

bool Lilac::SparseVoxelOctree::tryCollapseNodes()
{
	return tryCollapseNode(nullptr, 0, m_head);
}

bool Lilac::SparseVoxelOctree::tryCollapseNode(Node* parent, size_t childIndex, Node* node)
{
	if (node->isLeaf())
	{
		return false;
	}

	for (int i = 0; i < 8; i++)
	{
		tryCollapseNode(node, i, node->children[i]);

		if (parent == nullptr)
		{
			node = m_head;
		}
		else
		{
			node = parent->children[childIndex];
		}
	}

	if (node->isChildrenHomogenous())
	{
		collapseNode(parent, childIndex, node);

		return true;
	}

	return false;
}

void Lilac::SparseVoxelOctree::collapseNode(Node* parent, size_t childIndex, Node* node)
{
	std::cout << "Collapsing Node: " << "<" << node->min.x << ", " << node->min.y << ", " << node->min.z << ">" <<
		"Scale: " << node->scale << " " <<
		"MaterialId: " << node->materialId << std::endl;

	auto min = node->min;
	auto scale = node->scale;
	auto materialId = node->children[0]->materialId;

	delete node;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "DanglingPointer"
	if (node == m_head)
	{
		m_head = new Node(min, scale, materialId);
	}
	else
	{
		parent->children[childIndex] = new Node(min, scale, materialId);
	}
#pragma clang diagnostic pop
}

glm::vec3 Lilac::SparseVoxelOctree::octantIndexToOffset(size_t i)
{
	return {
		i & 0b001,
		(i & 0b010) >> 1,
		(i & 0b100) >> 2
	};
}

size_t Lilac::SparseVoxelOctree::globalPositionToChildIndex(Node* node, uint16_t x, uint16_t y, uint16_t z)
{
	auto min = node->min;
	auto halfScale = node->scale / 2.0f;

	return ((float)x - min.x >= halfScale) * 1 +
		((float)y - min.y >= halfScale) * 2 +
		((float)z - min.z >= halfScale) * 4;
}

void Lilac::SparseVoxelOctree::splitLeaf(Node* parent, size_t leafIndex, Node* leaf)
{
	auto min = leaf->min;
	auto halfScale = leaf->scale / 2;
	auto materialId = leaf->materialId;
	Node* octant;

	if (parent != nullptr)
	{
        octant = new Node(min, leaf->scale, materialId);
	}
	else
	{
        octant = m_head;
	}

	for (int i = 0; i < 8; i++)
	{
        octant->children[i] = new Node(min + (float)halfScale * octantIndexToOffset(i), halfScale, materialId);
	}

	if (parent != nullptr)
	{
		delete leaf;
		parent->children[leafIndex] = octant;
	}
}


// Initialize a leaf node
Lilac::SparseVoxelOctree::Node::Node(glm::vec3 min, uint16_t scale, uint16_t materialId)
	: min(min)
	, scale(scale)
	, materialId(materialId)
    , children {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
{
}

Lilac::SparseVoxelOctree::Node::~Node()
{
	for (auto & i : children)
	{
		delete i;
		i = nullptr;
	}
}

bool Lilac::SparseVoxelOctree::Node::isLeaf() const
{
	return children[0] == nullptr;
}

bool Lilac::SparseVoxelOctree::Node::isChildrenHomogenous() const
{
	if (isLeaf())
	{
		return false;
	}

	for (auto i : children)
	{
		if (!i->isLeaf())
		{
			return false;
		}
	}

	auto materialId = children[0]->materialId;
    return std::ranges::all_of(
        children,
        [materialId](const Node* child) { return child->materialId == materialId; }
    );
}