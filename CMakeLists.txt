cmake_minimum_required(VERSION 3.21)

project(Classbound
	VERSION 1.0.0.0
	DESCRIPTION "Skyrim Class Framework"
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# 1. fmt (needed by spdlog)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

# 2. spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.11.0
)
set(SPDLOG_FMT_EXTERNAL ON)
FetchContent_MakeAvailable(spdlog)

# 3. ImGui (for Menu Framework)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
endif()

# 5. CommonLibSSE-NG (Local source)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(COMMONLIBSSE_NG_DIR "" CACHE PATH "Path to a CommonLibSSE-NG checkout")
if(NOT COMMONLIBSSE_NG_DIR)
    message(FATAL_ERROR "Set COMMONLIBSSE_NG_DIR to your CommonLibSSE-NG checkout")
endif()
add_subdirectory("${COMMONLIBSSE_NG_DIR}" CommonLibSSE-NG EXCLUDE_FROM_ALL)

# Core Plugin Library
add_library(${PROJECT_NAME} SHARED
	src/Main.cpp
	src/ClassManager.cpp
	src/ClassRegistry.cpp
	src/SaveManager.cpp
	src/UIManager.cpp
	src/EventManager.cpp
	src/XPManager.cpp
	src/EffectManager.cpp
	src/SkillManager.cpp
	src/UniversalSkillManager.cpp
	src/SettingsManager.cpp
	src/ClassChangeManager.cpp
	src/GlobalCharacterDB.cpp
	src/FollowerManager.cpp
	src/AffinityManager.cpp
	src/StartingGearManager.cpp
	src/KingCoreAdapter.cpp
	src/PrismaUIManager.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    CommonLibSSE
    spdlog::spdlog
    imgui
    winmm
    mfplat
    mfreadwrite
    mfuuid
    ole32
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set(KINGCORE_INCLUDE_DIR "" CACHE PATH "Path to King Core headers")
if(NOT KINGCORE_INCLUDE_DIR)
    message(FATAL_ERROR "Set KINGCORE_INCLUDE_DIR to the King Core include directory")
endif()
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    "${KINGCORE_INCLUDE_DIR}"
)
target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX- /EHsc /permissive->
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	SKSE_SUPPORT_XBYAK=1
	NOMINMAX
	WIN32_LEAN_AND_MEAN
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
