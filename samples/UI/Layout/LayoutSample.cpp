/**
 * @file LayoutSample.cpp
 * @brief UI Layout System Sample demonstrating vertical, horizontal, and grid layouts
 * 
 * This sample demonstrates:
 * - KUIVerticalLayout for stacking elements vertically
 * - KUIHorizontalLayout for stacking elements horizontally
 * - KUIGridLayout for arranging elements in a grid
 * - KUICheckbox for toggle controls
 * - KUISlider for value adjustment
 */

#include "../../../Engine/Core/Engine.h"
#include "../../../Engine/UI/UITypes.h"
#include "../../../Engine/UI/UIFont.h"
#include "../../../Engine/UI/UICanvas.h"
#include "../../../Engine/UI/UIElement.h"
#include "../../../Engine/UI/UIPanel.h"
#include "../../../Engine/UI/UIText.h"
#include "../../../Engine/UI/UIButton.h"
#include "../../../Engine/UI/UICheckbox.h"
#include "../../../Engine/UI/UISlider.h"
#include "../../../Engine/UI/UILayout.h"
#include "../../../Engine/UI/UIVerticalLayout.h"
#include "../../../Engine/UI/UIHorizontalLayout.h"
#include "../../../Engine/UI/UIGridLayout.h"
#include "../../../Engine/Input/InputManager.h"
#include <memory>
#include <string>

class LayoutSample : public KEngine
{
public:
    LayoutSample() 
        : m_bgRed(0.1f)
        , m_bgGreen(0.1f)
        , m_bgBlue(0.15f)
        , m_showSettings(true)
        , m_volume(0.5f)
    {}
    ~LayoutSample() = default;

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

        CreateUILayouts();

        KInputManager* inputManager = GetInputManager();
        if (inputManager)
        {
            inputManager->RegisterAction("Click", VK_LBUTTON, 0);
            LOG_INFO("Layout Sample initialized");
        }

