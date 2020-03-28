#include <Kross_pch.h>
#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shaders.h"
#include "Renderer.h"
#include "Kross/Core/Application.h"
#include "Kross/Core/Stack.h"

#include "Textures/Array.h"
#include "Textures/Atlas.h"

static constexpr size_t MaxQuadCount = 65536 / 2;
static constexpr size_t MaxVertexCount = MaxQuadCount * 4;
static constexpr size_t MaxIndexCount = MaxQuadCount * 6;

namespace Kross {
	bool Renderer2D::SceneBegan = false;
	bool Renderer2D::batch = false;

	struct Vertex
	{
		Vertex() = default;
		Vertex(
			const glm::vec3& pos,
			const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
			const glm::vec2& texCoord = { 0.0f, 0.0f },
			float texIndex = 0.0f)
			:
			pos(pos),
			color(color),
			texCoord(texCoord),
			texIndex(texIndex)
		{}
		glm::vec3 pos;
		glm::vec4 color;
		glm::vec2 texCoord;
		float texIndex;
	};
	struct Quad
	{
		Quad() = default;
		Quad(Vertex bl, Vertex br, Vertex tr, Vertex tl) : bl(bl), br(br), tr(tr), tl(tl) {}
		Quad(const glm::vec3& pos, const glm::vec4& color, const glm::vec2& size, const float texture)
			:
			bl(pos, color, { 0.0f, 0.0f }, texture),
			br({ pos.x + size.x	,	pos.y,			pos.z }, color, { 1.0f, 0.0f }, texture),
			tr({ pos.x + size.x ,	pos.y + size.y,	pos.z }, color, { 1.0f, 1.0f }, texture),
			tl({ pos.x			,	pos.y + size.y,	pos.z }, color, { 0.0f, 1.0f }, texture)
		{}
		Quad(
			const glm::vec2& pos,
			const glm::vec4& color,
			const glm::vec2& size,
			const glm::vec2& texOff,
			const glm::vec2& texSize,
			const glm::vec2& flip,
			const float texture)
			:
			bl({ pos.x,			pos.y,			0.0f }, color, { texOff.x				, texOff.y }, texture),
			br({ pos.x + size.x,pos.y,			0.0f }, color, { texOff.x + texSize.x	, texOff.y }, texture),
			tr({ pos.x + size.x,pos.y + size.y,	0.0f }, color, { texOff.x + texSize.x	, texOff.y + texSize.y }, texture),
			tl({ pos.x,			pos.y + size.y,	0.0f }, color, { texOff.x				, texOff.y + texSize.y }, texture)
		{}
		Quad(const QuadParams& p, const float texture)
			:
			bl({ p.position.x,				p.position.y,				0.0f }, p.color, { p.texOffSet.x					, p.texOffSet.y }, texture),
			br({ p.position.x + p.size.x,	p.position.y,				0.0f }, p.color, { p.texOffSet.x + p.texSubSize.x	, p.texOffSet.y }, texture),
			tr({ p.position.x + p.size.x,	p.position.y + p.size.y,	0.0f }, p.color, { p.texOffSet.x + p.texSubSize.x	, p.texOffSet.y + p.texSubSize.y }, texture),
			tl({ p.position.x,				p.position.y + p.size.y,	0.0f }, p.color, { p.texOffSet.x					, p.texOffSet.y + p.texSubSize.y }, texture)
		{}
		Vertex bl;
		Vertex br;
		Vertex tr;
		Vertex tl;
	};

	struct R2DData {
		// Basic 2D Shader
		Ref<Shader> shader;

		// 1x1 White Texture
		Ref<Texture::T2D> whiteTex;

		//Batch data
		Scope<VertexArray> va;
		Ref<Buffer::Vertex> vb;
		Ref<Buffer::Index> ib;

		//Batch Buffer
		//Quad myBuffer[MaxQuadCount] = { Quad() };
		//Pointer to keep track of the Batch Buffer's fist Quad
		Quad* BufferFirst = nullptr;
		//Current Reading Head of the Batch Buffer
		Quad* BufferHead = nullptr;

		//Batch Quad Counter
		unsigned int quadIndex = 0;

		//Non Batch data
		Scope<VertexArray> NBva;
		Ref<Buffer::Vertex> NBsqrVB;
		Ref<Buffer::Index> NBsqrIB;

		//Texture Array
		Scope<Texture::T2DArray> texArray;

