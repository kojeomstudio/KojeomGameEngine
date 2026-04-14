#pragma once

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#define NOMINMAX

#include <windows.h>

#include <d3d11_1.h>
#include <directxcolors.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <iostream>
#include <crtdbg.h>

using namespace DirectX;

using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

#define SAFE_RELEASE(p) do { if((p)) { (p)->Release(); (p) = nullptr; } } while(0)
#define SAFE_DELETE(p) do { if((p)) { delete (p); (p) = nullptr; } } while(0)
#define SAFE_DELETE_ARRAY(p) do { if((p)) { delete[] (p); (p) = nullptr; } } while(0)

#define CHECK_HRESULT(hr) do { if(FAILED(hr)) return (hr); } while(0)
#define LOG_ERROR(msg) KLogger::Error(msg)
#define LOG_INFO(msg) KLogger::Info(msg)
#define LOG_WARNING(msg) KLogger::Warning(msg)

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class KEngine;
class KGraphicsDevice;
class KRenderer;
class KCamera;
class KLogger;

namespace StringUtils
{
    inline std::string WideToMultiByte(const std::wstring& WideStr)
    {
        if (WideStr.empty()) return std::string();
        
        int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &WideStr[0], (int)WideStr.size(), NULL, 0, NULL, NULL);
        std::string StrTo(SizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, &WideStr[0], (int)WideStr.size(), &StrTo[0], SizeNeeded, NULL, NULL);
        return StrTo;
    }

    inline std::wstring MultiByteToWide(const std::string& MultiStr)
    {
        if (MultiStr.empty()) return std::wstring();
        
        int SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &MultiStr[0], (int)MultiStr.size(), NULL, 0);
        std::wstring WStrTo(SizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, &MultiStr[0], (int)MultiStr.size(), &WStrTo[0], SizeNeeded);
        return WStrTo;
    }
}

namespace PathUtils
{
    inline bool ContainsTraversal(const std::wstring& Path)
    {
        if (Path.empty()) return true;

        std::wstring normalized = Path;
        for (auto& c : normalized)
        {
            if (c == L'/') c = L'\\';
        }

        if (normalized.find(L"..\\") != std::wstring::npos ||
            normalized.find(L"../") != std::wstring::npos)
        {
            return true;
        }

        if (normalized.size() >= 2 && normalized[0] == L'\\' && normalized[1] == L'\\')
        {
            return true;
        }

        return false;
    }

    inline bool IsPathSafe(const std::wstring& Path, const std::wstring& BaseDir)
    {
        if (ContainsTraversal(Path)) return false;

        if (BaseDir.empty()) return true;

        wchar_t fullPath[MAX_PATH] = {};
        wchar_t resolvedBase[MAX_PATH] = {};

        if (!_wfullpath(fullPath, Path.c_str(), MAX_PATH)) return false;
        if (!_wfullpath(resolvedBase, BaseDir.c_str(), MAX_PATH)) return false;

        size_t basePathLen = wcslen(resolvedBase);
        if (wcsncmp(fullPath, resolvedBase, basePathLen) != 0) return false;

        return true;
    }
}

namespace EngineConstants
{
    constexpr UINT32 DEFAULT_WINDOW_WIDTH = 1024;
    constexpr UINT32 DEFAULT_WINDOW_HEIGHT = 768;
    constexpr float DEFAULT_FOV = XM_PIDIV4;
    constexpr float DEFAULT_NEAR_PLANE = 0.1f;
    constexpr float DEFAULT_FAR_PLANE = 1000.0f;
} 
