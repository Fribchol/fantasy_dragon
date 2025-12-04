#pragma once

#include "sor/sdl_game.hpp"
#include "sor/sdl_smartptr.hpp"
#include <chrono>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <memory> // WICHTIG für std::unique_ptr

// Fallback Pfade
#ifndef BasePathFont
#define BasePathFont "assets/fonts/"
#endif
#ifndef BasePathGraphic
#define BasePathGraphic "assets/graphics/"
#endif

// HSNR64 Header Fallback
#if __has_include("hsnr64/tiles.hpp")
    #include "hsnr64/tiles.hpp"
#endif

namespace JanSordid::SDL_Example
{
    // =========================================================
    // REPARATUR-BLOCK: Typen manuell definieren
    // =========================================================
    using namespace std::chrono_literals;

    // KORREKTUR: Wir definieren Owned selbst, da es in Core fehlt
    template <typename T>
    using Owned = std::unique_ptr<T>;

    // Array definieren
    template <typename T, std::size_t N>
    using Array = std::array<T, N>;

    using i32 = int;
    using f32 = float;
    using u64 = unsigned long long;
    using Duration = std::chrono::nanoseconds;

    // SDL Typen importieren
    using JanSordid::SDL::Color;
    using JanSordid::SDL::Font;
    using JanSordid::SDL::Texture;
    using JanSordid::SDL::Point;
    using JanSordid::SDL::FPoint;
    using JanSordid::SDL::FRect;
    using JanSordid::SDL::Event;

    using EditorGameBase = JanSordid::SDL::Game<>;

    // =========================================================

    class EditorState : public JanSordid::SDL::GameState<EditorGameBase>
    {
        using Base = JanSordid::SDL::GameState<EditorGameBase>;

        constexpr static Array<Color,8> BaseColors = {
            Color{ 0,   0,   0,   255 },
            Color{ 255, 0,   0,   255 },
            Color{ 0,   255, 0,   255 },
            Color{ 255, 255, 0,   255 },
            Color{ 0,   0,   255, 255 },
            Color{ 255, 0,   255, 255 },
            Color{ 0,   255, 255, 255 },
            Color{ 255, 255, 255, 255 },
        };

        Owned<Font>    _font;
        Owned<Texture> _tileSet;

        using WorldState = Array<Array<int, 40>, 20>;

        const bool _doGenerateEmptyMap = true;
        WorldState _worldState1;
        WorldState _worldState2;

        WorldState
            * _currState = &_worldState1,
            * _nextState = &_worldState2;

        Point  _tileSetSize;
        Point  _tileSize;
        Point  _tileCount;
        FPoint _camera;
        Point  _pickedIdx    = Point{ 0, 0 };
        i32    _mapScale     = 2;
        i32    _paletteScale = 1;
        bool   _isPainting   = false;
        bool   _isPanning    = false;
        bool   _showGrid     = false;

        // Variable für TAB-Taste (Palette anzeigen)
        bool   _showPalette  = false;

        constexpr static Duration UpdateDeltaTime = 100ms;

        Duration _nextUpdateTime = {};

    public:
        using Base::Base;

        void Init() override;
        void Destroy() override;

        bool Input( const Event & event ) override;
        void Update( u64 framesSinceStart, Duration timeSinceStart, f32 deltaT       ) override;
        void Render( u64 framesSinceStart, Duration timeSinceStart, f32 deltaTNeeded ) override;

        constexpr Color clearColor() const noexcept override
        {
             return Color{ 100, 100, 100, 255 };
        }
    };
}