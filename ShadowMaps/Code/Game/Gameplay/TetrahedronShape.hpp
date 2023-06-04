#pragma once
#include "Game/Gameplay/ConvexPoly3DShape.hpp"

class TetrahedronShape : public ConvexPoly3DShape {
public:
	TetrahedronShape(Game* game, Vec3 position, float height);

};