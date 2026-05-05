#include "App/Engine.h"
#include "App/AppMode.h"
#include "Renderer/OpenGL/OpenGLRenderer.h"

namespace Kojeom
{
std::unique_ptr<IRendererBackend> CreateOpenGLBackend()
{
    return std::make_unique<OpenGLRenderer>();
}
}
