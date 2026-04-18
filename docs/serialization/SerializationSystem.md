# Serialization System

The KojeomGameEngine serialization system provides two complementary approaches for persisting data: a high-performance **binary archive** (`KBinaryArchive`) for compact, fast serialization, and a human-readable **JSON archive** (`KJsonArchive`) for configuration files, scene metadata, and editor data. Both live under `Engine/Serialization/`.

## Source Files

| File | Description |
|------|-------------|
| `Engine/Serialization/BinaryArchive.h` | `KBinaryArchive` class declaration |
| `Engine/Serialization/BinaryArchive.cpp` | Binary archive implementation |
| `Engine/Serialization/JsonArchive.h` | `KJsonArchive`, `KJsonValue`, `KJsonObject`, `KJsonArray` declarations |
| `Engine/Serialization/JsonArchive.cpp` | JSON DOM parser, serializer, and archive implementation |

---

## KBinaryArchive

A stream-based binary serializer that uses an in-memory buffer with file I/O via `std::fstream`. Data is written to the buffer during the session and flushed to disk on `Close()`.

### Modes

```cpp
enum class EMode { Read, Write };
```

- **Read** -- On `Open()`, the entire file is read into the in-memory buffer. Subsequent `>>` operations consume from the buffer sequentially.
- **Write** -- On `Open()`, the buffer is cleared. Subsequent `<<` operations append to the buffer. The buffer is written to disk on `Close()`.

### Construction

```cpp
KBinaryArchive Archive(KBinaryArchive::EMode::Write);
KBinaryArchive Reader(KBinaryArchive::EMode::Read);
```

The archive is non-copyable (`delete` copy constructor and assignment operator).

### File Operations

| Method | Signature | Description |
|--------|-----------|-------------|
| `Open` | `bool Open(const std::wstring& Path)` | Opens a file for reading or writing. Returns `false` on failure. |
| `Close` | `void Close()` | Flushes the write buffer to disk (write mode) and releases the file handle. |
| `IsOpen` | `bool IsOpen() const` | Returns `true` if the underlying file stream is open. |
| `IsReading` | `bool IsReading() const` | `true` if the archive was created with `EMode::Read`. |
| `IsWriting` | `bool IsWriting() const` | `true` if the archive was created with `EMode::Write`. |

### Stream Operators

All primitive types and several engine/math types are supported via overloaded `<<` (write) and `>>` (read) operators.

#### Primitive Types

```cpp
int8, uint8, int16, uint16, int32, uint32, int64, uint64, float, double, bool
```

Each primitive is written as its raw binary representation (no endianness conversion).

#### Strings

```cpp
Archive << myString;    // std::string  -- writes uint32 length prefix, then raw chars
Archive << myWideString; // std::wstring -- writes uint32 length prefix, then raw wchar_t data
```

Strings are length-prefixed with a `uint32` count followed by the character data. This allows correct deserialization without delimiters.

#### DirectX Math Types

```cpp
XMFLOAT3  // writes x, y, z as three floats
XMFLOAT4  // writes x, y, z, w as four floats
XMFLOAT4X4 // writes all 16 elements row-major (m[0][0]..m[3][3])
```

#### std::vector\<T\>

```cpp
// Writes uint32 count, then each element via operator<<
template<typename T>
KBinaryArchive& operator<<(const std::vector<T>& Value);

// Reads uint32 count, resizes, then reads each element via operator>>
template<typename T>
KBinaryArchive& operator>>(std::vector<T>& Value);
```

Any `T` that has `<<`/`>>` overloads (primitives, strings, `XMFLOAT*`, nested vectors) works automatically.

### Raw Access

| Method | Signature | Description |
|--------|-----------|-------------|
| `WriteRaw` | `void WriteRaw(const void* Data, size_t Size)` | Writes raw bytes to the buffer (write mode only). |
| `ReadRaw` | `void ReadRaw(void* Data, size_t Size)` | Reads raw bytes from the buffer (read mode only). Logs an error on overflow. |
| `GetSize` | `size_t GetSize() const` | Returns the current buffer size in bytes. |
| `GetData` | `const uint8* GetData() const` | Returns a pointer to the in-memory buffer. |