		//Renderer Status for ImGui window
		Renderer2D::Stats rendererStats = Renderer2D::Stats();

		//Cache for Texture (Performance Optimization)
		std::vector<Ref<Texture::T2D>> texCache;

		// FrameBuffer
		Ref<Texture::FrameBuffer> framebuffer;

		Ref<VertexArray> quad_VertexArrayID;
		Ref<Buffer::Vertex> quad_vertexbuffer;
		Ref<Buffer::Index> quad_indexbuffer;
		Ref<Shader> quad_programID;
	};
	// Renderer2D data
	static R2DData* data;

	const Renderer2D::Stats& Renderer2D::getStats()
	{
		return data->rendererStats;
	}
	void Renderer2D::ResetStats()
	{
		data->rendererStats.maxQuads = MaxQuadCount;
		data->rendererStats.DrawCount = 0;
		data->rendererStats.QuadCount = 0;
	}

	void Renderer2D::Init()
	{
		KROSS_PROFILE_FUNC();
		static bool called = false;
		if (!called)
		{
			data = new R2DData;
			data->texCache.resize(Texture::Base::QueryMaxSlots());
			data->texArray = Texture::T2DArray::CreateScope(Texture::Base::QueryMaxSlots());

			float sqrVertices[] = {
				-0.5f, -0.5f, 0.0f,		1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 0.0f,		1.0f,
				 0.5f, -0.5f, 0.0f,		1.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f,		1.0f,
				 0.5f,  0.5f, 0.0f,		1.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,		1.0f,
				-0.5f,  0.5f, 0.0f,		1.0f, 1.0f, 1.0f, 1.0f,		0.0f, 1.0f,		1.0f
			};
			uint32_t sqrIndices[] = {
				0, 1, 2, 2, 3, 0
			};

			data->NBva = VertexArray::CreateScope();
			data->NBsqrVB = Buffer::Vertex::Create(sqrVertices, sizeof(sqrVertices), false);
			data->NBsqrVB->SetLayout({
				{ Buffer::ShaderDataType::Float3, "a_Position" },
				{ Buffer::ShaderDataType::Float4, "a_Color"},
				{ Buffer::ShaderDataType::Float2, "a_TexCoord" },
				{ Buffer::ShaderDataType::Float,  "a_TexIndex"}
				});
			data->NBva->AddVertex(data->NBsqrVB);

			data->NBsqrIB = Buffer::Index::Create(sqrIndices, sizeof(sqrIndices) / sizeof(uint32_t));
			data->NBva->SetIndex(data->NBsqrIB);




			float g_quad_vertex_buffer_data[] = {
				//-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
				// 1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
				// 1.0f,  1.0f, 0.0f,		1.0f, 1.0f,
				//-1.0f,  1.0f, 0.0f,		0.0f, 1.0f


				//-1.0f, -1.0f, 0.0f,
				// 1.0f, -1.0f, 0.0f,
				// 1.0f,  1.0f, 0.0f,
				//-1.0f,  1.0f, 0.0f,

				-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
				 1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
				-1.0f,  1.0f, 0.0f,		0.0f, 1.0f,
				-1.0f,  1.0f, 0.0f,		0.0f, 1.0f,
				 1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
				 1.0f,  1.0f, 0.0f,		1.0f, 1.0f
			};
			uint32_t g_quad_index_buffer_data[] = {
				0, 1, 2, 2, 3, 0
			};

			data->quad_VertexArrayID = VertexArray::CreateRef();
			data->quad_vertexbuffer = Buffer::Vertex::Create(g_quad_vertex_buffer_data, sizeof(g_quad_vertex_buffer_data));
			data->quad_vertexbuffer->SetLayout({
				{ Buffer::ShaderDataType::Float3, "a_Position" },
				{ Buffer::ShaderDataType::Float2, "a_TexCoord" }
				});
			data->quad_VertexArrayID->AddVertex(data->quad_vertexbuffer);
			data->quad_indexbuffer = Buffer::Index::Create(g_quad_index_buffer_data, sizeof(g_quad_index_buffer_data) / sizeof(uint32_t));
			data->quad_VertexArrayID->SetIndex(data->quad_indexbuffer);
			Stack<Shader>::get().Add(data->quad_programID = Shader::CreateRef("assets/shaders/OpenGL/framebuffer"));




			data->va = VertexArray::CreateScope();

			data->BufferFirst = new Quad[MaxQuadCount];
			data->BufferHead = data->BufferFirst;

			data->vb = Buffer::Vertex::Create(nullptr, sizeof(Quad) * MaxQuadCount, true);
			data->vb->SetLayout({
				{ Buffer::ShaderDataType::Float3, "a_Position"	},
				{ Buffer::ShaderDataType::Float4, "a_Color"		},
				{ Buffer::ShaderDataType::Float2, "a_TexCoord"	},
				{ Buffer::ShaderDataType::Float,  "a_TexIndex"	}
				});
			data->va->AddVertex(data->vb);

			uint32_t* indices = new uint32_t[MaxIndexCount];
			uint32_t offset = 0;
			for (int i = 0; i < MaxIndexCount; i += 6)
			{
				indices[i + 0] = 0 + offset;
				indices[i + 1] = 1 + offset;
				indices[i + 2] = 2 + offset;

				indices[i + 3] = 2 + offset;
				indices[i + 4] = 3 + offset;
				indices[i + 5] = 0 + offset;

				offset += 4;
			}

			data->ib = Buffer::Index::Create(indices, MaxIndexCount);
			data->va->SetIndex(data->ib);
			delete[] indices;

			Stack<Shader>::get().Add(data->shader = Shader::CreateRef("assets/shaders/OpenGL/Shader2D"));
			data->shader->SetFloat("u_Repeat", 1);
			data->shader->SetFloat4("u_Color", glm::vec4(1));

			data->shader->SetIntV("u_Textures", Texture::Base::QueryMaxSlots(), nullptr);

			uint32_t white = 0xffffffff;
			data->texArray->Add(Stack<Texture::T2D>::get().Add(data->whiteTex = Texture::T2D::CreateRef(1, 1, "blank", &white)));

			data->framebuffer = Texture::FrameBuffer::Create("Renderer2D");

			SceneBegan = false;
			called = true;
		}
		else
		{
			KROSS_MSGBOX("Renderer2D is already initialized. Cannot call Renderer2D::Init(void) twice. Forget to call Renderer2D::Shutdown(void)?", __FUNCTION__, _ERROR_);
		}
	}
	void Renderer2D::Shutdown()
	{
		KROSS_PROFILE_FUNC();
		static bool called = false;
		if (!called)
		{
			Stack<Texture::T2D>::get().clear();
			Stack<Shader>::get().clear();
			delete[] data->BufferFirst;
			delete data;
		}
		else
		{
			KROSS_MSGBOX("Renderer2D is not initialized. Cannot call Renderer2D::Shutdown(void) while Renderer2D::Init(void) is not called.", __FUNCTION__, _ERROR_);
		}
	}
	void Renderer2D::Begin(Camera::Camera& camera)
	{
		KROSS_PROFILE_FUNC();
		if (!SceneBegan)
			SceneBegan = true;
		else
			KROSS_CORE_WARN("Calling {0} without calling Renderer2D::End(void). Overriding previous scene!", __FUNCTION__);
		data->shader->Bind();
		data->shader->SetMat4("u_ViewProjection", camera.GetVPM());
		data->shader->SetMat4("u_Transform", glm::mat4(1.0f));
		BatchBegin();
		data->framebuffer->Bind(800,400);
	}
	void Renderer2D::BatchBegin()
	{
		if (batch == false)
		{
			data->quadIndex = 0;
			data->BufferHead = data->BufferFirst;
			batch = true;
		}
		else
		{
			KROSS_CORE_WARN("Batch already initiated.");
		}
	}
	void Renderer2D::Flush()
	{
		data->texArray->Bind(0);
		data->va->Bind();
		Renderer::Command::DrawIndexed(data->va, data->quadIndex * 6);
		data->rendererStats.DrawCount++;

		data->quadIndex = 0;
		Texture::Base::ResetIndex();
	}
	void Renderer2D::BatchEnd()
	{
		if (batch)
		{
			//data->vb->upload(data->myBuffer, sizeof(Quad) * data->quadIndex);
			data->vb->upload(data->BufferFirst, (char*)data->BufferHead - (char*)data->BufferFirst);
			Flush();
			batch = false;
		}
		else
		{
			KROSS_CORE_WARN("Ending a non initiated batch");
		}
	}
	void Renderer2D::End()
	{
		if (SceneBegan)
		{
			BatchEnd();
			SceneBegan = false;

			data->framebuffer->unBind();
			Renderer::Command::SetClear({ 0.2f, 0.2f, 0.2f, 1.0f });
			Renderer::Command::Clear();
			//data->quad_programID->Bind();
			//data->whiteTex->Bind();
			//Stack<Texture::T2D>::get().Get("checker", "assets/textures/cage_mamma.png")->Bind();
			//data->framebuffer->GetKrossTexture()->Bind();
			//Renderer::Command::DrawIndexed(data->quad_VertexArrayID);
		}
		else
			KROSS_CORE_WARN("Calling {0} without calling Renderer2D::Begin(Camera::Camera&). Did you forget to Begin the Scene?", __FUNCTION__);
	}

