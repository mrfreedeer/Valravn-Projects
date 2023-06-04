#pragma once
#include "Game/Gameplay/GameMode.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/FloatRange.hpp"

enum class BumperType {
	Disc,
	Box,
	Capsule,
	NUM_BUMPER_TYPES
};


struct PachinkoBall {
public:

	PachinkoBall(Vec2 const& position, Vec2 const& initialVelocity, float radius, float elasticity);

	void Update(float deltaSeconds);
	void AddVertsForRendering(std::vector<Vertex_PCU>& verts) const;

	float const m_elasticity = 0.0f;
	float const m_radius = 0.0f;
	Vec2 m_position = Vec2::ZERO;
	Vec2 m_velocity = Vec2::ZERO;

	Rgba8 const m_color = Rgba8::WHITE;
};

struct PachinkoBumper {
public:
	PachinkoBumper(BumperType bumperType, Vec2 const& position, Vec2 const& dimensions, float orientation = 0.0f);

	void AddVertsForDrawing(std::vector<Vertex_PCU>& verts);
	void BounceBallOffBumper(PachinkoBall& ball) const;

	void BounceBallOffDiscBumper(PachinkoBall& ball) const;
	void BounceBallOffCapsuleBumper(PachinkoBall& ball) const;
	void BounceBallOffOBB2Bumper(PachinkoBall& ball) const;

	Vec2 const m_position = Vec2::ZERO;
	Vec2 const m_dimensions = Vec2::ZERO;
	BumperType const m_bumperType = BumperType::Disc;
	Rgba8 m_color = Rgba8::WHITE;
	float const m_orientation = 0.0f;
	float m_boundsRadius = 0.0f;
	float m_elasticity = 0.0f;

	OBB2 m_obb2;
	Capsule2 m_capsule;
};


class PachinkoMachineMode : public GameMode {
public:
	PachinkoMachineMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize);
	~PachinkoMachineMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	virtual void UpdateInput(float deltaSeconds) override;
	void AddVertsForPachinkoBallsAndRaycast();
	void AddVertsForRaycastImpactOnDisc(PachinkoBall& shape, RaycastResult2D& raycastResult);

	void UpdatePachinkoBalls(float deltaSeconds);

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderBumpers() const;
	void SpawnNewBall(Vec2 const& pos, Vec2 const& velocity);

	void CheckBallsCollisionWithEachOther();
	void CheckBallsCollisionsWithWalls();
	void CheckBallsCollisionsWithBumpers();
	void BounceBallsOfEachOtherSpecialPachinko(Vec2& aCenter, Vec2& aVelocity, float aRadius, Vec2& bCenter, Vec2& bVelocity, float bRadius, float combinedElasticity);
private:
	std::vector<PachinkoBall> m_allBalls;
	std::vector<PachinkoBumper> m_bumpers;
	std::vector<PachinkoBumper> m_obb2Bumpers;
	std::vector<PachinkoBumper> m_discBumpers;
	std::vector<PachinkoBumper> m_capsuleBumpers;

	std::vector<PachinkoBall*> m_highlightedBalls;
	std::vector<Vertex_PCU> m_raycastVsDiscCollisionVerts;

	Vec2 m_rayStart = Vec2::ZERO;
	Vec2 m_rayEnd = Vec2::ZERO; 

	RaycastResult2D m_raycastResult;
	bool m_impactedDisc = false;
	float m_dotSpeed = g_gameConfigBlackboard.GetValue("DOT_SPEED", 40.0f);

	std::vector<Vertex_PCU> m_bumperVerts;
	std::vector<Vertex_PCU> m_ballsVerts;

	FloatRange m_ballSizeRange = g_gameConfigBlackboard.GetValue("PACHINKO_BALL_SIZE_RANGE", FloatRange::ZERO_TO_ONE);
	float m_defaultBallElasticity = g_gameConfigBlackboard.GetValue("PACHINKO_DEFAULT_BALL_ELASTICITY", 0.9f);
	float m_defaultPhysicsTimeStepMiliseconds = g_gameConfigBlackboard.GetValue("PACHINKO_PHYSICS_TIME_STEP_MS", 0.005f);
	float m_owedPhysicsTime = 0.0f;

	bool m_collapseFloor = false;
	bool m_useTimeStep = false;
	std::string m_originalHelperText = "F8 to randomize. LMB/RMB set ray start/end. Hold T = slow. B = Collapse Floor. L toggle TimeStep";

};