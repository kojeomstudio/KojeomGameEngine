# Claude AI Agent Rules for KojeomGameEngine

## Project Overview

KojeomGameEngine is a C++ game engine built with DirectX 11. Follow these rules when working on this project.

## Code Style

### Naming Conventions
- Classes: `K` prefix (e.g., `KRenderer`, `KGraphicsDevice`)
- Structs: `F` prefix (e.g., `FRenderObject`, `FDirectionalLight`)
- Functions: PascalCase (e.g., `Initialize()`, `BeginFrame()`)
- Variables: camelCase (e.g., `graphicsDevice`, `currentCamera`)
- Member Variables: camelCase with `b` prefix for booleans
- Constants: PascalCase in namespace

### DirectX 11 Guidelines
- Always use `Microsoft::WRL::ComPtr` for COM objects
- Always check HRESULT return values
- Constant buffers must be 16-byte aligned
- Use Shader Model 5.0 (`vs_5_0`, `ps_5_0`)

## Project Structure

```
Engine/
├── Core/           # Engine core
├── Graphics/       # Rendering components
├── Input/          # Input management
├── Serialization/  # Serialization system
├── UI/             # UI components
└── Utils/          # Utility classes

Editor/
├── KojeomEditor/   # WPF-based editor
```

## Documentation

- Rules: `docs/rules/`
- Plans: `docs/plans/`
- All documentation in Markdown format

## Git Commit Format

```
[Category] Brief description

- List of changes
```

Categories: `[Core]`, `[Graphics]`, `[Docs]`, `[Build]`, `[Refactor]`, `[Fix]`, `[Feature]`

## Important Rules

1. Read existing code before making changes
2. Follow existing naming conventions
3. Test changes before marking work complete
4. Keep changes focused - one feature/fix per commit
5. Use smart pointers for automatic memory management
6. Delete copy constructor/assignment for resource-owning classes

## Prohibited Actions

- Do not modify third-party library code
- Do not commit debug/test code to main branch
- Do not break existing API without updating all usages
- Do not use raw pointers for owning resources