	const Ref<Texture::FrameBuffer> Renderer2D::GetFrameBuffer()
	{
		if (!data)
		{
			KROSS_CORE_ERROR("[ {0} ] |||| Renderer has not yet been initialized.", __FUNCTION__);
			return nullptr;
		}
		return data->framebuffer;
	}

	//void Renderer2D::DrawQuad(const glm::vec2& position, float size, const glm::vec4& color)
	//{
	//	DrawQuad(position, { size, size }, color);
	//}
	//void Renderer2D::DrawQuad(const glm::vec3& position, float size, const glm::vec4& color)
	//{
	//	DrawQuad(position, { size, size }, color);
	//}
	//void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	//{
	//	DrawQuad({ position.x, position.y, 0.0f }, size, color);
	//}
	//void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	//{
	//	KROSS_PROFILE_FUNC();
	//	glm::mat4 transform;
	//	{
	//		KROSS_PROFILE_SCOPE("transform");
	//		transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	//	}
	//	data->shader->Bind();
	//	data->shader->SetMat4("u_Transform", transform);
	//	data->shader->SetFloat4("u_Color", color);
	//	data->whiteTex->Bind();
	//	data->NBva->Bind();
	//	Renderer::Command::DrawIndexed(data->NBva);
	//}
	////
	//void Renderer2D::DrawQuad(const glm::vec2& position, float size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	DrawQuad(position, { size, size }, texture, color, repeat);
	//}
	//void Renderer2D::DrawQuad(const glm::vec3& position, float size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	DrawQuad(position, { size, size }, texture, color, repeat);
	//}
	//void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	DrawQuad({ position.x, position.y, 0.0f }, size, texture, color, repeat);
	//}
	//void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	KROSS_PROFILE_FUNC();
	//	glm::mat4 transform;
	//	{
	//		KROSS_PROFILE_SCOPE("transform");
	//		transform =
	//			glm::translate(glm::mat4(1.0f), position)
	//			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	//	}
	//	data->shader->Bind();
	//	data->shader->SetMat4("u_Transform", transform);
	//	data->shader->SetFloat4("u_Color", color);
	//	data->shader->SetFloat("u_Repeat", repeat);
	//
	//	KROSS_R_MSGBOX(texture,
	//		texture->Bind();
	//	, "Trying to Draw with a NULL texture", _WARN_);
	//
	//	data->NBva->Bind();
	//	Renderer::Command::DrawIndexed(data->NBva);
	//}
	////
	////
	//void Renderer2D::DrawRotatedQuad(const glm::vec2& position, float size, float rotation, const glm::vec4& color)
	//{
	//	DrawRotatedQuad(position, { size, size }, rotation, color);
	//}
	//void Renderer2D::DrawRotatedQuad(const glm::vec3& position, float size, float rotation, const glm::vec4& color)
	//{
	//	DrawRotatedQuad(position, { size, size }, rotation, color);
	//}
	//void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	//{
	//	DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	//}
	//void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	//{
	//	KROSS_PROFILE_FUNC();
	//	glm::mat4 transform;
	//	{
	//		KROSS_PROFILE_SCOPE("transform");
	//		transform =
	//			glm::translate(glm::mat4(1.0f), position)
	//			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
	//			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	//	}
	//	data->shader->Bind();
	//	data->shader->SetMat4("u_Transform", transform);
	//	data->shader->SetFloat4("u_Color", color);
	//	data->whiteTex->Bind();
	//	data->NBva->Bind();
	//	Renderer::Command::DrawIndexed(data->NBva);
	//}
	////
	//void Renderer2D::DrawRotatedQuad(const glm::vec2& position, float size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	DrawRotatedQuad(position, { size, size }, texture, rotation, color, repeat);
	//}
	//void Renderer2D::DrawRotatedQuad(const glm::vec3& position, float size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	DrawRotatedQuad(position, { size, size }, texture, rotation, color, repeat);
	//}
	//void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	DrawRotatedQuad({ position.x, position.y, 0.0f }, size, texture, rotation, color, repeat);
	//}
	//void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	KROSS_PROFILE_FUNC();
	//	glm::mat4 transform;
	//	{
	//		KROSS_PROFILE_SCOPE("transform");
	//		transform =
	//			glm::translate(glm::mat4(1.0f), position)
	//			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
	//			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	//	}
	//	data->shader->Bind();
	//	data->shader->SetMat4("u_Transform", transform);
	//	data->shader->SetFloat4("u_Color", color);
	//	data->shader->SetFloat("u_Repeat", repeat);
	//
	//	texture->Bind();
	//
	//	data->NBva->Bind();
	//	Renderer::Command::DrawIndexed(data->NBva);
	//}
	////
	////
	////
	//void Renderer2D::BatchQuad(const glm::vec2& position, float size, const glm::vec4& color)
	//{
	//	BatchQuad(position, { size, size }, color);
	//}
	//void Renderer2D::BatchQuad(const glm::vec3& position, float size, const glm::vec4& color)
	//{
	//	BatchQuad(position, { size, size }, color);
	//}
	//void Renderer2D::BatchQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	//{
	//	BatchQuad({ position.x, position.y, 0.0f }, size, color);
	//}
	//void Renderer2D::BatchQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	//{
	//	KROSS_PROFILE_FUNC();
	//	if (++data->quadIndex >= MaxQuadCount)
	//	{
	//		BatchEnd();
	//		BatchBegin();
	//	}
	//	data->myBuffer[data->quadIndex] = Quad(position, color, size, 0.0f);
	//	data->rendererStats.QuadCount++;
	//}
	////
	//void Renderer2D::BatchQuad(const glm::vec2& position, float size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	BatchQuad(position, { size, size }, texture, color, repeat);
	//}
	//void Renderer2D::BatchQuad(const glm::vec3& position, float size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	BatchQuad(position, { size, size }, texture, color, repeat);
	//}
	//void Renderer2D::BatchQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	BatchQuad({ position.x, position.y, 0.0f }, size, texture, color, repeat);
	//}
	//void Renderer2D::BatchQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, const glm::vec4& color, float repeat)
	//{
	//	KROSS_PROFILE_FUNC();
	//	if (++data->quadIndex >= MaxQuadCount)
	//	{
	//		BatchEnd();
	//		BatchBegin();
	//	}
	//	float texIndex = 0.0f;
	//	//for (auto iter = data->texCache.begin() + 1; iter != data->texCache.end(); iter++)
	//	//{
	//	//	if (*iter)
	//	//	{
	//	//		if ((*iter)->GetID() == texture->GetID())
	//	//		{
	//	//			texIndex = (float)std::distance(data->texCache.begin(), iter);
	//	//			break;
	//	//		}
	//	//	}
	//	//}
	//	//if (texIndex == 0.0f)
	//	//{
	//	//	int index = 0;
	//	//	for (auto iter = data->texCache.begin() + 1; iter != data->texCache.end(); iter++)
	//	//	{
	//	//		index++;
	//	//		if (!(*iter))
	//	//		{
	//	//			*iter = texture;
	//	//			break;
	//	//		}
	//	//	}
	//	//	if (index == (int)data->texCache.size())
	//	//	{
	//	//		KROSS_CORE_WARN("Texture Cache at maximum capacity. New Textures discarted");
	//	//	}
	//	//}
	//	////data->myBuffer[data->quadIndex] = Quad(position, color, size, texIndex);
	//	data->myBuffer[data->quadIndex] = {
	//		Vertex({position.x			, position.y			, position.z}, color, {0.0f, 0.0f}, texIndex),
	//		Vertex({position.x + size.x	, position.y			, position.z}, color, {0.5f, 0.0f}, texIndex),
	//		Vertex({position.x + size.x	, position.y + size.y	, position.z}, color, {0.5f, 0.5f}, texIndex),
	//		Vertex({position.x			, position.y + size.y	, position.z}, color, {0.0f, 0.5f}, texIndex)
	//	};
	//	data->rendererStats.QuadCount++;
	//}