### Internal Buffer

The archive uses a `std::vector<uint8>` as its backing store:

- **Write mode**: Data is appended to the vector via `Buffer.insert()`.
- **Read mode**: The file is fully loaded into the vector on `Open()`. A `ReadPosition` cursor tracks the current read offset. Reads use `memcpy` from `Buffer.data() + ReadPosition`.

### Example: Writing Binary Data

```cpp
#include "Serialization/BinaryArchive.h"

void SavePlayerData(const std::wstring& Path)
{
    KBinaryArchive Archive(KBinaryArchive::EMode::Write);
    if (!Archive.Open(Path))
        return;

    std::string playerName = "Hero";
    int32 level = 42;
    float health = 95.5f;
    XMFLOAT3 position = { 10.0f, 0.0f, -5.0f };
    std::vector<int32> inventory = { 1001, 1002, 1003 };
    bool isAlive = true;

    Archive << playerName;
    Archive << level;
    Archive << health;
    Archive << position;
    Archive << inventory;
    Archive << isAlive;

    Archive.Close();
}
```

### Example: Reading Binary Data

```cpp
void LoadPlayerData(const std::wstring& Path)
{
    KBinaryArchive Archive(KBinaryArchive::EMode::Read);
    if (!Archive.Open(Path))
        return;

    std::string playerName;
    int32 level = 0;
    float health = 0.0f;
    XMFLOAT3 position = {};
    std::vector<int32> inventory;
    bool isAlive = false;

    Archive >> playerName;
    Archive >> level;
    Archive >> health;
    Archive >> position;
    Archive >> inventory;
    Archive >> isAlive;

    Archive.Close();
}
```

### Example: Writing Raw Struct Data

```cpp
struct FCustomData
{
    int32 Id;
    float Values[8];
};

void WriteCustomStruct(KBinaryArchive& Archive, const FCustomData& Data)
{
    Archive.WriteRaw(&Data, sizeof(FCustomData));
}
```

---

## KJsonArchive

A JSON serializer/deserializer built on a custom DOM parser with zero external dependencies. Provides full JSON round-tripping including escape/unescape support for strings.

### Type Hierarchy

All JSON values inherit from `KJsonValue` and are managed via `std::shared_ptr`:

```
KJsonValue (base)
  +-- KJsonNull
  +-- KJsonBool
  +-- KJsonNumber
  +-- KJsonString
  +-- KJsonArray
  +-- KJsonObject
```

#### Type Aliases

```cpp
using JsonValuePtr  = std::shared_ptr<KJsonValue>;
using JsonObjectPtr = std::shared_ptr<KJsonObject>;
using JsonArrayPtr  = std::shared_ptr<KJsonArray>;
```

#### EJsonType Enum

```cpp
enum class EJsonType
{
    None,    // default/uninitialized
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};
```

### KJsonValue (Base)

The abstract base for all JSON nodes. Provides type checking and value conversion:

| Method | Signature | Description |
|--------|-----------|-------------|
| `GetType` | `EJsonType GetType() const` | Returns the JSON type of this value. |
| `IsNull` | `bool IsNull() const` | Type check for null. |
| `IsBool` | `bool IsBool() const` | Type check for boolean. |
| `IsNumber` | `bool IsNumber() const` | Type check for number. |
| `IsString` | `bool IsString() const` | Type check for string. |
| `IsArray` | `bool IsArray() const` | Type check for array. |
| `IsObject` | `bool IsObject() const` | Type check for object. |
| `AsBool` | `virtual bool AsBool() const` | Convert to bool (returns `false` by default). |
| `AsNumber` | `virtual double AsNumber() const` | Convert to double (returns `0.0` by default). |
| `AsInt` | `virtual int AsInt() const` | Convert to int via `AsNumber()`. |
| `AsFloat` | `virtual float AsFloat() const` | Convert to float via `AsNumber()`. |
| `AsString` | `virtual const std::string& AsString() const` | Convert to string (returns empty ref by default). |
| `AsObject` | `virtual JsonObjectPtr AsObject() const` | Convert to object (returns `nullptr` by default). |
| `AsArray` | `virtual JsonArrayPtr AsArray() const` | Convert to array (returns `nullptr` by default). |

