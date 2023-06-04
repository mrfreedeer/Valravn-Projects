#include "Game/Gameplay/TetrahedronShape.hpp"

TetrahedronShape::TetrahedronShape(Game* game, Vec3 position, float height) :
	ConvexPoly3DShape(game, position, ConvexPoly3D())
{
	float heightHalf = height * 0.5f;
	std::vector<Vec3> vertexes{
		Vec3(0.0f, 0.0f,heightHalf),
		Vec3(heightHalf, 0.0f, -heightHalf),
		Vec3(-heightHalf, heightHalf,-heightHalf),
		Vec3(-heightHalf, -heightHalf,-heightHalf)
	};

	std::vector<Face> faces{
		Face{std::vector<int>({2,3,0})},
		Face{std::vector<int>({3,1,0})},
		Face{std::vector<int>({1,2,0})},
		Face{std::vector<int>({1,3,2})},
	};

	m_convexPoly.m_faces.insert(m_convexPoly.m_faces.end(), faces.begin(), faces.end());
	m_convexPoly.m_vertexes.insert(m_convexPoly.m_vertexes.end(), vertexes.begin(), vertexes.end());
	RandomizeFaceColors();
	m_convexPoly.CalculatePolyInfo();
}
