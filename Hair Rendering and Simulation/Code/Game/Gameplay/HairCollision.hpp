#pragma once
#include "Engine/Renderer/ConstantBuffer.hpp"

class ConstantBuffer;
struct Vec3;

void StartupHairCollision();
void ShutdownHairCollision();
void ClearCollisionObjects();
void AddHairCollisionSphere(Vec3 const& position, float radius);
ConstantBuffer* GetHairCollisionBuffer();