### KJsonNull

Represents a JSON `null` value. Constructed with `KJsonNull()`.

### KJsonBool

Wraps a `bool` value:

```cpp
auto value = std::make_shared<KJsonBool>(true);
bool b = value->AsBool(); // true
```

### KJsonNumber

Stores all numeric values as `double` internally:

```cpp
auto value = std::make_shared<KJsonNumber>(3.14);
double d = value->AsNumber(); // 3.14
int i = value->AsInt();       // 3
float f = value->AsFloat();   // 3.14f
```

### KJsonString

Wraps a `std::string`:

```cpp
auto value = std::make_shared<KJsonString>("hello");
const std::string& s = value->AsString(); // "hello"
```

### KJsonObject

An unordered collection of key-value pairs backed by `std::unordered_map<std::string, JsonValuePtr>`.

#### Construction

```cpp
auto obj = KJsonArchive::CreateObject();
// or
auto obj = std::make_shared<KJsonObject>();
```

#### Set Methods

Overloaded for convenience with primitive types:

```cpp
void Set(const std::string& Key, JsonValuePtr Value);
void Set(const std::string& Key, bool Value);
void Set(const std::string& Key, int Value);
void Set(const std::string& Key, float Value);
void Set(const std::string& Key, double Value);
void Set(const std::string& Key, const std::string& Value);
void Set(const std::string& Key, const char* Value);
void SetNull(const std::string& Key);
```

#### Get Methods

```cpp
bool Has(const std::string& Key) const;
JsonValuePtr Get(const std::string& Key) const;  // nullptr if missing
```

#### Typed Getters (with defaults)

All typed getters return a default value if the key is missing or the type does not match:

```cpp
bool        GetBool(const std::string& Key, bool Default = false) const;
int         GetInt(const std::string& Key, int Default = 0) const;
float       GetFloat(const std::string& Key, float Default = 0.0f) const;
double      GetDouble(const std::string& Key, double Default = 0.0) const;
std::string GetString(const std::string& Key, const std::string& Default = "") const;
JsonObjectPtr GetObject(const std::string& Key) const;   // nullptr if missing/wrong type
JsonArrayPtr  GetArray(const std::string& Key) const;    // nullptr if missing/wrong type
```

#### Iteration

```cpp
const std::unordered_map<std::string, JsonValuePtr>& GetValues() const;
```

### KJsonArray

An ordered list of `JsonValuePtr` elements backed by `std::vector<JsonValuePtr>`.

#### Construction

```cpp
auto arr = KJsonArchive::CreateArray();
// or
auto arr = std::make_shared<KJsonArray>();
```

#### Add Methods

Overloaded for convenience:

```cpp
void Add(JsonValuePtr Value);
void Add(bool Value);
void Add(int Value);
void Add(float Value);
void Add(double Value);
void Add(const std::string& Value);
void Add(const char* Value);
```

#### Access

```cpp
size_t Size() const;
JsonValuePtr Get(size_t Index) const;  // nullptr if out of bounds
const std::vector<JsonValuePtr>& GetValues() const;
```

### KJsonArchive

The top-level archive class that owns a root `KJsonObject` and provides file I/O and string serialization.

#### File I/O

| Method | Signature | Description |
|--------|-----------|-------------|
| `LoadFromFile` | `bool LoadFromFile(const std::wstring& Path)` | Reads a JSON file and populates the root object. Returns `false` on failure. |
| `SaveToFile` | `bool SaveToFile(const std::wstring& Path)` | Serializes the root object to a JSON file. Returns `false` on failure. |

#### String I/O

| Method | Signature | Description |
|--------|-----------|-------------|
| `SerializeToString` | `std::string SerializeToString() const` | Serializes the root to a JSON string. Returns `"{}"` if no root is set. |
| `DeserializeFromString` | `bool DeserializeFromString(const std::string& JsonString)` | Parses a JSON string and sets the root object. Returns `false` if the top-level value is not an object. |

