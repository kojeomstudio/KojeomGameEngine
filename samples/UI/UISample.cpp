#include "../../Engine/Core/Engine.h"
#include "../../Engine/UI/UITypes.h"
#include "../../Engine/UI/UIFont.h"
#include "../../Engine/UI/UICanvas.h"
#include "../../Engine/UI/UIElement.h"
#include "../../Engine/UI/UIPanel.h"
#include "../../Engine/UI/UIText.h"
#include "../../Engine/UI/UIButton.h"
#include "../../Engine/Input/InputManager.h"
#include <memory>
#include <string>

class UISample : public KEngine
{
public:
    UISample() : m_clickCount(0) {}
    ~UISample() = default;

    HRESULT InitializeSample()
    {
        auto renderer = GetRenderer();
        auto graphicsDevice = GetGraphicsDevice();
        if (!renderer || !graphicsDevice)
        {
            LOG_ERROR("Renderer not initialized");
            return E_FAIL;
        }

        UINT32 width = GetWindowWidth();
        UINT32 height = GetWindowHeight();

        m_canvas = std::make_shared<KUICanvas>();
        if (FAILED(m_canvas->Initialize(graphicsDevice->GetDevice(), width, height)))
        {
            LOG_ERROR("Failed to initialize UI canvas");
            return E_FAIL;
        }

        m_font = std::make_shared<KUIFont>();
        if (FAILED(m_font->Initialize(graphicsDevice->GetDevice(), L"")))
        {
            LOG_ERROR("Failed to initialize default font");
            return E_FAIL;
        }
        m_canvas->SetDefaultFont(m_font);

        CreateUIElements();

        KInputManager* inputManager = GetInputManager();
        if (inputManager)
        {
            inputManager->RegisterAction("Click", VK_LBUTTON, 0);
            LOG_INFO("UI Sample initialized");
        }

        return S_OK;
    }

    void CreateUIElements()
    {
        auto backgroundPanel = std::make_shared<KUIPanel>();
        backgroundPanel->SetName("BackgroundPanel");
        backgroundPanel->SetPosition(50, 50);
        backgroundPanel->SetSize(400, 500);
        backgroundPanel->SetBackgroundColor(FColor(0.1f, 0.1f, 0.15f, 0.9f));
        m_canvas->AddElement(backgroundPanel);

        auto titleText = std::make_shared<KUIText>();
        titleText->SetName("TitleText");
        titleText->SetText(L"KojeomGameEngine UI Demo");
        titleText->SetPosition(70, 70);
        titleText->SetSize(360, 40);
        titleText->SetColor(FColor::Cyan());
        titleText->SetFontSize(1.5f);
        titleText->SetFont(m_font);
        m_canvas->AddElement(titleText);

        auto subtitleText = std::make_shared<KUIText>();
        subtitleText->SetName("SubtitleText");
        subtitleText->SetText(L"Demonstrating UI components");
        subtitleText->SetPosition(70, 110);
        subtitleText->SetSize(360, 25);
        subtitleText->SetColor(FColor::Gray(0.7f));
        subtitleText->SetFontSize(1.0f);
        subtitleText->SetFont(m_font);
        m_canvas->AddElement(subtitleText);

        auto infoPanel = std::make_shared<KUIPanel>();
        infoPanel->SetName("InfoPanel");
        infoPanel->SetPosition(70, 150);
        infoPanel->SetSize(360, 100);
        infoPanel->SetBackgroundColor(FColor(0.15f, 0.15f, 0.2f, 1.0f));
        m_canvas->AddElement(infoPanel);

        auto infoText = std::make_shared<KUIText>();
        infoText->SetName("InfoText");
        infoText->SetText(L"This panel demonstrates:\n- Panels with borders\n- Text elements\n- Buttons");
        infoText->SetPosition(80, 160);
        infoText->SetSize(340, 80);
        infoText->SetColor(FColor::White());
        infoText->SetFontSize(0.9f);
        infoText->SetFont(m_font);
        m_canvas->AddElement(infoText);

        m_statusText = std::make_shared<KUIText>();
        m_statusText->SetName("StatusText");
        m_statusText->SetText(L"Click count: 0");
        m_statusText->SetPosition(70, 350);
        m_statusText->SetSize(360, 30);
        m_statusText->SetColor(FColor::Yellow());
        m_statusText->SetFontSize(1.0f);
        m_statusText->SetFont(m_font);
        m_canvas->AddElement(m_statusText);

        auto statsPanel = std::make_shared<KUIPanel>();
        statsPanel->SetName("StatsPanel");
        statsPanel->SetPosition(500, 50);
        statsPanel->SetSize(474, 500);
        statsPanel->SetBackgroundColor(FColor(0.12f, 0.12f, 0.18f, 0.9f));
        m_canvas->AddElement(statsPanel);

        auto statsTitle = std::make_shared<KUIText>();
        statsTitle->SetName("StatsTitle");
        statsTitle->SetText(L"Features");
        statsTitle->SetPosition(520, 70);
        statsTitle->SetSize(434, 40);
        statsTitle->SetColor(FColor(1.0f, 0.7f, 0.3f, 1.0f));
        statsTitle->SetFontSize(1.3f);
        statsTitle->SetFont(m_font);
        m_canvas->AddElement(statsTitle);

        auto statsText = std::make_shared<KUIText>();
        statsText->SetName("StatsText");
        statsText->SetText(L"UI System Features:\n\n- UICanvas container\n- UIPanel backgrounds\n- UIText rendering\n- UIFont system\n- Mouse interaction\n\nPress Left Mouse Button\nto increment counter");
        statsText->SetPosition(520, 120);
        statsText->SetSize(434, 300);
        statsText->SetColor(FColor::White());
        statsText->SetFontSize(1.0f);
        statsText->SetFont(m_font);
        m_canvas->AddElement(statsText);

        auto footerText = std::make_shared<KUIText>();
        footerText->SetName("FooterText");
        footerText->SetText(L"Use mouse to interact");
        footerText->SetPosition(70, 460);
        footerText->SetSize(360, 20);
        footerText->SetColor(FColor::Gray(0.5f));
        footerText->SetFontSize(0.8f);
        footerText->SetFont(m_font);
        m_canvas->AddElement(footerText);
    }

    void Update(float DeltaTime) override
    {
        KEngine::Update(DeltaTime);
        m_canvas->Update(DeltaTime);

        KInputManager* inputManager = GetInputManager();
        if (inputManager)
        {
            if (inputManager->IsActionPressed("Click"))
            {
                m_clickCount++;
                if (m_statusText)
                {
                    std::wstring text = L"Click count: " + std::to_wstring(m_clickCount);
                    m_statusText->SetText(text);
                }
            }
        }
    }

    void Render() override
    {
        KEngine::Render();

        auto graphicsDevice = GetGraphicsDevice();
        if (graphicsDevice)
        {
            m_canvas->Render(graphicsDevice->GetContext());
        }
    }

private:
    std::shared_ptr<KUICanvas> m_canvas;
    std::shared_ptr<KUIFont> m_font;
    std::shared_ptr<KUIText> m_statusText;
    int m_clickCount;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return KEngine::RunApplication<UISample>(
        hInstance,
        L"KojeomGameEngine - UI Sample",
        1024, 768,
        [](UISample* Sample) -> HRESULT {
            return Sample->InitializeSample();
        }
    );
}
