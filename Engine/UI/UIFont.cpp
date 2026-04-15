#include "UIFont.h"
#include "../Utils/Common.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <sstream>

KUIFont::KUIFont()
    : FontSize(0)
    , LineHeight(0)
    , Base(0)
    , TextureWidth(0)
    , TextureHeight(0)
    , bInitialized(false)
{
}

KUIFont::~KUIFont()
{
    Cleanup();
}

void KUIFont::Cleanup()
{
    FontTextureSRV.Reset();
    Characters.clear();
    Pages.clear();
    bInitialized = false;
}

HRESULT KUIFont::Initialize(ID3D11Device* Device, const std::wstring& FontPath)
{
    if (!Device)
    {
        LOG_ERROR("Invalid device for font initialization");
        return E_INVALIDARG;
    }

    HRESULT hr = LoadFontFile(FontPath);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to load font file");
        return hr;
    }

    if (!Pages.empty())
    {
        hr = CreateFontTexture(Device, Pages[0].TexturePath);
        if (FAILED(hr))
        {
            LOG_WARNING("Failed to load font texture, using generated texture");
        }
    }

    bInitialized = true;
    LOG_INFO("Font initialized successfully");
    return S_OK;
}

HRESULT KUIFont::LoadFontFile(const std::wstring& FontPath)
{
    if (PathUtils::ContainsTraversal(FontPath))
    {
        LOG_ERROR("Font: path contains traversal patterns");
        return E_INVALIDARG;
    }

    std::ifstream file(FontPath);
    if (!file.is_open())
    {
        std::ifstream fntFile(FontPath + L".fnt");
        if (!fntFile.is_open())
        {
            FontSize = 24.0f;
            LineHeight = 30.0f;
            Base = 20.0f;
            TextureWidth = 256.0f;
            TextureHeight = 256.0f;

            Pages.push_back({ L"", 256.0f, 256.0f });

            for (wchar_t c = 32; c < 127; c++)
            {
                FFontChar ch;
                ch.Character = c;
                ch.Width = FontSize * 0.6f;
                ch.Height = FontSize;
                ch.XOffset = 0;
                ch.YOffset = 0;
                ch.XAdvance = ch.Width + 2;
                ch.Page = 0;

                int index = c - 32;
                int cols = 16;
                ch.X = (index % cols) * (TextureWidth / cols);
                ch.Y = (index / cols) * (TextureHeight / 8);

                Characters[c] = ch;
            }

            return S_OK;
        }
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "info")
        {
            std::string token;
            while (iss >> token)
            {
                if (token.find("size=") == 0)
                {
                    try { FontSize = std::stof(token.substr(5)); }
                    catch (...) { FontSize = 24.0f; }
                }
            }
        }
        else if (type == "common")
        {
            std::string token;
            while (iss >> token)
            {
                if (token.find("lineHeight=") == 0)
                {
                    try { LineHeight = std::stof(token.substr(11)); }
                    catch (...) { LineHeight = 30.0f; }
                }
                else if (token.find("base=") == 0)
                {
                    try { Base = std::stof(token.substr(5)); }
                    catch (...) { Base = 20.0f; }
                }
                else if (token.find("scaleW=") == 0)
                {
                    try { TextureWidth = std::stof(token.substr(7)); }
                    catch (...) { TextureWidth = 256.0f; }
                }
                else if (token.find("scaleH=") == 0)
                {
                    try { TextureHeight = std::stof(token.substr(7)); }
                    catch (...) { TextureHeight = 256.0f; }
                }
            }
        }
        else if (type == "page")
        {
            FFontPage page;
            std::string token;
            while (iss >> token)
            {
                if (token.find("file=") == 0)
                {
                    std::string filename = token.substr(6);
                    if (!filename.empty() && filename.front() == '"')
                        filename = filename.substr(1);
                    if (!filename.empty() && filename.back() == '"')
                        filename = filename.substr(0, filename.length() - 1);

                    std::wstring baseDir = FontPath.substr(0, FontPath.find_last_of(L"\\/") + 1);
                    page.TexturePath = baseDir + std::wstring(filename.begin(), filename.end());
                }
            }
            Pages.push_back(page);
        }
        else if (type == "char")
        {
            FFontChar ch;
            ch.Page = 0;
            std::string token;
            while (iss >> token)
            {
                size_t eq = token.find('=');
                if (eq != std::string::npos)
                {
                    std::string key = token.substr(0, eq);
                    float value = 0.0f;
                    try { value = std::stof(token.substr(eq + 1)); }
                    catch (...) { continue; }

                    if (key == "id") ch.Character = (wchar_t)value;
                    else if (key == "x") ch.X = value;
                    else if (key == "y") ch.Y = value;
                    else if (key == "width") ch.Width = value;
                    else if (key == "height") ch.Height = value;
                    else if (key == "xoffset") ch.XOffset = value;
                    else if (key == "yoffset") ch.YOffset = value;
                    else if (key == "xadvance") ch.XAdvance = value;
                    else if (key == "page") ch.Page = value;
                }
            }
            Characters[ch.Character] = ch;
        }
    }

    return S_OK;
}

