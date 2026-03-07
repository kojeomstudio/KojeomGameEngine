#include "DebugUI.h"
#include "../Utils/Logger.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#endif

namespace KojeomEngine {

KDebugUI* KDebugUI::GetInstance() {
    static KDebugUI instance;
    return &instance;
}

HRESULT KDebugUI::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (bInitialized) {
        return S_OK;
    }
    
    if (!device || !context) {
        LOG_ERROR("Invalid device or context for DebugUI initialization");
        return E_INVALIDARG;
    }
    
#ifdef USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGuiCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ImGuiCtx));
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    
    if (!ImGui_ImplWin32_Init(GetActiveWindow())) {
        LOG_ERROR("Failed to initialize ImGui Win32 backend");
        return E_FAIL;
    }
    
    if (!ImGui_ImplDX11_Init(device, context)) {
        LOG_ERROR("Failed to initialize ImGui DX11 backend");
        return E_FAIL;
    }
#endif
    
    bInitialized = true;
    LOG_INFO("DebugUI initialized successfully");
    return S_OK;
}

void KDebugUI::Cleanup() {
    if (!bInitialized) {
        return;
    }
    
#ifdef USE_IMGUI
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(static_cast<ImGuiContext*>(ImGuiCtx));
    ImGuiCtx = nullptr;
#endif
    
    DebugPanels.clear();
    bInitialized = false;
    LOG_INFO("DebugUI cleaned up");
}

void KDebugUI::NewFrame() {
    if (!bInitialized || !bVisible) {
        return;
    }
    
#ifdef USE_IMGUI
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
#endif
}

void KDebugUI::Render(ID3D11DeviceContext* context) {
    if (!bInitialized || !bVisible) {
        return;
    }
    
#ifdef USE_IMGUI
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ImGuiCtx));
    
    RenderMainMenuBar();
    RenderStatsOverlay();
    RenderRegisteredPanels();
    
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
}

void KDebugUI::BeginFrame() {
    NewFrame();
}

void KDebugUI::EndFrame() {
}

bool KDebugUI::HandleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
#ifdef USE_IMGUI
    if (bInitialized && bVisible) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        return true;
    }
#endif
    return false;
}

void KDebugUI::RegisterDebugPanel(const std::string& name, std::function<void()> renderCallback) {
    FDebugPanel panel;
    panel.Name = name;
    panel.RenderCallback = renderCallback;
    DebugPanels.push_back(panel);
}

void KDebugUI::UnregisterDebugPanel(const std::string& name) {
    DebugPanels.erase(
        std::remove_if(DebugPanels.begin(), DebugPanels.end(),
            [&name](const FDebugPanel& panel) { return panel.Name == name; }),
        DebugPanels.end()
    );
}

void KDebugUI::SetFrameStats(const FFrameStats& stats) {
    FrameStats = stats;
}

void KDebugUI::SetRenderStats(UINT drawCalls, UINT triangles, UINT vertices) {
    FrameStats.DrawCalls = drawCalls;
    FrameStats.TriangleCount = triangles;
    FrameStats.VertexCount = vertices;
}

void KDebugUI::RenderMainMenuBar() {
#ifdef USE_IMGUI
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Stats Overlay", nullptr, &bVisible);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
#endif
}

void KDebugUI::RenderStatsOverlay() {
#ifdef USE_IMGUI
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_AlwaysAutoResize | 
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;
    
    if (ImGui::Begin("Stats Overlay", nullptr, flags)) {
        ImGui::Text("FPS: %.1f (%.2f ms)", FrameStats.FPS, FrameStats.FrameTime);
        ImGui::Text("Draw Calls: %u", FrameStats.DrawCalls);
        ImGui::Text("Triangles: %u", FrameStats.TriangleCount);
        ImGui::Text("Vertices: %u", FrameStats.VertexCount);
    }
    ImGui::End();
#endif
}

void KDebugUI::RenderRegisteredPanels() {
#ifdef USE_IMGUI
    for (const auto& panel : DebugPanels) {
        if (ImGui::Begin(panel.Name.c_str())) {
            if (panel.RenderCallback) {
                panel.RenderCallback();
            }
        }
        ImGui::End();
    }
#endif
}

}