	void Renderer2D::BatchQuad(const QuadParams& params)
	{
		KROSS_PROFILE_FUNC();
		if (data->quadIndex >= MaxQuadCount)
		{
			BatchEnd();
			BatchBegin();
		}

		//data->BufferHead = &Quad(params, (float)data->texArray->Get(params.texture));
		float texture = (float)data->texArray->Get(params.texture);
		data->BufferHead->bl = { { params.position.x,				params.position.y,					0.0f }, params.color, { params.texOffSet.x							, params.texOffSet.y }, texture };
		data->BufferHead->br = { { params.position.x + params.size.x,params.position.y,					0.0f }, params.color, { params.texOffSet.x + params.texSubSize.x	, params.texOffSet.y }, texture };
		data->BufferHead->tr = { { params.position.x + params.size.x,params.position.y + params.size.y,	0.0f }, params.color, { params.texOffSet.x + params.texSubSize.x	, params.texOffSet.y + params.texSubSize.y }, texture };
		data->BufferHead->tl = { { params.position.x,				params.position.y + params.size.y,	0.0f }, params.color, { params.texOffSet.x							, params.texOffSet.y + params.texSubSize.y }, texture };
		data->BufferHead++;
		//data->myBuffer[data->quadIndex] = Quad(params, (float)data->texArray->Get(params.texture));

		data->quadIndex++;
		data->rendererStats.QuadCount++;
	}

