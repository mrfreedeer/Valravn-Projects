#pragma once
#include "Engine/Math/AABB2.hpp"
#include <vector>

class ConvexPolyShape2D;

struct AABB2TreeNode {
	AABB2 m_bounds = AABB2::ZERO_TO_ONE;
	AABB2TreeNode* m_firstChild = nullptr;
	AABB2TreeNode* m_secondChild = nullptr;
	std::vector<int> m_containedShapesIndexes;

	bool HasChildren() const { return m_firstChild || m_secondChild; }
};