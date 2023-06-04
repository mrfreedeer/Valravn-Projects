#pragma once
#include <vector>
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Plane3D.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

struct HairData {
	Vec4 Position;
	Vec4 Velocity;
};

struct HairUAVInfo {
	std::vector<Vertex_PNCU>* InitialHairData = nullptr;
	UnorderedAccessBuffer* Out_HairPositions = nullptr;
	UnorderedAccessBuffer* Out_PreviousHairPositions = nullptr;
	UnorderedAccessBuffer* Out_VirtualHairPositions = nullptr;
};


struct Vertex_PNCU;
struct HairConstants {
	HairConstants() = default;
	HairConstants(HairConstants const& copyFrom) = default;

	Vec3 EyePosition = Vec3::ZERO;
	float HairWidth = 0.001f;
	// -------------------------------------------
	float DiffuseCoefficient = 0.4f;
	float SpecularCoefficient = 0.6f;
	int SpecularExponent = 4;
	float StartTime = 0.0f;
	// -------------------------------------------
	int InterpolationFactor = 1;
	int InterpolationFactorMultiStrand = 1;
	float InterpolationRadius = 1.0f;
	float TessellationFactor = 1.0f;
	// -------------------------------------------
	float LongitudinalWidth = 5.0f;
	float ScaleShift = -10.0f;
	unsigned int  UseUnrealParameters = 1;
	float SpecularMarchner = 0.5f;
	// -------------------------------------------
	float DeltaTime = 0.0f;
	Vec3 ExternalForces = Vec3::ZERO;
	// -------------------------------------------
	Vec3 Displacement = Vec3::ZERO;
	float Gravity = -9.8f;
	// -------------------------------------------
	float SegmentLength = 0.1f;
	float BendInitialLength = 0.2f;
	float TorsionInitialLength = 0.3f;
	float DampingCoefficient = 0.9f;
	// -------------------------------------------
	float EdgeStiffness = 1800.0f;
	float BendStiffness = 1800.0f;
	float TorsionStiffness = 1800.0f;
	unsigned int  IsHairCurly = true;
	// -------------------------------------------
	float Mass = 3.0f;
	unsigned int SimulationAlgorithm = 0;
	float GridCellWidth = 100.0f;
	float GridCellHeight = 100.0f;
	// -------------------------------------------
	float FrictionCoefficient = 0.1f;
	IntVec3 GridDimensions = IntVec3::ZERO;
	// -------------------------------------------
	float CollisionTolerance = 0.05f;
	unsigned int HairSegmentCount = 12;
	float StrainLimitingCoefficient = 0.5f;
	unsigned int  InterpolateUsingRadius = 1;
	// -------------------------------------------
	//Plane3D LimitingPlane;
	// -------------------------------------------
	float MarschnerTransmCoeff = 0.015f;
	float MarschnerTRTCoeff = 1.0f;
	unsigned int UseModelColor = 1;
	unsigned int InvertLightDir = 0;

};

struct HairGuide {
public:
	HairGuide(Vec3 const& position, Rgba8 const& color, Vec3 const& normal, int segmentCount);
	HairGuide(Vec3 const& position, Rgba8 const& color, Vec3 const& startNormal, Vec3 const& endNormal, int segmentCount);

	void AddVerts(std::vector<Vertex_PNCU>& hairVertexes) const;

	static float HairWidth;
	static float HairSegmentLength;
	static unsigned int HairSegmentCount;
private:
	Vec3 m_position = Vec3::ZERO;
	Rgba8 m_color = Rgba8::WHITE;
	Vec3 m_startNormal = Vec3::ZERO;
	Vec3 m_endNormal = Vec3::ZERO;
	int m_segmentCount = 0;
};


enum class SimulAlgorithm {
	DFTL = 0,
	MASS_SPRINGS,
	NUM_SIMULATION_ALGORITHMS
};
struct HairSimulationInit {
	Vec3 position = Vec3::ZERO;
	Vec3 normal = Vec3::ZERO;
	float mass = 1.0f;
	float edgeStiffness = 1800.0f;
	float bendStiffness = 1800.0f;
	float torsionStiffness = 1800.0f;
	float damping = 0.9f;
	int segmentCount = 12;
	SimulAlgorithm usedAlgorithm = SimulAlgorithm::DFTL;
	bool isCurlyHair = false;
};

class HairSimulGuide {
public:
	HairSimulGuide(HairSimulationInit const& hairSimulationInitial);