	//void Renderer2D::BatchRotatedQuad(const glm::vec2& position, float size, float rotation, const glm::vec4& color)
	//{
	//	BatchRotatedQuad(position, { size, size }, rotation, color);
	//}
	//void Renderer2D::BatchRotatedQuad(const glm::vec3& position, float size, float rotation, const glm::vec4& color)
	//{
	//	BatchRotatedQuad(position, { size, size }, rotation, color);
	//}
	//void Renderer2D::BatchRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	//{
	//	BatchRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	//}
	//void Renderer2D::BatchRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	//{
	//	KROSS_PROFILE_FUNC();
	//	glm::mat4 transform;
	//	{
	//		KROSS_PROFILE_SCOPE("transform");
	//		transform =
	//			glm::translate(glm::mat4(1.0f), position)
	//			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
	//			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	//	}
	//	data->shader->Bind();
	//	data->shader->SetMat4("u_Transform", transform);
	//	data->shader->SetFloat4("u_Color", color);
	//	data->whiteTex->Bind();
	//	data->NBva->Bind();
	//	Renderer::Command::DrawIndexed(data->NBva);
	//}
	////
	//void Renderer2D::BatchRotatedQuad(const glm::vec2& position, float size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	BatchRotatedQuad(position, { size, size }, texture, rotation, color, repeat);
	//}
	//void Renderer2D::BatchRotatedQuad(const glm::vec3& position, float size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	BatchRotatedQuad(position, { size, size }, texture, rotation, color, repeat);
	//}
	//void Renderer2D::BatchRotatedQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	BatchRotatedQuad({ position.x, position.y, 0.0f }, size, texture, rotation, color, repeat);
	//}
	//void Renderer2D::BatchRotatedQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture::T2D>& texture, float rotation, const glm::vec4& color, float repeat)
	//{
	//	KROSS_PROFILE_FUNC();
	//	glm::mat4 transform;
	//	{
	//		KROSS_PROFILE_SCOPE("transform");
	//		transform =
	//			glm::translate(glm::mat4(1.0f), position)
	//			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
	//			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	//	}
	//	data->shader->Bind();
	//	data->shader->SetMat4("u_Transform", transform);
	//	data->shader->SetFloat4("u_Color", color);
	//	data->shader->SetFloat("u_Repeat", repeat);
	//
	//	texture->Bind();
	//
	//	data->NBva->Bind();
	//	Renderer::Command::DrawIndexed(data->NBva);
	//}

	void QuadParams::FlipX()
	{
		texOffSet.x += texSubSize.x;
		texSubSize.x *= -1;
	}

	void QuadParams::FlipY()
	{
		texOffSet.y += texSubSize.y;
		texSubSize.y *= -1;
	}

	void QuadParams::Reset()
	{
		color = glm::vec4(1);
		size = glm::vec2(1);
		position = glm::vec2(0);
		texture = NULL;
		repeat = 1;
		texOffSet = position;
		texSubSize = size;
	}
}

