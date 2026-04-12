#pragma once

#include "../Utils/Common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <sstream>

class KJsonValue;
class KJsonObject;
class KJsonArray;

using JsonValuePtr = std::shared_ptr<KJsonValue>;
using JsonObjectPtr = std::shared_ptr<KJsonObject>;
using JsonArrayPtr = std::shared_ptr<KJsonArray>;

enum class EJsonType
{
    None,
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class KJsonValue
{
public:
    KJsonValue() : Type(EJsonType::None) {}
    explicit KJsonValue(EJsonType InType) : Type(InType) {}
    virtual ~KJsonValue() = default;

    EJsonType GetType() const { return Type; }

    virtual bool AsBool() const { return false; }
    virtual double AsNumber() const { return 0.0; }
    virtual int AsInt() const { return static_cast<int>(AsNumber()); }
    virtual float AsFloat() const { return static_cast<float>(AsNumber()); }
    virtual const std::string& AsString() const { static std::string empty; return empty; }
    virtual JsonObjectPtr AsObject() const { return nullptr; }
    virtual JsonArrayPtr AsArray() const { return nullptr; }

    bool IsNull() const { return Type == EJsonType::Null; }
    bool IsBool() const { return Type == EJsonType::Bool; }
    bool IsNumber() const { return Type == EJsonType::Number; }
    bool IsString() const { return Type == EJsonType::String; }
    bool IsArray() const { return Type == EJsonType::Array; }
    bool IsObject() const { return Type == EJsonType::Object; }

protected:
    EJsonType Type;
};

class KJsonNull : public KJsonValue
{
public:
    KJsonNull() : KJsonValue(EJsonType::Null) {}
};

class KJsonBool : public KJsonValue
{
public:
    explicit KJsonBool(bool Value) : KJsonValue(EJsonType::Bool), BoolValue(Value) {}
    bool AsBool() const override { return BoolValue; }
private:
    bool BoolValue;
};

class KJsonNumber : public KJsonValue
{
public:
    explicit KJsonNumber(double Value) : KJsonValue(EJsonType::Number), NumberValue(Value) {}
    double AsNumber() const override { return NumberValue; }
private:
    double NumberValue;
};

class KJsonString : public KJsonValue
{
public:
    explicit KJsonString(const std::string& Value) : KJsonValue(EJsonType::String), StringValue(Value) {}
    const std::string& AsString() const override { return StringValue; }
private:
    std::string StringValue;
};

class KJsonArray : public KJsonValue, public std::enable_shared_from_this<KJsonArray>
{
public:
    KJsonArray() : KJsonValue(EJsonType::Array) {}

    JsonArrayPtr AsArray() const override
    {
        return std::const_pointer_cast<KJsonArray>(shared_from_this());
    }

    void Add(JsonValuePtr Value) { Values.push_back(Value); }
    void Add(bool Value) { Values.push_back(std::make_shared<KJsonBool>(Value)); }
    void Add(int Value) { Values.push_back(std::make_shared<KJsonNumber>(static_cast<double>(Value))); }
    void Add(float Value) { Values.push_back(std::make_shared<KJsonNumber>(static_cast<double>(Value))); }
    void Add(double Value) { Values.push_back(std::make_shared<KJsonNumber>(Value)); }
    void Add(const std::string& Value) { Values.push_back(std::make_shared<KJsonString>(Value)); }
    void Add(const char* Value) { Values.push_back(std::make_shared<KJsonString>(std::string(Value))); }

    size_t Size() const { return Values.size(); }
    JsonValuePtr Get(size_t Index) const 
    { 
        return Index < Values.size() ? Values[Index] : nullptr; 
    }

    const std::vector<JsonValuePtr>& GetValues() const { return Values; }

private:
    std::vector<JsonValuePtr> Values;
};

class KJsonObject : public KJsonValue, public std::enable_shared_from_this<KJsonObject>
{
public:
    KJsonObject() : KJsonValue(EJsonType::Object) {}

    JsonObjectPtr AsObject() const override 
    { 
        return std::const_pointer_cast<KJsonObject>(std::shared_ptr<const KJsonObject>(shared_from_this())); 
    }

