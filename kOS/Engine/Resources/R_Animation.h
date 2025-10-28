#pragma once
#include "Config/pch.h"
#include "Resource.h"
#include "ResourceHeader.h"
class R_Animation :public Resource
{
	class Bone
	{
	public:
		glm::mat4 Interpolate(float time) const;

		const std::string& GetName() const;
		int GetID() const;

		std::string m_Name;
		int m_ID;

		std::vector<glm::vec3> m_Positions;
		std::vector<float> m_PosTimes;

		std::vector<glm::quat> m_Rotations;
		std::vector<float> m_RotTimes;

		std::vector<glm::vec3> m_Scales;
		std::vector<float> m_ScaleTimes;

		int FindIndex(const std::vector<float>& times, float animTime) const;
		float GetFactor(float start, float end, float time) const;

		glm::mat4 InterpolatePosition(float time) const;
		glm::mat4 InterpolateRotation(float time) const;
		glm::mat4 InterpolateScale(float time) const;
	};

	struct NodeData
	{
		std::string name;
		glm::mat4 transformation;
		std::vector<NodeData> children;
	};
public:
	using Resource::Resource;
	void Load() override;
	void Unload() override;
	void Update(float currentTime, const glm::mat4& parentTransform, const glm::mat4& globalInverse,
		const std::unordered_map<std::string, int>& boneMap,
		const std::vector<BoneInfo>& boneInfo);

	float GetCurrentTime() const { return m_CurrentTime; };
	float GetDuration() const { return m_Duration; };
	float GetTicksPerSecond() const { return m_TicksPerSecond; };
	const NodeData& GetRootNode() const { return m_RootNode; };
	const std::vector<glm::mat4> GetBoneFinalMatrices() const { return m_FinalBoneTransforms; };

	float m_CurrentTime{};

	REFLECTABLE(R_Animation);
private:

	const Bone* FindBone(const std::string& name) const {
		std::unordered_map<std::string, Bone>::const_iterator it = m_Bones.find(name);
		return it != m_Bones.end() ? &it->second : nullptr;
	}
	void CalculateBoneTransform(const NodeData& node, const glm::mat4& parentTransform, const glm::mat4& globalInverse,
		const std::unordered_map<std::string, int>& boneMap,
		const std::vector<BoneInfo>& boneInfo);

	template <typename T> T DecodeBinary(std::string& bin, int& offset);
	NodeData NodeDataParser(std::string& buffer, int& offset);
	float m_Duration{};
	float m_TicksPerSecond{};

	std::string m_Name{};
	std::unordered_map<std::string, Bone> m_Bones{};
	std::vector<glm::mat4> m_FinalBoneTransforms{};
	NodeData m_RootNode;

};