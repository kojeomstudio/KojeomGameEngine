#pragma once

#include "../Utils/Common.h"
#include "RenderModule.h"
#include "Shadow/ShadowRenderer.h"
#include "Shadow/CascadedShadowRenderer.h"
#include "Deferred/DeferredRenderer.h"
#include "PostProcess/PostProcessor.h"
#include "SSAO/SSAO.h"
#include "SSR/SSR.h"
#include "TAA/TAA.h"
#include "Volumetric/VolumetricFog.h"
#include "SSGI/SSGI.h"
#include "PostProcess/MotionBlur.h"
#include "PostProcess/DepthOfField.h"
#include "PostProcess/LensEffects.h"
#include "Sky/SkySystem.h"
#include "IBL/IBLSystem.h"

class KShadowRenderModule : public IRenderModule
{
public:
    KShadowRenderModule(KShadowRenderer& InShadowRenderer, KCascadedShadowRenderer& InCascadedShadow)
        : ShadowRenderer(InShadowRenderer), CascadedShadow(InCascadedShadow) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override {}

    const std::string& GetName() const override { static const std::string name = "Shadow"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::Shadow; }
    bool IsInitialized() const override { return ShadowRenderer.IsInitialized(); }

    KShadowRenderer& GetShadowRenderer() { return ShadowRenderer; }
    KCascadedShadowRenderer& GetCascadedShadow() { return CascadedShadow; }

private:
    KShadowRenderer& ShadowRenderer;
    KCascadedShadowRenderer& CascadedShadow;
};

class KDeferredRenderModule : public IRenderModule
{
public:
    KDeferredRenderModule(KDeferredRenderer& InDeferredRenderer)
        : DeferredRenderer(InDeferredRenderer) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (DeferredRenderer.IsInitialized())
        {
            DeferredRenderer.Resize(Device, NewWidth, NewHeight);
        }
    }

    const std::string& GetName() const override { static const std::string name = "Deferred"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::Geometry; }
    bool IsInitialized() const override { return DeferredRenderer.IsInitialized(); }

    KDeferredRenderer& GetDeferredRenderer() { return DeferredRenderer; }

private:
    KDeferredRenderer& DeferredRenderer;
};

class KPostProcessRenderModule : public IRenderModule
{
public:
    KPostProcessRenderModule(KPostProcessor& InPostProcessor, KMotionBlur& InMotionBlur,
                             KDepthOfField& InDepthOfField, KLensEffects& InLensEffects)
        : PostProcessor(InPostProcessor), MotionBlur(InMotionBlur),
          DepthOfField(InDepthOfField), LensEffects(InLensEffects) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (PostProcessor.IsInitialized()) PostProcessor.Resize(Device, NewWidth, NewHeight);
        if (MotionBlur.IsInitialized()) MotionBlur.Resize(Device, NewWidth, NewHeight);
        if (DepthOfField.IsInitialized()) DepthOfField.Resize(Device, NewWidth, NewHeight);
        if (LensEffects.IsInitialized()) LensEffects.Resize(Device, NewWidth, NewHeight);
    }

    const std::string& GetName() const override { static const std::string name = "PostProcess"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
    bool IsInitialized() const override { return PostProcessor.IsInitialized(); }

    KPostProcessor& GetPostProcessor() { return PostProcessor; }
    KMotionBlur& GetMotionBlur() { return MotionBlur; }
    KDepthOfField& GetDepthOfField() { return DepthOfField; }
    KLensEffects& GetLensEffects() { return LensEffects; }

private:
    KPostProcessor& PostProcessor;
    KMotionBlur& MotionBlur;
    KDepthOfField& DepthOfField;
    KLensEffects& LensEffects;
};

class KSSAORenderModule : public IRenderModule
{
public:
    KSSAORenderModule(KSSAO& InSSAO) : SSAO(InSSAO) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (SSAO.IsInitialized()) SSAO.Resize(Device, NewWidth, NewHeight);
    }

    const std::string& GetName() const override { static const std::string name = "SSAO"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
    bool IsInitialized() const override { return SSAO.IsInitialized(); }

    KSSAO& GetSSAO() { return SSAO; }

private:
    KSSAO& SSAO;
};

class KSSRRenderModule : public IRenderModule
{
public:
    KSSRRenderModule(KSSR& InSSR) : SSR(InSSR) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (SSR.IsInitialized()) SSR.Resize(Device, NewWidth, NewHeight);
    }

    const std::string& GetName() const override { static const std::string name = "SSR"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
    bool IsInitialized() const override { return SSR.IsInitialized(); }

    KSSR& GetSSR() { return SSR; }

private:
    KSSR& SSR;
};

class KTAARenderModule : public IRenderModule
{
public:
    KTAARenderModule(KTAA& InTAA) : TAA(InTAA) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (TAA.IsInitialized()) TAA.Resize(Device, NewWidth, NewHeight);
    }

    const std::string& GetName() const override { static const std::string name = "TAA"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
    bool IsInitialized() const override { return TAA.IsInitialized(); }

    KTAA& GetTAA() { return TAA; }

private:
    KTAA& TAA;
};

class KSSGIRenderModule : public IRenderModule
{
public:
    KSSGIRenderModule(KSSGI& InSSGI) : SSGI(InSSGI) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (SSGI.IsInitialized()) SSGI.Resize(Device, NewWidth, NewHeight);
    }

    const std::string& GetName() const override { static const std::string name = "SSGI"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
    bool IsInitialized() const override { return SSGI.IsInitialized(); }

    KSSGI& GetSSGI() { return SSGI; }

private:
    KSSGI& SSGI;
};

class KVolumetricFogRenderModule : public IRenderModule
{
public:
    KVolumetricFogRenderModule(KVolumetricFog& InVolumetricFog) : VolumetricFog(InVolumetricFog) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override
    {
        if (VolumetricFog.IsInitialized()) VolumetricFog.Resize(Device, NewWidth, NewHeight);
    }

    const std::string& GetName() const override { static const std::string name = "VolumetricFog"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::PostProcess; }
    bool IsInitialized() const override { return VolumetricFog.IsInitialized(); }

    KVolumetricFog& GetVolumetricFog() { return VolumetricFog; }

private:
    KVolumetricFog& VolumetricFog;
};

class KSkyRenderModule : public IRenderModule
{
public:
    KSkyRenderModule(KSkySystem& InSkySystem) : SkySystem(InSkySystem) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override {}

    const std::string& GetName() const override { static const std::string name = "Sky"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::Overlay; }
    bool IsInitialized() const override { return SkySystem.IsInitialized(); }

    KSkySystem& GetSkySystem() { return SkySystem; }

private:
    KSkySystem& SkySystem;
};

class KIBLRenderModule : public IRenderModule
{
public:
    KIBLRenderModule(KIBLSystem& InIBLSystem) : IBLSystem(InIBLSystem) {}

    HRESULT Initialize(KGraphicsDevice* InDevice) override { return S_OK; }
    void Shutdown() override {}

    void OnResize(ID3D11Device* Device, UINT32 NewWidth, UINT32 NewHeight) override {}

    const std::string& GetName() const override { static const std::string name = "IBL"; return name; }
    ERenderModulePhase GetPhase() const override { return ERenderModulePhase::Lighting; }
    bool IsInitialized() const override { return IBLSystem.IsInitialized(); }

    KIBLSystem& GetIBLSystem() { return IBLSystem; }

private:
    KIBLSystem& IBLSystem;
};
