#include <Kross_pch.h>
#include "GLErrors.h"

std::vector<glError> glError::cache;
unsigned int glError::count = 0;
bool glError::active = false;

void glDebugMessage(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	char* ctype = "Unknown", * cseverity = "Unknown", * csource = "Unknown", * cid = "Unknown";
	switch (source)
	{
	case GL_DEBUG_SOURCE_API://Calls to the OpenGL API
	{csource = "API"; break; }
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM://Calls to a window - system API
	{csource = "WINDOW_SYSTEM"; break; }
	case GL_DEBUG_SOURCE_SHADER_COMPILER://A compiler for a shading language
	{csource = "SHADER_COMPILER"; break; }
	case GL_DEBUG_SOURCE_THIRD_PARTY://An application associated with OpenGL
	{csource = "THIRD_PARTY"; break; }
	case GL_DEBUG_SOURCE_APPLICATION://Generated by the user of this application
	{csource = "APPLICATION"; break; }
	case GL_DEBUG_SOURCE_OTHER://Some source that isn't one of these
	{csource = "OTHER"; break; }
	}
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR://An error, typically from the API
	{ ctype = "ERROR"; break; }
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR://Some behavior marked deprecated has been used
	{ ctype = "DEPRECATED_BEHAVIOR"; break; }
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR://Something has invoked undefined behavior
	{ ctype = "UNDEFINED_BEHAVIOR"; break; }
	case GL_DEBUG_TYPE_PORTABILITY://Some functionality the user relies upon is not portable
	{ ctype = "PORTABILITY"; break; }
	case GL_DEBUG_TYPE_PERFORMANCE://Code has triggered possible performance issues
	{ ctype = "PERFORMANCE"; break; }
	case GL_DEBUG_TYPE_MARKER://Command stream annotation
	{ ctype = "MARKER"; break; }
	case GL_DEBUG_TYPE_PUSH_GROUP://Group pushing
	{ ctype = "PUSH_GROUP"; break; }
	case GL_DEBUG_TYPE_POP_GROUP://foo
	{ ctype = "POP_GROUP"; break; }
	case GL_DEBUG_TYPE_OTHER://Some type that isn't one of these
	{ ctype = "OTHER"; break; }
	}
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH://All OpenGL Errors, shader compilation/linking errors, or highly-dangerous undefined behavior
	{ cseverity = "HIGH"; break; }
	case GL_DEBUG_SEVERITY_MEDIUM://Major performance warnings, shader compilation/linking warnings, or the use of deprecated functionality
	{ cseverity = "MEDIUM"; break; }
	case GL_DEBUG_SEVERITY_LOW://Redundant state change performance warning, or unimportant undefined behavior
	{ cseverity = "LOW"; break; }
	}
	switch (id)
	{
	case GL_INVALID_ENUM:
	{cid = "INVALID_ENUM"; break; }
	case GL_INVALID_VALUE:
	{cid = "GL_INVALID_VALUE"; break; }
	case GL_INVALID_OPERATION:
	{cid = "GL_INVALID_OPERATION"; break; }
	case GL_INVALID_FRAMEBUFFER_OPERATION:
	{cid = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;	}
	case GL_OUT_OF_MEMORY:
	{cid = "GL_OUT_OF_MEMORY"; break;	}
	case GL_STACK_UNDERFLOW:
	{cid = "GL_STACK_UNDERFLOW"; break;	}
	}

	//KROSS_CORE_ERROR_("	**GL CALLBACK**\nTYPE: {0}\nSEVERITY: {1}\nSOURCE: {2}\nID: {3}\nMESSAGE: {4}\n", ctype, cseverity, csource, cid, message);
	
	KROSS_CORE_ERROR_("	**GL CALLBACK**");
	KROSS_CORE_ERROR_("TYPE: {0}", ctype);
	KROSS_CORE_ERROR_("SEVERITY: {0}", cseverity);
	KROSS_CORE_ERROR_("SOURCE: {0}", csource);
	KROSS_CORE_ERROR_("ID: {0}", cid);
	KROSS_CORE_ERROR_("MESSAGE: {0}", message);
}

glError::glError(const std::string& func_, const std::string& file_, int line_)
	: func(func_),
	file(file_),
	line(line_),
	codes(),
	id(++count)
{
	if (active)
		code = glGetError();
	else
	{
		KROSS_CORE_ERROR_("Trying to use glError without active instance of handler");
	}
}
glError::glError(const std::string& func_, const std::string& file_, int line_, unsigned int code_)
	: func(func_),
	file(file_),
	line(line_),
	codes(),
	code(code_),
	id(++count)
{}
void glError::Begin()
{
	if (cache.empty() && !active && count == 0)
		active = true;
	else
	{
		KROSS_CORE_ERROR_("Trying to initilize OpenGL Error Handler with an already initialized instance.");
	}
}
void glError::flushCache()
{
	if (active)
	{
		cache.clear();
		count = 0;
		active = false;
	}
	else
	{
		KROSS_CORE_ERROR_("Trying to flush OpenGL Error Handler without an valid instance.");
	}
}
bool glError::hasError()
{
	return code != GL_NO_ERROR;
}
bool glError::handle()
{
	if (active)
	{
		while (hasError() && !handled(*this))
		{
			codes.push_back(code);
			display(*this);
		}
	}
	else return false;
	return true;
}
void glError::display(const glError& error)
{
	switch (error.code)
	{
	case GL_INVALID_ENUM: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  GL_INVALID_ENUM\n", error.file, error.line, error.func);
		break;
	}
	case GL_INVALID_VALUE: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  GL_INVALID_VALUE\n", error.file, error.line, error.func);
		break;
	}
	case GL_INVALID_OPERATION: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  GL_INVALID_OPERATION\n", error.file, error.line, error.func);
		break;
	}
	case GL_INVALID_FRAMEBUFFER_OPERATION: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  GL_INVALID_FRAMEBUFFER_OPERATION\n", error.file, error.line, error.func);
		break;
	}
	case GL_OUT_OF_MEMORY: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  GL_OUT_OF_MEMORY\n", error.file, error.line, error.func);
		break;
	}
	case GL_STACK_UNDERFLOW: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  GL_STACK_UNDERFLOW\n", error.file, error.line, error.func);
		break;
	}
	default: {
		KROSS_CORE_ERROR_("ERROR:\nFILE: {0}\nLINE:  {1}\nFUNCTION: {2}\nCODE:  UNKNOWN ERROR\n", error.file, error.line, error.func);
		break;
	}
	}
}
bool glError::handled(const glError& error)
{
	auto i = std::lower_bound(cache.begin(), cache.end(), error,
		[](const glError& other, const glError& k) { return other.id < k.id; });
	if (i < cache.end() && !(i->id < error.id)) return true;
	else cache.emplace(i, error);
	return false;
}
glError::operator unsigned int()
{
	return code;
}
void glError::glGetErrors(const char* function, const char* file, int line)
{
	glError error(function, file, line);
	error.handle();
}
