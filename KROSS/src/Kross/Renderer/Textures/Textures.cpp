#include "Kross_pch.h"
#include "Textures.h"
#include "Kross/Renderer/Renderer.h"
#include "GFXAPI/OpenGL/Texture/GLTexture.h"
//#include "GFXAPI/DirectX/Texture.h"

namespace Kross::Texture {
	uint32_t Base::texSlotIndex = 0;

	uint32_t Base::QueryMaxSlots()
	{
		switch (Kross::Renderer::GetAPI())
		{
		case Kross::RendererAPI::API::None:		KROSS_CORE_ERROR("Renderer API (None) is not supported"); return -1;
		case Kross::RendererAPI::API::OpenGL:	return Kross::OpenGL::Texture::Base::QueryMaxSlots();
			//case Kross::RendererAPI::API::DirectX:		return makeRef<Kross::DirectX::Texture::T2D>(path);
		}

		KROSS_CORE_ERROR("Unknown Renderer API");
		return -1;
	}
	uint32_t Base::IncSlot()
	{
		if (texSlotIndex + 1 >= QueryMaxSlots())
		{
			KROSS_CORE_WARN("Textures Slots Overflow.");
			texSlotIndex = 1;
		}
		return texSlotIndex++;
	}
	void Base::ResetIndex()
	{
		texSlotIndex = 1;
	}
	uint32_t Base::GetSlot()
	{
		return texSlotIndex;
	}
	Ref<T2D> T2D::CreateRef(const char* name, const char* path)
	{
		switch (Kross::Renderer::GetAPI())
		{
		case Kross::RendererAPI::API::None:		KROSS_CORE_ERROR("Renderer API (None) is not supported"); return nullptr;
		case Kross::RendererAPI::API::OpenGL:		return makeRef<Kross::OpenGL::Texture::T2D>(name, path);
			//case Kross::RendererAPI::API::DirectX:		return makeRef<Kross::DirectX::Texture::T2D>(path);
		}

		KROSS_CORE_ERROR("Unknown Renderer API");
		return nullptr;
	}
	Ref<T2D> T2D::CreateRef(const char* name, uint32_t width, uint32_t height, uint8_t* data)
	{
		switch (Kross::Renderer::GetAPI())
		{
		case Kross::RendererAPI::API::None:		KROSS_CORE_ERROR("Renderer API (None) is not supported"); return nullptr;
		case Kross::RendererAPI::API::OpenGL:		return makeRef<Kross::OpenGL::Texture::T2D>(name, width, height, data);
			//case Kross::RendererAPI::API::DirectX:		return makeRef<Kross::DirectX::Texture::T2D>(width, height, data);
		}

		KROSS_CORE_ERROR("Unknown Renderer API");
		return nullptr;
	}
	Scope<T2D> T2D::CreateScope(const char* name, const char* path)
	{
		switch (Kross::Renderer::GetAPI())
		{
		case Kross::RendererAPI::API::None:		KROSS_CORE_ERROR("Renderer API (None) is not supported"); return nullptr;
		case Kross::RendererAPI::API::OpenGL:		return makeScope<Kross::OpenGL::Texture::T2D>(name, path);
			//case Kross::RendererAPI::API::DirectX:		return makeScope<Kross::DirectX::Texture::T2D>(path);
		}

		KROSS_CORE_ERROR("Unknown Renderer API");
		return nullptr;
	}
	Scope<T2D> T2D::CreateScope(const char* name, uint32_t width, uint32_t height, uint8_t* data)
	{
		switch (Kross::Renderer::GetAPI())
		{
		case Kross::RendererAPI::API::None:		KROSS_CORE_ERROR("Renderer API (None) is not supported"); return nullptr;
		case Kross::RendererAPI::API::OpenGL:		return makeScope<Kross::OpenGL::Texture::T2D>(name, width, height, data);
			//case Kross::RendererAPI::API::DirectX:		return makeScope<Kross::DirectX::Texture::T2D>(width, height, data);
		}

		KROSS_CORE_ERROR("Unknown Renderer API");
		return nullptr;
	}
}
