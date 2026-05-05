#pragma once

#include <glad/gl.h>
#include "Core/Log.h"

namespace Kojeom
{
class OpenGLFramebuffer
{
public:
    OpenGLFramebuffer() = default;
    ~OpenGLFramebuffer() { Destroy(); }

    void CreateSceneFBO(int width, int height,
        GLuint& outColorTex, GLuint& outDepthTex)
    {
        Destroy();

        if (width <= 0 || height <= 0) return;

        glGenFramebuffers(1, &m_fbo);
        glGenTextures(1, &outColorTex);
        glGenTextures(1, &outDepthTex);

        glBindTexture(GL_TEXTURE_2D, outColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, outDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outColorTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, outDepthTex, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            KE_LOG_ERROR("Framebuffer incomplete: {}", static_cast<int>(status));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        KE_LOG_INFO("Created scene framebuffer {}x{}", width, height);
    }

    GLuint CreateShadowMapFBO(int width, int height, GLuint shadowTexture)
    {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return fbo;
    }

    GLuint CreateColorFBO(int width, int height, GLuint colorTexture)
    {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return fbo;
    }

    static void DeleteFBO(GLuint& fbo)
    {
        if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
    }

    static void DeleteTexture(GLuint& tex)
    {
        if (tex) { glDeleteTextures(1, &tex); tex = 0; }
    }

    void Destroy()
    {
        if (m_fbo) { glDeleteFramebuffers(1, &m_fbo); m_fbo = 0; }
    }

    GLuint GetFBO() const { return m_fbo; }
    void Release() { m_fbo = 0; }

private:
    GLuint m_fbo = 0;
};
}
