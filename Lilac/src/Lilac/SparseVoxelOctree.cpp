#include <Lilac/SparseVoxelOctree.h>

#include <glm/vec3.hpp>

#include <iostream>
#include <algorithm>
#include <vector>
#include <functional>


Lilac::SparseVoxelOctree::SparseVoxelOctree(glm::vec3 min, const std::vector<Voxel>& voxels)
	: m_head(nullptr)
	, m_buffer()
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
		powerOfTwoScale = powerOfTwoScale *= 2;
	}

	m_head = new Node(min, powerOfTwoScale, 0);

	addVoxels(voxels);
}

void Lilac::SparseVoxelOctree::walk(std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)> func)
{
	walk_internal(func, m_head, { });
}

void Lilac::SparseVoxelOctree::walk_internal(std::function<void(std::vector<size_t>, glm::vec3, uint16_t, uint16_t)> func, Node* node, std::vector<size_t> indices)
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

	collapseNodes();
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

void Lilac::SparseVoxelOctree::collapseNodes()
{
	collapseNode(nullptr, 0, m_head);
}

void Lilac::SparseVoxelOctree::collapseNode(Node* parent, size_t childIndex, Node* node)
{
	if (node->isLeaf())
	{
		return;
	}

	std::cout << "Collapsing Node: " << "<" << node->min.x << ", " << node->min.y << ", " << node->min.z << ">" << "Scale: " << node->scale << std::endl;

	for (int i = 0; i < 8; i++)
	{
		collapseNode(node, i, node->children[i]);

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
		auto min = node->min;
		auto scale = node->scale;
		auto materialId = node->children[0]->materialId;
		delete node;

		if (node == m_head)
		{
			m_head = new Node(min, scale, materialId);
		}
		else
		{
			parent->children[childIndex] = new Node(min, scale, materialId);
		}
	}
}

glm::vec3 Lilac::SparseVoxelOctree::octentIndexToOffset(size_t i)
{
	return glm::vec3(
		i & 0b001,
		(i & 0b010) >> 1,
		(i & 0b100) >> 2
	);
}

size_t Lilac::SparseVoxelOctree::globalPositionToChildIndex(Node* node, uint16_t x, uint16_t y, uint16_t z)
{
	auto min = node->min;
	auto halfScale = node->scale / 2;

	return ((float)x - min.x >= halfScale) * 1 +
		((float)y - min.y >= halfScale) * 2 +
		((float)z - min.z >= halfScale) * 4;
}

void Lilac::SparseVoxelOctree::splitLeaf(Node* parent, size_t leafIndex, Node* leaf)
{
	auto min = leaf->min;
	auto halfScale = leaf->scale / 2;
	auto materialId = leaf->materialId;
	Node* octent;

	if (parent != nullptr)
	{
		octent = new Node(min, leaf->scale, materialId);
	}
	else
	{
		octent = m_head;
	}

	for (int i = 0; i < 8; i++)
	{
		octent->children[i] = new Node(min + (float)halfScale * octentIndexToOffset(i), halfScale, materialId);
	}

	if (parent != nullptr)
	{
		delete leaf;
		parent->children[leafIndex] = octent;
	}
}


// Initialize a leaf node
Lilac::SparseVoxelOctree::Node::Node(glm::vec3 min, uint16_t scale, uint16_t materialId)
	: min(min)
	, scale(scale)
	, materialId(materialId)
{
	for (int i = 0; i < 8; i++)
	{
		children[i] = nullptr;
	}
}

Lilac::SparseVoxelOctree::Node::~Node()
{
	for (int i = 0; i < 8; i++)
	{
		delete children[i];
		children[i] = nullptr;
	}
}

bool Lilac::SparseVoxelOctree::Node::isLeaf()
{
	return children[0] == nullptr;
}

bool Lilac::SparseVoxelOctree::Node::isChildrenHomogenous()
{
	if (isLeaf())
	{
		return false;
	}

	if (!children[0]->isLeaf())
	{
		return false;
	}

	auto materialId = children[0]->materialId;

	for (auto child : children)
	{
		if (child->materialId != materialId)
		{
			return false;
		}
	}

	return true;
}