        return S_OK;
    }

    void CreateUILayouts()
    {
        auto mainPanel = std::make_shared<KUIPanel>();
        mainPanel->SetName("MainPanel");
        mainPanel->SetPosition(50, 50);
        mainPanel->SetSize(900, 600);
        mainPanel->SetBackgroundColor(FColor(0.08f, 0.08f, 0.12f, 0.95f));
        m_canvas->AddElement(mainPanel);

        auto titleText = std::make_shared<KUIText>();
        titleText->SetName("TitleText");
        titleText->SetText(L"UI Layout System Demo");
        titleText->SetPosition(70, 70);
        titleText->SetSize(860, 40);
        titleText->SetColor(FColor::Cyan());
        titleText->SetFontSize(1.6f);
        titleText->SetFont(m_font);
        m_canvas->AddElement(titleText);

        CreateVerticalLayoutSection();
        CreateHorizontalLayoutSection();
        CreateGridLayoutSection();
        CreateSettingsPanel();
        CreateStatusPanel();
    }

    void CreateVerticalLayoutSection()
    {
        auto sectionPanel = std::make_shared<KUIPanel>();
        sectionPanel->SetName("VerticalLayoutPanel");
        sectionPanel->SetPosition(70, 130);
        sectionPanel->SetSize(250, 200);
        sectionPanel->SetBackgroundColor(FColor(0.12f, 0.12f, 0.18f, 1.0f));
        m_canvas->AddElement(sectionPanel);

        auto sectionTitle = std::make_shared<KUIText>();
        sectionTitle->SetName("VerticalLayoutTitle");
        sectionTitle->SetText(L"Vertical Layout");
        sectionTitle->SetPosition(80, 140);
        sectionTitle->SetSize(230, 25);
        sectionTitle->SetColor(FColor(1.0f, 0.8f, 0.3f, 1.0f));
        sectionTitle->SetFontSize(1.1f);
        sectionTitle->SetFont(m_font);
        m_canvas->AddElement(sectionTitle);

        m_verticalLayout = std::make_shared<KUIVerticalLayout>();
        m_verticalLayout->SetName("VerticalLayout");
        m_verticalLayout->SetPosition(85, 175);
        m_verticalLayout->SetSize(220, 140);
        m_verticalLayout->SetSpacing(8.0f);
        m_verticalLayout->SetPadding(FPadding(5, 5, 5, 5));
        m_canvas->AddElement(m_verticalLayout);

        for (int i = 0; i < 4; i++)
        {
            auto button = std::make_shared<KUIButton>();
            button->SetName("VButton" + std::to_string(i));
            button->SetText(L"Option " + std::to_wstring(i + 1));
            button->SetSize(200, 28);
            button->GetTextElement()->SetFont(m_font);
            button->SetOnClickCallback([this, i]() {
                m_lastActionText->SetText(L"Vertical button " + std::to_wstring(i + 1) + L" clicked");
            });
            m_verticalLayout->AddChild(button);
        }
    }

    void CreateHorizontalLayoutSection()
    {
        auto sectionPanel = std::make_shared<KUIPanel>();
        sectionPanel->SetName("HorizontalLayoutPanel");
        sectionPanel->SetPosition(340, 130);
        sectionPanel->SetSize(280, 200);
        sectionPanel->SetBackgroundColor(FColor(0.12f, 0.12f, 0.18f, 1.0f));
        m_canvas->AddElement(sectionPanel);

        auto sectionTitle = std::make_shared<KUIText>();
        sectionTitle->SetName("HorizontalLayoutTitle");
        sectionTitle->SetText(L"Horizontal Layout");
        sectionTitle->SetPosition(350, 140);
        sectionTitle->SetSize(260, 25);
        sectionTitle->SetColor(FColor(0.3f, 1.0f, 0.5f, 1.0f));
        sectionTitle->SetFontSize(1.1f);
        sectionTitle->SetFont(m_font);
        m_canvas->AddElement(sectionTitle);

        m_horizontalLayout = std::make_shared<KUIHorizontalLayout>();
        m_horizontalLayout->SetName("HorizontalLayout");
        m_horizontalLayout->SetPosition(350, 175);
        m_horizontalLayout->SetSize(260, 140);
        m_horizontalLayout->SetSpacing(8.0f);
        m_horizontalLayout->SetPadding(FPadding(5, 5, 5, 5));
        m_canvas->AddElement(m_horizontalLayout);

        auto colors = { FColor::Red(), FColor::Green(), FColor::Blue(), FColor::Yellow() };
        auto names = { L"R", L"G", L"B", L"Y" };
        int idx = 0;
        for (auto& color : colors)
        {
            auto panel = std::make_shared<KUIPanel>();
            panel->SetName("HPanel" + std::to_string(idx));
            panel->SetSize(55, 130);
            panel->SetBackgroundColor(color);
            m_horizontalLayout->AddChild(panel);
            idx++;
        }
    }

    void CreateGridLayoutSection()
    {
        auto sectionPanel = std::make_shared<KUIPanel>();
        sectionPanel->SetName("GridLayoutPanel");
        sectionPanel->SetPosition(640, 130);
        sectionPanel->SetSize(290, 200);
        sectionPanel->SetBackgroundColor(FColor(0.12f, 0.12f, 0.18f, 1.0f));
        m_canvas->AddElement(sectionPanel);

        auto sectionTitle = std::make_shared<KUIText>();
        sectionTitle->SetName("GridLayoutTitle");
        sectionTitle->SetText(L"Grid Layout (3x3)");
        sectionTitle->SetPosition(650, 140);
        sectionTitle->SetSize(270, 25);
        sectionTitle->SetColor(FColor(0.8f, 0.4f, 1.0f, 1.0f));
        sectionTitle->SetFontSize(1.1f);
        sectionTitle->SetFont(m_font);
        m_canvas->AddElement(sectionTitle);

        m_gridLayout = std::make_shared<KUIGridLayout>();
        m_gridLayout->SetName("GridLayout");
        m_gridLayout->SetPosition(650, 175);
        m_gridLayout->SetSize(270, 145);
        m_gridLayout->SetColumns(3);
        m_gridLayout->SetSpacing(6.0f);
        m_gridLayout->SetPadding(FPadding(5, 5, 5, 5));
        m_canvas->AddElement(m_gridLayout);

        for (int i = 0; i < 9; i++)
        {
            auto button = std::make_shared<KUIButton>();
            button->SetName("GridButton" + std::to_string(i));
            button->SetText(std::to_wstring(i + 1));
            button->SetSize(80, 40);
            button->GetTextElement()->SetFont(m_font);
            button->SetOnClickCallback([this, i]() {
                m_lastActionText->SetText(L"Grid cell " + std::to_wstring(i + 1) + L" clicked");
            });
            m_gridLayout->AddChild(button);
        }
    }

    void CreateSettingsPanel()
    {
        auto settingsPanel = std::make_shared<KUIPanel>();
        settingsPanel->SetName("SettingsPanel");
        settingsPanel->SetPosition(70, 350);
        settingsPanel->SetSize(860, 150);
        settingsPanel->SetBackgroundColor(FColor(0.1f, 0.1f, 0.14f, 1.0f));
        m_canvas->AddElement(settingsPanel);

        auto settingsTitle = std::make_shared<KUIText>();
        settingsTitle->SetName("SettingsTitle");
        settingsTitle->SetText(L"Interactive Controls");
        settingsTitle->SetPosition(85, 360);
        settingsTitle->SetSize(200, 25);
        settingsTitle->SetColor(FColor(1.0f, 0.6f, 0.2f, 1.0f));
        settingsTitle->SetFontSize(1.1f);
        settingsTitle->SetFont(m_font);
        m_canvas->AddElement(settingsTitle);

        m_checkbox1 = std::make_shared<KUICheckbox>();
        m_checkbox1->SetName("ShowSettingsCheckbox");
        m_checkbox1->SetPosition(85, 400);
        m_checkbox1->SetSize(200, 25);
        m_checkbox1->SetChecked(m_showSettings);
        m_checkbox1->SetOnValueChangedCallback([this](bool bChecked) {
            m_showSettings = bChecked;
            m_lastActionText->SetText(L"Show settings: " + std::wstring(bChecked ? L"Yes" : L"No"));
        });
        m_canvas->AddElement(m_checkbox1);

        auto checkboxLabel1 = std::make_shared<KUIText>();
        checkboxLabel1->SetName("CheckboxLabel1");
        checkboxLabel1->SetText(L"  Enable Settings");
        checkboxLabel1->SetPosition(110, 400);
        checkboxLabel1->SetSize(180, 25);
        checkboxLabel1->SetColor(FColor::White());
        checkboxLabel1->SetFontSize(0.9f);
        checkboxLabel1->SetFont(m_font);
        m_canvas->AddElement(checkboxLabel1);

        m_checkbox2 = std::make_shared<KUICheckbox>();
        m_checkbox2->SetName("FullscreenCheckbox");
        m_checkbox2->SetPosition(300, 400);
        m_checkbox2->SetSize(200, 25);
        m_checkbox2->SetChecked(false);
        m_checkbox2->SetOnValueChangedCallback([this](bool bChecked) {
            m_lastActionText->SetText(L"Fullscreen: " + std::wstring(bChecked ? L"Yes" : L"No"));
        });
        m_canvas->AddElement(m_checkbox2);

        auto checkboxLabel2 = std::make_shared<KUIText>();
        checkboxLabel2->SetName("CheckboxLabel2");
        checkboxLabel2->SetText(L"  Fullscreen Mode");
        checkboxLabel2->SetPosition(325, 400);
        checkboxLabel2->SetSize(180, 25);
        checkboxLabel2->SetColor(FColor::White());
        checkboxLabel2->SetFontSize(0.9f);
        checkboxLabel2->SetFont(m_font);
        m_canvas->AddElement(checkboxLabel2);

        auto volumeLabel = std::make_shared<KUIText>();
        volumeLabel->SetName("VolumeLabel");
        volumeLabel->SetText(L"Volume:");
        volumeLabel->SetPosition(85, 450);
        volumeLabel->SetSize(80, 25);
        volumeLabel->SetColor(FColor::White());
        volumeLabel->SetFontSize(0.9f);
        volumeLabel->SetFont(m_font);
        m_canvas->AddElement(volumeLabel);

        m_volumeSlider = std::make_shared<KUISlider>();
        m_volumeSlider->SetName("VolumeSlider");
        m_volumeSlider->SetPosition(170, 450);
        m_volumeSlider->SetSize(300, 25);
        m_volumeSlider->SetMinValue(0.0f);
        m_volumeSlider->SetMaxValue(100.0f);
        m_volumeSlider->SetValue(m_volume * 100.0f);
        m_volumeSlider->SetOnValueChangedCallback([this](float Value) {
            m_volume = Value / 100.0f;
            m_volumeValueText->SetText(std::to_wstring((int)Value) + L"%");
        });
        m_canvas->AddElement(m_volumeSlider);

        m_volumeValueText = std::make_shared<KUIText>();
        m_volumeValueText->SetName("VolumeValueText");
        m_volumeValueText->SetText(L"50%");
        m_volumeValueText->SetPosition(480, 450);
        m_volumeValueText->SetSize(50, 25);
        m_volumeValueText->SetColor(FColor::Cyan());
        m_volumeValueText->SetFontSize(0.9f);
        m_volumeValueText->SetFont(m_font);
        m_canvas->AddElement(m_volumeValueText);

        auto brightnessLabel = std::make_shared<KUIText>();
        brightnessLabel->SetName("BrightnessLabel");
        brightnessLabel->SetText(L"Brightness:");
        brightnessLabel->SetPosition(550, 450);
        brightnessLabel->SetSize(100, 25);
        brightnessLabel->SetColor(FColor::White());
        brightnessLabel->SetFontSize(0.9f);
        brightnessLabel->SetFont(m_font);
        m_canvas->AddElement(brightnessLabel);

        m_brightnessSlider = std::make_shared<KUISlider>();
        m_brightnessSlider->SetName("BrightnessSlider");
        m_brightnessSlider->SetPosition(650, 450);
        m_brightnessSlider->SetSize(250, 25);
        m_brightnessSlider->SetMinValue(0.0f);
        m_brightnessSlider->SetMaxValue(200.0f);
        m_brightnessSlider->SetValue(100.0f);
        m_brightnessSlider->SetOnValueChangedCallback([this](float Value) {
            m_brightnessValueText->SetText(std::to_wstring((int)Value) + L"%");
        });
        m_canvas->AddElement(m_brightnessSlider);

        m_brightnessValueText = std::make_shared<KUIText>();
        m_brightnessValueText->SetName("BrightnessValueText");
        m_brightnessValueText->SetText(L"100%");
        m_brightnessValueText->SetPosition(910, 450);
        m_brightnessValueText->SetSize(50, 25);
        m_brightnessValueText->SetColor(FColor::Cyan());
        m_brightnessValueText->SetFontSize(0.9f);
        m_brightnessValueText->SetFont(m_font);
        m_canvas->AddElement(m_brightnessValueText);
    }

    void CreateStatusPanel()
    {
        auto statusPanel = std::make_shared<KUIPanel>();
        statusPanel->SetName("StatusPanel");
        statusPanel->SetPosition(70, 520);
        statusPanel->SetSize(860, 110);
        statusPanel->SetBackgroundColor(FColor(0.08f, 0.1f, 0.12f, 1.0f));
        m_canvas->AddElement(statusPanel);

        auto statusTitle = std::make_shared<KUIText>();
        statusTitle->SetName("StatusTitle");
        statusTitle->SetText(L"Status & Information");
        statusTitle->SetPosition(85, 530);
        statusTitle->SetSize(200, 25);
        statusTitle->SetColor(FColor::Yellow());
        statusTitle->SetFontSize(1.0f);
        statusTitle->SetFont(m_font);
        m_canvas->AddElement(statusTitle);

        m_lastActionText = std::make_shared<KUIText>();
        m_lastActionText->SetName("LastActionText");
        m_lastActionText->SetText(L"Interact with the controls above");
        m_lastActionText->SetPosition(85, 560);
        m_lastActionText->SetSize(830, 25);
        m_lastActionText->SetColor(FColor::White());
        m_lastActionText->SetFontSize(0.9f);
        m_lastActionText->SetFont(m_font);
        m_canvas->AddElement(m_lastActionText);

        auto infoText = std::make_shared<KUIText>();
        infoText->SetName("InfoText");
        infoText->SetText(L"Layout System: Vertical | Horizontal | Grid | Checkbox | Slider");
        infoText->SetPosition(85, 590);
        infoText->SetSize(830, 25);
        infoText->SetColor(FColor::Gray(0.6f));
        infoText->SetFontSize(0.85f);
        infoText->SetFont(m_font);
        m_canvas->AddElement(infoText);
    }

    void Update(float DeltaTime) override
    {
        KEngine::Update(DeltaTime);
        m_canvas->Update(DeltaTime);
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

    std::shared_ptr<KUIVerticalLayout> m_verticalLayout;
    std::shared_ptr<KUIHorizontalLayout> m_horizontalLayout;
    std::shared_ptr<KUIGridLayout> m_gridLayout;

    std::shared_ptr<KUICheckbox> m_checkbox1;
    std::shared_ptr<KUICheckbox> m_checkbox2;
    std::shared_ptr<KUISlider> m_volumeSlider;
    std::shared_ptr<KUISlider> m_brightnessSlider;

    std::shared_ptr<KUIText> m_volumeValueText;
    std::shared_ptr<KUIText> m_brightnessValueText;
    std::shared_ptr<KUIText> m_lastActionText;

    float m_bgRed;
    float m_bgGreen;
    float m_bgBlue;
    bool m_showSettings;
    float m_volume;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return KEngine::RunApplication<LayoutSample>(
        hInstance,
        L"KojeomGameEngine - UI Layout Sample",
        1024, 720,
        [](LayoutSample* Sample) -> HRESULT {
            return Sample->InitializeSample();
        }
    );
}
