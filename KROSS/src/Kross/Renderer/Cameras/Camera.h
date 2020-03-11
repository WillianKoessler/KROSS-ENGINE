#pragma once

#include "glm/glm/gtc/matrix_transform.hpp"
#include "Kross/Renderer/RendererAPI.h"

namespace Kross::Camera {

#define CAMERA_TYPE(type)\
	static Type GetStaticType() { return Type::##type; }\
	virtual Type GetType() const override { return GetStaticType(); }\
	virtual const std::string GetCameraName() const override { return (const std::string)#type; }

	class Camera
	{
	public:
		virtual void SetPosition(const glm::vec3& pos) = 0;
		virtual void SetRotation(float angle) = 0;
		virtual void SetProjMat(float left, float right, float bottom, float top) = 0;

		virtual const glm::mat4& GetProjMat()	const = 0;
		virtual const glm::mat4& GetViewMat()	const = 0;
		virtual const glm::mat4& GetVPM()		const = 0;
		virtual const glm::vec3& GetPosition()	const = 0;
		virtual const float GetRotation()		const = 0;

		enum class Type
		{
			None = 0, Orthographic_2D
		};
		
		virtual Type GetType() const = 0;
		virtual const std::string GetCameraName() const = 0;
		virtual const std::string& GetName() const = 0;

		inline static Camera* GetSelf() { return self; }
		inline static void SetSelf(Camera* other) { self = other; }

	private:
		static Camera* self;
	};

}