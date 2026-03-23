# KojeomGameEngine Documentation

Welcome to the KojeomGameEngine documentation. This directory contains all technical documentation, development plans, and guidelines.

## Directory Structure

```
docs/
├── README.md                        # This file
├── renderer/                        # Renderer documentation
│   └── DirectX11Renderer.md        # DirectX 11 renderer technical docs
├── rules/                           # AI agent rules and guidelines
│   └── AIAgentRules.md             # Rules for AI agents working on the project
└── plans/                           # Development plans and progress tracking
    └── RendererDevelopmentPlan.md  # Renderer enhancement roadmap
```

## Quick Links

### For Developers

- [DirectX 11 Renderer Documentation](renderer/DirectX11Renderer.md) - Technical details about the rendering system
- [Renderer Development Plan](plans/RendererDevelopmentPlan.md) - Future development roadmap

### For AI Agents

- [AI Agent Rules](rules/AIAgentRules.md) - **Required reading** for AI agents working on this project
- `.gemini/GEMINI.md` - Gemini-specific rules
- `.claude/CLAUDE.md` - Claude-specific rules

## Documentation Categories

### Technical Documentation (`docs/`)

General technical documentation about the engine:

- Architecture overviews
- API references
- Integration guides
- Performance considerations

### Rules and Guidelines (`docs/rules/`)

Documents that define standards and practices:

- Code style guidelines
- Naming conventions
- AI agent instructions
- Review checklists

### Development Plans (`docs/plans/`)

Work planning and progress tracking:

- Feature roadmaps
- Task breakdowns
- Commit tracking with hashes
- Progress status

## Contributing

When adding new documentation:

1. **Technical docs** → `docs/` or appropriate subdirectory
2. **Rules/guidelines** → `docs/rules/`
3. **Work plans** → `docs/plans/` (include commit hash tracking)

## Document Standards

- Use Markdown format for all documentation
- Include clear headings and table of contents for long documents
- Keep documents up-to-date with code changes
- Use relative links for internal references

## Engine Overview

KojeomGameEngine is a C++ game engine featuring:

- **DirectX 11** rendering backend
- **Modular architecture** with clear separation of concerns
- **Modern C++** practices (smart pointers, RAII)
- **Component-based** design

### Core Components

| Component | Description |
|-----------|-------------|
| KGraphicsDevice | DirectX 11 device and swap chain management |
| KRenderer | Main rendering pipeline |
| KCamera | View and projection matrix management |
| KShaderProgram | Shader compilation and management |
| KMesh | Geometry and vertex/index buffers |
| KTexture | Texture loading and sampling |

## Getting Started

1. Clone the repository
2. Open `KojeomEngine.sln` in Visual Studio 2019 or later
3. Build the solution (requires Windows SDK with DirectX 11)
4. Run one of the examples in the `Examples/` directory

## License

See [LICENSE](../LICENSE) for license information.
