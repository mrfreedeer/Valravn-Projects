#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"

struct Vertex_PCU {
public:
	Vec3  m_position;
	Rgba8 m_color;
	Vec2 m_uvTexCoords;

public:
	Vertex_PCU();
	explicit Vertex_PCU(Vec3 const& position, Rgba8 const& tint, Vec2 const& uvTexCoords);
	explicit Vertex_PCU(float x, float y, float z, Rgba8 const& tint, float u, float v);
};