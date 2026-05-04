# KojeomGameEngine - Agent Work Instruction

## RULES

- 이전 세션에 이어서 작업을 시작하기 전에 반드시 context compaction을 진행하십시오.
- 이 프로세스는 비대화형 자율 모드로 실행 중입니다. 사용자가 결정을 내릴 수 없는 상태이므로, 귀하가 자율적으로 결정을 내려야 합니다.
- work 및 pipeline 경로에 있는 파일은 절대 수정하면 안됩니다. (CI/CD에서 사용하는 경로이므로 절대 수정을 해서는 안됩니다.)
- 사용자가 work\work_content.md 파일을 읽고 이 파일을 기준으로 작업을 진행하라는 명시적인 요청사항이 있지 않다면 이 파일에 대한 내용을 기준으로 작업을 진행하지 마십시오.
- 코딩 컨벤션, 안정성, 유지보수, 보안 가이드라인은 `docs/rules/AIAgentRules.md`를 참조하십시오.
- 반드시 작업 완료 후 컴파일 테스트 후 오리진에 반영하십시오.
- 현재 작업 환경(터미널, 운영체제)이 리눅스(우분투)/MacOS라면 CMake 및 현재 설치된 .Net sdk로 빌드 테스트를 진행하셔야합니다. 반대로 Windows라면 MSVC 및 .Net sdk로 진행하시면 됩니다.
만약 CMake 관련 내용이 없다면 현재 프로젝트 기준으로 필요한 파일을 셋업하십시오.
- 리눅스/윈도우즈 환경에서 게임 엔진은 동작해야합니다. 따라서, 윈도우즈의 경우는 directx11 기반 그리고, 리눅스의 경우 opengl 기반으로 동작할 수 있게 해야합니다. work/architecture 아래 문서를 참조하되, 이 규칙을 바탕으로 멀티플랫폼에서 동작할 수 있게 코드 아키텍처를 설정하십시오.

## ARCHITECTURE (우선순위 최상위)

구현 가이드는 반드시 `work/architecture/` 아래 문서를 참조하십시오. 이 문서의 지시가 다른 규칙과 충돌할 경우 이 문서를 우선합니다.

| 문서 | 내용 |
|------|------|
| `00_Overview.md` | 엔진 목표, 범위, 기술 선택, 코어 아키텍처 원칙 |
| `01_Runtime_Architecture.md` | 모듈 의존성, 실행 흐름, 월드/렌더러/에셋 구조 |
| `02_Feature_Roadmap.md` | Phase별 구현 순서, 완료 기준, 마일스톤 규칙 |
| `03_Testing_Quality_Maintenance.md` | 테스트 레이어, 품질 체계, AI Agent 유지보수 규칙 |
| `04_Tools_Editor_Strategy.md` | C#/C++ 도구 경계, 에디터 도입 시점, 데이터 포맷 전략 |
| `05_Renderable_Content_Features.md` | 스태틱 메시, 스켈레탈 메시, 지형, 애니메이션 구현 가이드 |
| `06_Agent_CLI_Test_Interface.md` | CLI 모드, exit code, JSON result, Agent 검증 워크플로우 |

## TASKS

작업 시 반드시 위 ARCHITECTURE 문서를 참조하여 구현하십시오.

- **렌더러**: `01_Runtime_Architecture.md`의 Renderer 섹션과 `05_Renderable_Content_Features.md`를 참조하여 파이프라인, 셰이더 품질, PBR 구현을 검토하고 개선하십시오.
- **모델 로딩**: `05_Renderable_Content_Features.md`의 Recommended Asset Format과 구현 순서를 참조하여 FBX/GLTF 로딩, 에셋 캐시, 머티리얼 임포트를 강화하십시오.
- **애니메이션**: `05_Renderable_Content_Features.md`의 Animation 섹션을 참조하여 Blend Tree, Root Motion, Animation Event를 구현하십시오.
- **에디터**: `04_Tools_Editor_Strategy.md`를 참조하여 씬 저장/로드, Play-In-Editor, 머티리얼 에디터를 구현하십시오.
- **CLI 테스트**: `06_Agent_CLI_Test_Interface.md`를 참조하여 Scene Dump, Asset Validation, Screenshot Comparison 모드를 구현하십시오.
- **안정성**: `03_Testing_Quality_Maintenance.md`를 참조하여 GitHub Actions CI/CD, 빌드 설정, 회귀 체크리스트를 점검하십시오.
- **보안**: `docs/rules/AIAgentRules.md`의 Security 섹션을 참조하여 경로 검증, 버퍼 바운드, 리소스 정리, 시크릿 스캔을 점검하십시오.
