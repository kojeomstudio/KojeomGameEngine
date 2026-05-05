cmake_minimum_required(VERSION 3.20)
project(KojeomEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXTRA_INC_DIR "${CMAKE_SOURCE_DIR}/_ext_include")
file(MAKE_DIRECTORY ${EXTRA_INC_DIR})
if(EXISTS /tmp/pkg/usr/include)
    file(COPY /tmp/pkg/usr/include/ DESTINATION ${EXTRA_INC_DIR})
endif()
include_directories(BEFORE ${EXTRA_INC_DIR})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -isystem ${EXTRA_INC_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${EXTRA_INC_DIR}")
set(CMAKE_REQUIRED_INCLUDES "${EXTRA_INC_DIR}")

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})

include(FetchContent)

set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")

# --- GLFW ---
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.9
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

# --- GLAD (OpenGL loader) ---
FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.6
    SOURCE_SUBDIR  cmake
)
set(GLAD_INSTALL OFF CACHE BOOL "" FORCE)
set(GLAD_REPRODUCIBLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glad)

glad_add_library(glad_gl45_core STATIC API gl:core=4.5)

# --- glm ---
find_package(glm QUIET)
if(NOT glm_FOUND)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

# --- nlohmann_json ---
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
    )
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(json)
endif()

# --- stb (header-only, use tinygltf's bundled stb) ---
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# --- assimp ---
find_package(assimp QUIET)
if(NOT assimp_FOUND)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG        v5.4.2
    )
    FetchContent_MakeAvailable(assimp)
endif()

# --- spdlog ---
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# --- tinygltf ---
FetchContent_Declare(tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.8.23
)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tinygltf)

# ---- Engine Static Library ----
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/App/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Math/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/World/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Renderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Animation/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Game/*.cpp"
)

add_library(KojeomEngineLib STATIC ${ENGINE_SOURCES})
target_include_directories(KojeomEngineLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${stb_SOURCE_DIR}
)
target_link_libraries(KojeomEngineLib PUBLIC
    glfw
    glad_gl45_core
    glm::glm
    nlohmann_json::nlohmann_json
    assimp::assimp
    spdlog::spdlog
    tinygltf
    ${CMAKE_DL_LIBS}
)
target_compile_definitions(KojeomEngineLib PUBLIC
    GLM_ENABLE_EXPERIMENTAL
    GLFW_INCLUDE_NONE
    $<$<CONFIG:Debug>:KOJEOM_DEBUG>
    $<$<CONFIG:Release>:KOJEOM_RELEASE>
)

# ---- Main Executable ----
add_executable(Simple3DEngine "${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp")
target_link_libraries(Simple3DEngine PRIVATE KojeomEngineLib)
set_target_properties(Simple3DEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)