    void Set(const std::string& Key, JsonValuePtr Value) { Values[Key] = Value; }
    void Set(const std::string& Key, bool Value) { Values[Key] = std::make_shared<KJsonBool>(Value); }
    void Set(const std::string& Key, int Value) { Values[Key] = std::make_shared<KJsonNumber>(static_cast<double>(Value)); }
    void Set(const std::string& Key, float Value) { Values[Key] = std::make_shared<KJsonNumber>(static_cast<double>(Value)); }
    void Set(const std::string& Key, double Value) { Values[Key] = std::make_shared<KJsonNumber>(Value); }
    void Set(const std::string& Key, const std::string& Value) { Values[Key] = std::make_shared<KJsonString>(Value); }
    void Set(const std::string& Key, const char* Value) { Values[Key] = std::make_shared<KJsonString>(std::string(Value)); }
    void SetNull(const std::string& Key) { Values[Key] = std::make_shared<KJsonNull>(); }

    bool Has(const std::string& Key) const { return Values.find(Key) != Values.end(); }
    JsonValuePtr Get(const std::string& Key) const 
    { 
        auto it = Values.find(Key);
        return it != Values.end() ? it->second : nullptr; 
    }

    bool GetBool(const std::string& Key, bool Default = false) const
    {
        auto val = Get(Key);
        return val && val->IsBool() ? val->AsBool() : Default;
    }

    int GetInt(const std::string& Key, int Default = 0) const
    {
        auto val = Get(Key);
        return val && val->IsNumber() ? val->AsInt() : Default;
    }

    float GetFloat(const std::string& Key, float Default = 0.0f) const
    {
        auto val = Get(Key);
        return val && val->IsNumber() ? val->AsFloat() : Default;
    }

    double GetDouble(const std::string& Key, double Default = 0.0) const
    {
        auto val = Get(Key);
        return val && val->IsNumber() ? val->AsNumber() : Default;
    }

    std::string GetString(const std::string& Key, const std::string& Default = "") const
    {
        auto val = Get(Key);
        return val && val->IsString() ? val->AsString() : Default;
    }

    JsonObjectPtr GetObject(const std::string& Key) const
    {
        auto val = Get(Key);
        return val && val->IsObject() ? val->AsObject() : nullptr;
    }

    JsonArrayPtr GetArray(const std::string& Key) const
    {
        auto val = Get(Key);
        return val && val->IsArray() ? val->AsArray() : nullptr;
    }

    const std::unordered_map<std::string, JsonValuePtr>& GetValues() const { return Values; }

private:
    std::unordered_map<std::string, JsonValuePtr> Values;
};

class KJsonArchive
{
public:
    KJsonArchive() = default;
    ~KJsonArchive() = default;

    bool LoadFromFile(const std::wstring& Path);
    bool SaveToFile(const std::wstring& Path);

    JsonObjectPtr GetRoot() const { return Root; }
    void SetRoot(JsonObjectPtr InRoot) { Root = InRoot; }

    static JsonObjectPtr CreateObject() { return std::make_shared<KJsonObject>(); }
    static JsonArrayPtr CreateArray() { return std::make_shared<KJsonArray>(); }

    std::string SerializeToString() const;
    bool DeserializeFromString(const std::string& JsonString);

private:
    JsonObjectPtr Root;

    std::string EscapeString(const std::string& Str) const;
    std::string UnescapeString(const std::string& Str) const;
    void SerializeValue(std::ostringstream& Stream, const JsonValuePtr& Value) const;
    JsonValuePtr ParseValue(const std::string& Str, size_t& Pos, int32 Depth = 0) const;
    JsonObjectPtr ParseObject(const std::string& Str, size_t& Pos, int32 Depth = 0) const;
    JsonArrayPtr ParseArray(const std::string& Str, size_t& Pos, int32 Depth = 0) const;
    std::string ParseString(const std::string& Str, size_t& Pos) const;
    double ParseNumber(const std::string& Str, size_t& Pos) const;
    bool ParseBool(const std::string& Str, size_t& Pos) const;
    void SkipWhitespace(const std::string& Str, size_t& Pos) const;

    static constexpr int32 MaxNestingDepth = 200;
    static constexpr size_t MaxArrayElements = 100000;
    static constexpr size_t MaxObjectProperties = 10000;
};
