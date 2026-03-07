#pragma once

#include "../Utils/Common.h"
#include <functional>
#include <vector>
#include <string>

struct ImGuiContext;

namespace KojeomEngine {

class KDebugUI {
public:
    static KDebugUI* GetInstance();
    
    KDebugUI() = default;
    ~KDebugUI() { Cleanup(); }
    KDebugUI(const KDebugUI&) = delete;
    KDebugUI& operator=(const KDebugUI&) = delete;
    
    HRESULT Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Cleanup();
    
    void NewFrame();
    void Render(ID3D11DeviceContext* context);
    
    void BeginFrame();
    void EndFrame();
    
    bool HandleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    bool IsInitialized() const { return bInitialized; }
    bool IsVisible() const { return bVisible; }
    void SetVisible(bool visible) { bVisible = visible; }
    void ToggleVisible() { bVisible = !bVisible; }
    
    void RegisterDebugPanel(const std::string& name, std::function<void()> renderCallback);
    void UnregisterDebugPanel(const std::string& name);
    
    struct FFrameStats {
        float FrameTime;
        float FPS;
        UINT DrawCalls;
        UINT TriangleCount;
        UINT VertexCount;
    };
    
    void SetFrameStats(const FFrameStats& stats);
    const FFrameStats& GetFrameStats() const { return FrameStats; }
    
    void SetRenderStats(UINT drawCalls, UINT triangles, UINT vertices);
    
private:
    void RenderMainMenuBar();
    void RenderStatsOverlay();
    void RenderRegisteredPanels();
    
    bool bInitialized = false;
    bool bVisible = true;
    
    FFrameStats FrameStats = {};
    
    struct FDebugPanel {
        std::string Name;
        std::function<void()> RenderCallback;
    };
    std::vector<FDebugPanel> DebugPanels;
    
    ImGuiContext* ImGuiCtx = nullptr;
};

}