#### Root Access

| Method | Signature | Description |
|--------|-----------|-------------|
| `GetRoot` | `JsonObjectPtr GetRoot() const` | Returns the root JSON object (may be `nullptr`). |
| `SetRoot` | `void SetRoot(JsonObjectPtr InRoot)` | Sets the root JSON object. |
| `CreateObject` | `static JsonObjectPtr CreateObject()` | Factory for `KJsonObject`. |
| `CreateArray` | `static JsonArrayPtr CreateArray()` | Factory for `KJsonArray`. |

#### String Escape/Unescape

The parser handles full JSON string escaping per the JSON specification:

- Standard escapes: `\"`, `\\`, `\b`, `\f`, `\n`, `\r`, `\t`
- Unicode escapes: `\uXXXX` (2-byte and 3-byte UTF-8 encoding)
- Control characters below `0x20` are serialized as `\uXXXX`

### Example: Building and Saving JSON

```cpp
#include "Serialization/JsonArchive.h"

void SaveGameConfig(const std::wstring& Path)
{
    KJsonArchive Archive;

    auto root = KJsonArchive::CreateObject();
    root->Set("windowWidth", 1920);
    root->Set("windowHeight", 1080);
    root->Set("fullscreen", false);
    root->Set("title", std::string("KojeomGameEngine"));

    auto resolution = KJsonArchive::CreateObject();
    resolution->Set("width", 1920);
    resolution->Set("height", 1080);
    resolution->Set("refreshRate", 144);
    root->Set("resolution", resolution);

    auto recentFiles = KJsonArchive::CreateArray();
    recentFiles->Add("scene_main.json");
    recentFiles->Add("scene_test.json");
    recentFiles->Add("scene_particles.json");
    root->Set("recentFiles", recentFiles);

    root->SetNull("lastSavedBy");

    Archive.SetRoot(root);
    Archive.SaveToFile(Path);
}
```

Output JSON:

```json
{
  "windowWidth": 1920,
  "windowHeight": 1080,
  "fullscreen": false,
  "title": "KojeomGameEngine",
  "resolution": {
    "width": 1920,
    "height": 1080,
    "refreshRate": 144
  },
  "recentFiles": [
    "scene_main.json",
    "scene_test.json",
    "scene_particles.json"
  ],
  "lastSavedBy": null
}
```

### Example: Loading and Reading JSON

```cpp
void LoadGameConfig(const std::wstring& Path)
{
    KJsonArchive Archive;
    if (!Archive.LoadFromFile(Path))
        return;

    auto root = Archive.GetRoot();
    if (!root)
        return;

    int width = root->GetInt("windowWidth", 1280);
    int height = root->GetInt("windowHeight", 720);
    bool fullscreen = root->GetBool("fullscreen");
    std::string title = root->GetString("title", "Untitled");

    auto resolution = root->GetObject("resolution");
    if (resolution)
    {
        int refreshRate = resolution->GetInt("refreshRate", 60);
    }

    auto recentFiles = root->GetArray("recentFiles");
    if (recentFiles)
    {
        for (size_t i = 0; i < recentFiles->Size(); ++i)
        {
            auto file = recentFiles->Get(i);
            if (file && file->IsString())
            {
                std::string path = file->AsString();
            }
        }
    }
}
```

### Example: In-Memory JSON Round-Trip

```cpp
void JsonRoundTrip()
{
    KJsonArchive Archive;

    auto root = KJsonArchive::CreateObject();
    root->Set("score", 100);
    root->Set("name", std::string("Player1"));

    std::string json = Archive.SerializeToString();
    // json = {"score":100,"name":"Player1"}

    KJsonArchive Archive2;
    Archive2.DeserializeFromString(json);
    auto loaded = Archive2.GetRoot();
    int score = loaded->GetInt("score");    // 100
    std::string name = loaded->GetString("name"); // "Player1"
}
```

---

## Scene Serialization