	void AddVerts(std::vector<Vertex_PNCU>& hairVertexes) const;
	void Update(float deltaSeconds);
	void AddForce(Vec3 force);
	void SetSpringStiffness(float const& edgeStiffness, float const& bendStiffness, float const& torsionStiffness);
	void GetSpringStiffness(float& edgeStiffness, float& bendStiffness, float& torsionStiffness) const;

	void SetSpringLengths(float const& edgeLength, float const& bendLength, float const& torsionLength);
	void GetSpringLengths(float& edgeLength, float& bendLength, float& torsionLength) const;
	float m_gravity = -9.8f;
public:
	//Vec3 CalculateHalfVelocity(float deltaSeconds);
	void CalculateRestitutionForces(float deltaSeconds, bool useHalfPosition = false);
	void CalculateRestitutionForcesCurly(float deltaSeconds, bool useHalfPosition);
	void CalculateRestitutionForcesStraight(float deltaSeconds, bool useHalfPosition);
	Vec3 GetSpringForce(float deltaSeconds, Vec3 const& positionOne, Vec3 const& positionTwo, float stiffness, float initialLength);
	void UpdateDFTL(float deltaSeconds); // Dynamic Follow The Leader
	void UpdateMassSprings(float deltaSeconds);

	Vec3 const GetVirtualParticlePos(int startIndex) const;

	std::vector<Vec3> m_virtualParticleVelocities;
	std::vector<Vec3> m_virtualParticleHalfVelocities;
	std::vector<Vec3> m_virtualParticlePositions;
	std::vector<Vec3> m_virtualParticlePrevPositions;
	std::vector<Vec3> m_velocities;
	std::vector<Vec3> m_halfVelocities;
	std::vector<Vec3> m_positions;
	std::vector<Vec3> m_prevPositions;
	std::vector<Vec3> m_forces;
	std::vector<Vec3> m_forcesParticles;

	float m_segmentLength = 0.0f;
	float m_bendInitialLength = 0.0f;
	float m_torsionInitialLength = 0.0f;

	HairSimulationInit m_hairSimulationInitParams;

	Vec3 m_genForce = Vec3::ZERO;
};


struct HairObjectInit {
	Vec3 m_startingPosition = Vec3::ZERO;
	Shader* m_shader = nullptr;
	Shader* m_simulationShader = nullptr;
	Shader* m_multInterpShader = nullptr;
	Image* m_hairDensityMap = nullptr;
	Image* m_hairDiffuseMap = nullptr;
	HairConstants** m_usedConstantBuffer = nullptr;

};

class HairObject :public Entity {
public:
	HairObject(Game* pointerToGame, HairObjectInit const& initParams);
	int GetHairCount() const { return m_hairCount; }
	virtual int GetMultiStrandBaseCount() const { return 0; }
	virtual ~HairObject();

	virtual void CreateHair() = 0;
	virtual void InitializeHairUAV(HairUAVInfo& uavInitialInfo);
	virtual void InitializeHairUAV() = 0;
	virtual void Render() const;
	virtual void Update(float deltaSeconds) override;
	virtual int GetThreadsToDispatch(size_t amountOfHairPos) const;

	virtual void SetShader(Shader* newShader);
	virtual void SetMutlInterpShader(Shader* newMultInterpShader);

	static int HairPerSection;
	static int SectionCount;
	static int StackCount;
	static int SliceCount;
	static float Radius;


	float m_diffuseCoefficient = g_gameConfigBlackboard.GetValue("HAIR_DIFFUSE_COEFFICIENT", 0.9f);
	float m_specularCoefficient = g_gameConfigBlackboard.GetValue("HAIR_SPECULAR_COEFFICIENT", 0.4f);
	int m_specularExponent = g_gameConfigBlackboard.GetValue("HAIR_SPECULAR_EXPONENT", 2);

	int m_hairPerSection = 0;
	int m_sectionCount = 0;
	float m_radius = g_gameConfigBlackboard.GetValue("DISC_RADIUS", 5.0f);
	int m_sliceCount = 0;
	int m_stackCount = 0;

protected:
	HairObjectInit m_initParams;
	std::vector<HairGuide> m_hairs;
	std::vector<Vertex_PNCU> m_hairVertexes;
	VertexBuffer* m_vertexBuffer = nullptr;
	UnorderedAccessBuffer* m_hairPositionUAV = nullptr;
	UnorderedAccessBuffer* m_hairPrevPositionUAV = nullptr;
	UnorderedAccessBuffer* m_virtualPositionUAV = nullptr;
	UnorderedAccessBuffer* m_gridUAV = nullptr;

	int m_hairCount = 0;
};


struct HairCollisionObject {
	Vec3 Position = Vec3::ZERO;
	float Radius = 0.0f;
};