#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Kojeom
{
class StringId
{
public:
    StringId() : m_id(0) {}
    explicit StringId(const std::string& str) : m_id(HashString(str))
    {
        auto& registry = GetRegistry();
        registry[m_id] = str;
    }

    uint64_t GetId() const { return m_id; }
    const std::string& GetString() const
    {
        auto& registry = GetRegistry();
        auto it = registry.find(m_id);
        if (it != registry.end()) return it->second;
        static const std::string empty = "";
        return empty;
    }

    bool operator==(const StringId& other) const { return m_id == other.m_id; }
    bool operator!=(const StringId& other) const { return m_id != other.m_id; }
    bool operator<(const StringId& other) const { return m_id < other.m_id; }
    explicit operator bool() const { return m_id != 0; }

private:
    static uint64_t HashString(const std::string& str)
    {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : str)
        {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    static std::unordered_map<uint64_t, std::string>& GetRegistry()
    {
        static std::unordered_map<uint64_t, std::string> registry;
        return registry;
    }

    uint64_t m_id;
};
}

namespace std
{
template<>
struct hash<Kojeom::StringId>
{
    size_t operator()(const Kojeom::StringId& id) const
    {
        return static_cast<size_t>(id.GetId());
    }
};
}
