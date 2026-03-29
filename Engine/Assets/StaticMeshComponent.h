#pragma once

#include "../Scene/Actor.h"
#include "../Assets/StaticMesh.h"
#include "../Graphics/Shader.h"
#include <memory>

class KStaticMeshComponent : public KActorComponent
{
public:
    KStaticMeshComponent() = default;
    virtual ~KStaticMeshComponent() = default;

    virtual void Render(class KRenderer* Renderer) override;

    void SetStaticMesh(std::shared_ptr<KStaticMesh> InMesh);
    std::shared_ptr<KStaticMesh> GetStaticMesh() const { return StaticMesh; }

    void SetShader(std::shared_ptr<KShaderProgram> InShader) { Shader = InShader; }
    std::shared_ptr<KShaderProgram> GetShader() const { return Shader; }

    void SetCastShadow(bool bCast) { bCastShadow = bCast; }
    bool GetCastShadow() const { return bCastShadow; }

    virtual EComponentType GetComponentTypeID() const override { return EComponentType::StaticMesh; }

    virtual void Serialize(KBinaryArchive& Archive) override;
    virtual void Deserialize(KBinaryArchive& Archive) override;

private:
    std::shared_ptr<KStaticMesh> StaticMesh;
    std::shared_ptr<KShaderProgram> Shader;
    bool bCastShadow = true;
};
