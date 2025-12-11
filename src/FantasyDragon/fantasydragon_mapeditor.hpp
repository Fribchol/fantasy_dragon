#pragma once

#include "sor/sdl_game.hpp"
#include "sor/sdl_smartptr.hpp"

// WICHTIG: Hier binden wir den Spieler ein!
#include "player.hpp"

// Einbindung Gegner
#include "enemy.hpp"

// WICHTIG: Mixer Header
#include <SDL3_mixer/SDL_mixer.h>

#include <chrono>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <memory>
#include <cstdint>
#include <filesystem> // WICHTIG für Pfadsuche

// Fallback Pfade (KORRIGIERT auf Einzahl laut Screenshot)
#ifndef BasePathFont
#define BasePathFont "asset/font/"
#endif
#ifndef BasePathGraphic
#define BasePathGraphic "asset/graphic/"
#endif
#ifndef BasePathAudio
#define BasePathAudio "asset/sound/"
#endif

// HSNR64 Header Fallback
#if __has_include("hsnr64/tiles.hpp")
    #include "hsnr64/tiles.hpp"
#endif

namespace JanSordid::SDL_Example
{
    // =========================================================
    // Typ-Definitionen
    // =========================================================
    using namespace std::chrono_literals;
    namespace fs = std::filesystem;

    template <typename T> using Owned = std::unique_ptr<T>;
    template <typename T, std::size_t N> using Array = std::array<T, N>;

    using u8  = std::uint8_t;
    using i32 = int;
    using f32 = float;
    using u64 = unsigned long long;
    using Duration = std::chrono::nanoseconds;

    using JanSordid::SDL::Color;
    using JanSordid::SDL::Font;
    using JanSordid::SDL::Texture;
    using JanSordid::SDL::Point;
    using JanSordid::SDL::FPoint;
    using JanSordid::SDL::FRect;
    using JanSordid::SDL::Event;

    using EditorGameBase = JanSordid::SDL::Game<>;

    // IDs für alle Zustände im Spiel
    enum class GameStateID : std::uint8_t {
        MainMenu = 0,
        Editor,
        Settings,
        Game
    };

    // Globale Settings
    struct GlobalSettings {
        static int musicVolume;
        static bool isFullscreen;
        static bool isEditorMode;
    };

    // Hilfsfunktion Deklaration
    std::string GetAssetPath(const std::string& subPath);

    // =========================================================
    // KLASSE: Editor
    // =========================================================
    class EditorState : public JanSordid::SDL::GameState<EditorGameBase>
    {
        using Base = JanSordid::SDL::GameState<EditorGameBase>;

        constexpr static Array<Color,8> BaseColors = {
            Color{ 0,0,0,255 }, Color{ 255,0,0,255 }, Color{ 0,255,0,255 }, Color{ 255,255,0,255 },
            Color{ 0,0,255,255 }, Color{ 255,0,255,255 }, Color{ 0,255,255,255 }, Color{ 255,255,255,255 },
        };

        Owned<Font>    _font;
        Owned<Texture> _tileSet;

        using WorldState = MapType;

        const bool _doGenerateEmptyMap = true;
        WorldState _worldState1;
        WorldState _worldState2;
        WorldState *_currState = &_worldState1, *_nextState = &_worldState2;

        Point  _tileSetSize;
        Point  _tileSize;
        Point  _tileCount;
        FPoint _camera;

        Player _player;
        Bee _bee;

        Point  _pickedIdx          = Point{ 0, 0 };
        Point  _pickedSize         = Point{ 1, 1 };
        Point  _selectionStart     = Point{ 0, 0 };
        bool   _isSelectingPalette = false;

        i32    _mapScale     = 2;
        i32    _paletteScale = 1;
        bool   _isPainting   = false;
        bool   _isPanning    = false;
        bool   _showGrid     = false;
        bool   _showPalette  = false;

        constexpr static Duration UpdateDeltaTime = 16ms;
        Duration _nextUpdateTime = {};

    public:
        using Base::Base;
        void Init() override;
        void Destroy() override;
        bool Input( const Event & event ) override;
        void Update( u64 framesSinceStart, Duration timeSinceStart, f32 deltaT ) override;
        void Render( u64 framesSinceStart, Duration timeSinceStart, f32 deltaTNeeded ) override;
        constexpr Color clearColor() const noexcept override { return Color{ 100, 100, 100, 255 }; }
    };

    // =========================================================
    // KLASSE: Hauptmenü
    // =========================================================
    class MainMenuState : public JanSordid::SDL::GameState<EditorGameBase>
    {
        using Base = JanSordid::SDL::GameState<EditorGameBase>;
        Owned<Font> _fontTitle;
        Owned<Font> _fontMenu;
        Owned<Texture> _background;

        Mix_Music* _bgMusic = nullptr;

    public:
        using Base::Base;
        void Init() override;
        void Destroy() override;
        bool Input( const Event & event ) override;
        void Update( u64, Duration, f32 ) override {}
        void Render( u64, Duration, f32 ) override;
        constexpr Color clearColor() const noexcept override { return Color{ 30, 30, 40, 255 }; }

    private:
        bool DrawButton(const char* text, float y, float mouseX, float mouseY, bool isClicked);
    };

    // =========================================================
    // KLASSE: Settings
    // =========================================================
    class SettingsState : public JanSordid::SDL::GameState<EditorGameBase>
    {
        using Base = JanSordid::SDL::GameState<EditorGameBase>;
        Owned<Font> _font;
        Owned<Texture> _background;

    public:
        using Base::Base;
        void Init() override;
        bool Input( const Event & event ) override;
        void Update( u64, Duration, f32 ) override {}
        void Render( u64, Duration, f32 ) override;
        constexpr Color clearColor() const noexcept override { return Color{ 40, 30, 30, 255 }; }

    private:
        bool DrawButton(const char* text, float y, float mouseX, float mouseY, bool isClicked);
        bool DrawSlider(const char* label, float y, float mouseX, float mouseY, bool isMouseDown);
    };
}