HRESULT KUIFont::CreateFontTexture(ID3D11Device* Device, const std::wstring& TexturePath)
{
    if (TexturePath.empty())
        return E_FAIL;

    return E_NOTIMPL;
}

void KUIFont::RenderText(ID3D11DeviceContext* Context,
                         const std::wstring& Text,
                         float X, float Y,
                         float Scale,
                         const FColor& Color,
                         std::vector<FUIVertex>& OutVertices,
                         std::vector<uint32>& OutIndices)
{
    float cursorX = X;
    float cursorY = Y;
    XMFLOAT4 color = Color.ToFloat4();

    uint32 vertexOffset = static_cast<uint32>(OutVertices.size());

    for (wchar_t c : Text)
    {
        if (c == L'\n')
        {
            cursorX = X;
            cursorY += LineHeight * Scale;
            continue;
        }

        if (c == L'\r')
            continue;

        auto it = Characters.find(c);
        if (it == Characters.end())
        {
            cursorX += FontSize * 0.5f * Scale;
            continue;
        }

        const FFontChar& ch = it->second;

        float x = cursorX + ch.XOffset * Scale;
        float y = cursorY + ch.YOffset * Scale;
        float w = ch.Width * Scale;
        float h = ch.Height * Scale;

        float u0 = ch.X / TextureWidth;
        float v0 = ch.Y / TextureHeight;
        float u1 = (ch.X + ch.Width) / TextureWidth;
        float v1 = (ch.Y + ch.Height) / TextureHeight;

        uint32 baseIndex = static_cast<uint32>(OutVertices.size());

        OutVertices.push_back(FUIVertex(XMFLOAT3(x, y, 0), XMFLOAT2(u0, v0), color));
        OutVertices.push_back(FUIVertex(XMFLOAT3(x + w, y, 0), XMFLOAT2(u1, v0), color));
        OutVertices.push_back(FUIVertex(XMFLOAT3(x + w, y + h, 0), XMFLOAT2(u1, v1), color));
        OutVertices.push_back(FUIVertex(XMFLOAT3(x, y + h, 0), XMFLOAT2(u0, v1), color));

        OutIndices.push_back(baseIndex + 0);
        OutIndices.push_back(baseIndex + 1);
        OutIndices.push_back(baseIndex + 2);
        OutIndices.push_back(baseIndex + 0);
        OutIndices.push_back(baseIndex + 2);
        OutIndices.push_back(baseIndex + 3);

        cursorX += ch.XAdvance * Scale;
    }
}

float KUIFont::GetTextWidth(const std::wstring& Text, float Scale) const
{
    float width = 0;
    float maxWidth = 0;

    for (wchar_t c : Text)
    {
        if (c == L'\n')
        {
            maxWidth = (std::max)(maxWidth, width);
            width = 0;
            continue;
        }

        auto it = Characters.find(c);
        if (it != Characters.end())
        {
            width += it->second.XAdvance * Scale;
        }
        else
        {
            width += FontSize * 0.5f * Scale;
        }
    }

    return (std::max)(maxWidth, width);
}

float KUIFont::GetLineHeight(float Scale) const
{
    return LineHeight * Scale;
}