Scene save/load uses `KBinaryArchive` to serialize the full actor hierarchy. This system is implemented in `Engine/Scene/Actor.h` and `Engine/Scene/Actor.cpp`.

### Scene File Format

```
[uint32: SCENE_VERSION]         -- Currently 1 (constexpr in Actor.h)
[std::string: SceneName]        -- Scene name
[uint32: NumRootActors]         -- Count of top-level actors (no parent)
  For each root actor:
    [Actor serialization block] -- Recursive; includes children
```

### Actor Serialization Format

```
[std::string: ActorName]
[float: Position.x][float: Position.y][float: Position.z]
[float: Rotation.x][float: Rotation.y][float: Rotation.z][float: Rotation.w]
[float: Scale.x][float: Scale.y][float: Scale.z]
[bool: bVisible]
[uint32: NumComponents]
  For each component:
    [uint32: ComponentTypeID]    -- EComponentType enum value
    [Component-specific data]    -- Varies by component type
[uint32: NumChildren]
  For each child:
    [Child actor serialization block] -- Recursive
```

### Component Type IDs

```cpp
enum class EComponentType : uint32
{
    Base        = 0,
    StaticMesh  = 1,
    SkeletalMesh = 2,
    Water       = 3,
    Terrain     = 4,
    Light       = 5,
};
```

Components are created during deserialization via the factory function `CreateComponentByType()`.

### KSceneManager API

`KSceneManager` (in `Engine/Scene/SceneManager.h`) provides high-level scene persistence:

```cpp
std::shared_ptr<KScene> LoadScene(const std::wstring& Path);
HRESULT SaveScene(const std::wstring& Path, std::shared_ptr<KScene> Scene);
```

### Example: Saving a Scene

```cpp
void SaveCurrentScene(KSceneManager& Manager)
{
    auto scene = Manager.GetActiveScene();
    if (!scene)
        return;

    HRESULT hr = Manager.SaveScene(L"scenes/my_scene.bin", scene);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to save scene");
    }
}
```

### Example: Loading a Scene

```cpp
void LoadSceneFile(KSceneManager& Manager)
{
    auto scene = Manager.LoadScene(L"scenes/my_scene.bin");
    if (scene)
    {
        Manager.SetActiveScene(scene);
    }
}
```

### Example: Direct Scene Save/Load

```cpp
auto scene = std::make_shared<KScene>();
scene->SetName("MyLevel");

auto actor = scene->CreateActor("MainCharacter");
actor->SetWorldPosition({ 0.0f, 1.0f, 0.0f });

HRESULT hr = scene->Save(L"scenes/my_level.bin");
if (SUCCEEDED(hr))
{
    auto loadedScene = std::make_shared<KScene>();
    loadedScene->Load(L"scenes/my_level.bin");
}
```

---

## Serialization Guidelines

### Choosing Between Binary and JSON

| Aspect | KBinaryArchive | KJsonArchive |
|--------|---------------|--------------|
| Performance | Fast, minimal overhead | Slower, string parsing |
| File size | Compact | Larger (text format) |
| Human-readable | No | Yes |
| Use case | Scene data, game saves, asset bundles | Configuration, editor settings, metadata |
| Schema flexibility | Strict read/write order | Key-based access, order-independent |
| External dependencies | None | None |

### Best Practices

1. **Match read and write order** -- Binary archives are positional; the read sequence must exactly match the write sequence.
2. **Version your data** -- Always write a version header (like `SCENE_VERSION`) to support forward-compatible deserialization.
3. **Use typed getters with defaults** -- When reading JSON, always provide default values via the `Default` parameter on `GetInt()`, `GetFloat()`, `GetString()`, etc.
4. **Check return values** -- `Open()`, `LoadFromFile()`, and `SaveToFile()` return `bool`/`HRESULT`. Always check for errors.
5. **Close archives explicitly** -- In write mode, `Close()` flushes the buffer to disk. Forgetting to close will lose data.
6. **Raw writes require raw reads** -- Data written with `WriteRaw()` must be read with `ReadRaw()` using the same size.
