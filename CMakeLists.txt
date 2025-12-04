cmake_minimum_required( VERSION 3.14 )

# Unused
#list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" )

project( sdl_base LANGUAGES CXX )

set( CMAKE_C_STANDARD               17  )
set( CMAKE_CXX_STANDARD             20  )
set( CMAKE_CXX_STANDARD_REQUIRED    ON  )
set( CMAKE_CXX_EXTENSIONS           OFF )

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    set( IS_DEBUG TRUE )
else()
    set( IS_DEBUG FALSE )
endif()

option( USE_DeployBinary    "Deploy the binary in the base directory"    OFF )
option( USE_FMT_HeaderOnly  "Use header-only implementation of FMT"      ON  )
option( USE_NFD_Native      "Use native encoding instead of UTF8 in NFD" OFF )
option( USE_TGUI            "Use Texus' GUI"          ON          )
option( USE_ImGui           "Use Dear ImGui"          ${IS_DEBUG} )
option( USE_ImGuiFreetype   "Use Dear ImGui Freetype" OFF         )
option( USE_OpenMP          "Use OpenMP"              ON          )
option( USE_RTTI            "Use RTTI"                ON          )
option( USE_Exceptions      "Use Exceptions"          ON          )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

find_package( SDL3          CONFIG REQUIRED )
find_package( SDL3_image    CONFIG REQUIRED )
find_package( SDL3_ttf      CONFIG REQUIRED )
find_package( SDL3_mixer    CONFIG REQUIRED )
find_package( fmt           CONFIG REQUIRED )
find_package( nfd           CONFIG REQUIRED )
find_package( benchmark     CONFIG REQUIRED )
find_package( Eigen3        CONFIG REQUIRED )
find_package( nlohmann_json CONFIG REQUIRED )
if( USE_TGUI )
    find_package( TGUI      CONFIG REQUIRED )
endif()
if( USE_ImGui )
    find_package( imgui     CONFIG REQUIRED )
    find_package( implot    CONFIG REQUIRED )
endif()
if( USE_OpenMP )
    find_package( OpenMP           REQUIRED )
endif()

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    set( CLANG 1 )
elseif( CMAKE_CXX_COMPILER_ID MATCHES "GNU" )
    set( GCC 1 )
endif()

add_compile_definitions(
        $<$<CONFIG:DEBUG>:_DEBUG>
        $<$<CONFIG:RELWITHDEBINFO>:OPTIMIZED>
        $<$<CONFIG:RELEASE>:FINAL> )

if( USE_DeployBinary )
    add_compile_definitions( DEPLOY_BINARY  )
endif()

if( USE_NFD_Native )
    add_compile_definitions( NFD_NATIVE     )
endif()

if( USE_ImGui )
    add_compile_definitions( IMGUI          )
    if( USE_ImGuiFreetype )
        add_compile_definitions( IMGUI_ENABLE_FREETYPE )
    endif()
endif()

if( WIN32 )
    add_compile_definitions( NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 )
endif()

if( GCC OR CLANG )
    add_compile_definitions( _GLIBCXX_USE_DEPRECATED=0 _LIBCPP_ENABLE_DEPRECATION_WARNINGS=1 )
    add_compile_options( -Wall -Wextra -Werror=parentheses -Werror=return-type )
endif()

if( NOT USE_RTTI )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> )
endif()

if( NOT USE_Exceptions )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions> )
endif()

if( USE_DeployBinary )
    add_compile_options( -Werror=unused-variable )
endif()

if( NOT USE_OpenMP )
    add_compile_options( -Wno-unknown-pragmas )
endif()

if( MaximumOptimize )
    add_compile_options( -march=native )
endif()

# --- OTHER TARGETS ---
add_executable( sor-pong src/sor/core.cpp src/sor/sdl_core.cpp src/example/pong.cpp )
target_include_directories( sor-pong PUBLIC src/ )
target_link_libraries( sor-pong PUBLIC $<IF:$<BOOL:${USE_FMT_HeaderOnly}>, fmt::fmt-header-only, fmt::fmt> $<TARGET_NAME_IF_EXISTS:SDL3::SDL3main> $<IF:$<TARGET_EXISTS:SDL3::SDL3>, SDL3::SDL3, SDL3::SDL3-static> $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image>, SDL3_image::SDL3_image, SDL3_image::SDL3_image-static> )

add_executable( sor-smartptr src/sor/core.cpp src/sor/sdl_core.cpp src/example/smartptrtest.cpp )
target_include_directories( sor-smartptr PUBLIC src/ )
target_link_libraries( sor-smartptr PUBLIC $<IF:$<BOOL:${USE_FMT_HeaderOnly}>, fmt::fmt-header-only, fmt::fmt> $<TARGET_NAME_IF_EXISTS:nfd::nfd> $<TARGET_NAME_IF_EXISTS:SDL3::SDL3main> $<IF:$<TARGET_EXISTS:SDL3::SDL3>, SDL3::SDL3, SDL3::SDL3-static> $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image>, SDL3_image::SDL3_image, SDL3_image::SDL3_image-static> )

