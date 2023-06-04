#include "Game/Gameplay/GameMode.hpp"
#include "Game/Gameplay/Entity.hpp"

class Player;
class Shape3D;

struct AABB3;

enum class ShapeType {
	Sphere = 0,
	Box,
	ZCylinder,
	NUM_SHAPE_TYPES
};

struct Shape3DRaycastResult : RaycastResult3D {
	Shape3DRaycastResult() {};
	Shape3DRaycastResult(RaycastResult3D const& raycastResult) {
		m_startPosition = raycastResult.m_startPosition;
		m_forwardNormal = raycastResult.m_forwardNormal;
		m_impactPos = raycastResult.m_impactPos;
		m_impactNormal = raycastResult.m_impactNormal;

		m_impactFraction = raycastResult.m_impactFraction;
		m_impactDist = raycastResult.m_impactDist;
		m_didImpact = raycastResult.m_didImpact;
		m_maxDistance = raycastResult.m_maxDistance;
		m_maxDistanceReached = raycastResult.m_maxDistanceReached;
	}


	Shape3D* m_shapeHit = nullptr;
};

class Shapes3DMode : public GameMode {
public:
	Shapes3DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);
	~Shapes3DMode();

	virtual void Startup() override;
	virtual void Shutdown() override;

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	void UpdateEntities(float deltaSeconds);
	void UpdateShapes();
	virtual void UpdateInput(float deltaSeconds) override;

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderShapes() const;
	void CheckShapeOverlap() const;

	void CheckSpheresVsSpheres() const;
	void CheckSpheresVsBoxes() const;
	void CheckSpheresVsZCylinders() const;

	void CheckBoxesVsBoxes() const;
	void CheckBoxesVsZCylinders() const;

	void CheckZCylindersVsZCylinders() const;

	void GetNearestPointSpheres();
	void GetNearestPointBoxes();
	void GetNearestPointCylinders();

	Vec3 const GetRandomPositionWithinAABB3(AABB3 const& bounds) const;
	Shape3DRaycastResult GetNearestRaycastHit() const;

	Shape3DRaycastResult RaycastVsSpheres() const;
	Shape3DRaycastResult RaycastVsBoxes() const;
	Shape3DRaycastResult RaycastVsCylinders() const;

	void UpdateRaycast();

private:
	EntityList m_allEntities;
	std::vector<Shape3D*> m_allShapes;
	std::vector<Shape3D*> m_shapesByType[(int)ShapeType::NUM_SHAPE_TYPES];
	std::vector<Vec3> m_allNearestPoints;
	Player* m_player = nullptr;
	bool m_lostFocusBefore = false;
	bool m_isCursorHidden = false;
	bool m_isCursorClipped = false;
	bool m_isCursorRelative = false;

	float m_raycastLength = g_gameConfigBlackboard.GetValue("RAYCAST_LENGTH", 10.0f);
	Vec3 m_rayStart = Vec3::ZERO;
	Vec3 m_rayForward = Vec3::ZERO;

	bool m_isRaycastLocked = false;

	Shape3D* m_closestHitShape = nullptr;
	Shape3D* m_lockedShape = nullptr;

	int m_minShapeAmount = g_gameConfigBlackboard.GetValue("SHAPES_3D_MIN", 10);
	int m_maxShapeAmount = g_gameConfigBlackboard.GetValue("SHAPES_3D_MAX", 12);

	Vec3 m_lastPosition = Vec3(-15.0f, -10.0f, 0.0f);
	EulerAngles m_lastOrientation = EulerAngles::ZERO;
};