add_executable( sor-mini src/sor/core.cpp src/sor/sdl_core.cpp src/sor/sdl_game.cpp src/example/mini.cpp )
target_include_directories( sor-mini PUBLIC src/ )
target_link_libraries( sor-mini PUBLIC $<TARGET_NAME_IF_EXISTS:OpenMP::OpenMP_CXX> $<IF:$<BOOL:${USE_FMT_HeaderOnly}>, fmt::fmt-header-only, fmt::fmt> $<TARGET_NAME_IF_EXISTS:nfd::nfd> $<TARGET_NAME_IF_EXISTS:SDL3::SDL3main> $<IF:$<TARGET_EXISTS:SDL3::SDL3>, SDL3::SDL3, SDL3::SDL3-static> $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image>, SDL3_image::SDL3_image, SDL3_image::SDL3_image-static> $<IF:$<TARGET_EXISTS:SDL3_ttf::SDL3_ttf>, SDL3_ttf::SDL3_ttf, SDL3_ttf::SDL3_ttf-static> $<IF:$<TARGET_EXISTS:SDL3_mixer::SDL3_mixer>, SDL3_mixer::SDL3_mixer, SDL3_mixer::SDL3_mixer-static> $<TARGET_NAME_IF_EXISTS:imgui::imgui> $<TARGET_NAME_IF_EXISTS:implot::implot> )

add_executable( sor-example src/sor/core.cpp src/sor/sdl_core.cpp src/sor/sdl_game.cpp src/hsnr64/tilefont.cpp src/hsnr64/tiles.cpp src/example/game/example_game.cpp src/example/game/introstate.cpp src/example/game/plasmastate.cpp src/example/game/sortstate.cpp src/example/game/camerastate.cpp src/example/game/shooterstate.cpp src/example/game/simpleeditor.cpp src/example/game/roflstate.cpp src/example/game/mapeditor.cpp src/example/game/tgui.cpp )
target_include_directories( sor-example PUBLIC src/ )
target_link_libraries( sor-example PUBLIC $<IF:$<BOOL:${USE_FMT_HeaderOnly}>, fmt::fmt-header-only, fmt::fmt> $<TARGET_NAME_IF_EXISTS:OpenMP::OpenMP_CXX> $<TARGET_NAME_IF_EXISTS:nfd::nfd> $<TARGET_NAME_IF_EXISTS:SDL3::SDL3main> $<IF:$<TARGET_EXISTS:SDL3::SDL3>, SDL3::SDL3, SDL3::SDL3-static> $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image>, SDL3_image::SDL3_image, SDL3_image::SDL3_image-static> $<IF:$<TARGET_EXISTS:SDL3_ttf::SDL3_ttf>, SDL3_ttf::SDL3_ttf, SDL3_ttf::SDL3_ttf-static> $<IF:$<TARGET_EXISTS:SDL3_mixer::SDL3_mixer>, SDL3_mixer::SDL3_mixer, SDL3_mixer::SDL3_mixer-static> $<TARGET_NAME_IF_EXISTS:Eigen3::Eigen> $<TARGET_NAME_IF_EXISTS:TGUI::TGUI> $<TARGET_NAME_IF_EXISTS:imgui::imgui> $<TARGET_NAME_IF_EXISTS:implot::implot> PRIVATE $<TARGET_NAME_IF_EXISTS:nlohmann_json::nlohmann_json> $<TARGET_NAME_IF_EXISTS:OpenMP::OpenMP_CXX> )


# -----------------------------------------------------------------------------
# FANTASY DRAGON CONFIGURATION (BEREINIGT)
# -----------------------------------------------------------------------------
add_executable(fantasydragon
        # ENGINE
        src/sor/core.cpp
        src/sor/sdl_core.cpp
        src/sor/sdl_game.cpp

        # DEIN NEUER EINSTIEGSPUNKT (stellt sicher, dass die neue main.cpp genutzt wird)
        src/FantasyDragon/main.cpp

        # DEIN EDITOR CODE
        src/FantasyDragon/fantasydragon_mapeditor.cpp

        # HELFER (Schriftarten und Kacheln für den Editor)
        src/hsnr64/tilefont.cpp
        src/hsnr64/tiles.cpp
)

# Wir sagen CMake, wo es Header-Dateien suchen soll.
# "src/example/game/" lassen wir drin, falls dein Editor Dinge wie "core.hpp" von dort erwartet,
# aber wir kompilieren keine .cpp Dateien von dort.
target_include_directories( fantasydragon PUBLIC
        src/
        src/example/game/
)

target_link_libraries(      fantasydragon PUBLIC
        $<IF:$<BOOL:${USE_FMT_HeaderOnly}>, fmt::fmt-header-only, fmt::fmt>
        $<TARGET_NAME_IF_EXISTS:nfd::nfd>
        $<TARGET_NAME_IF_EXISTS:SDL3::SDL3main>

        $<IF:$<TARGET_EXISTS:SDL3::SDL3>,               SDL3::SDL3,             SDL3::SDL3-static>
        $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image>,   SDL3_image::SDL3_image, SDL3_image::SDL3_image-static>
        $<IF:$<TARGET_EXISTS:SDL3_ttf::SDL3_ttf>,       SDL3_ttf::SDL3_ttf,     SDL3_ttf::SDL3_ttf-static>
        $<IF:$<TARGET_EXISTS:SDL3_mixer::SDL3_mixer>,   SDL3_mixer::SDL3_mixer, SDL3_mixer::SDL3_mixer-static>

        $<TARGET_NAME_IF_EXISTS:imgui::imgui>
        $<TARGET_NAME_IF_EXISTS:implot::implot>
        $<TARGET_NAME_IF_EXISTS:TGUI::TGUI>
        $<TARGET_NAME_IF_EXISTS:OpenMP::OpenMP_CXX>
)