#pragma once

#include "sor/sdl_game.hpp"
#include "sor/sdl_smartptr.hpp"

#include "player.hpp"
#include "enemy.hpp"
#include "magic/magic_system.hpp"
#include "magic/magic_debug_render.hpp"

// Mixer Header
#include <SDL3_mixer/SDL_mixer.h>

#include <chrono>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <memory>
#include <cstdint>
#include <filesystem>
#include <map>

// --- CMAKE OVERRIDE FIX ---
#ifdef BasePathFont
#undef BasePathFont
#endif
#define BasePathFont "font/"

#ifdef BasePathGraphic
#undef BasePathGraphic
#endif
#define BasePathGraphic "graphic/"

#ifdef BasePathAudio
#undef BasePathAudio
#endif
#define BasePathAudio "sound/"
// ---------------------------

#if __has_include("hsnr64/tiles.hpp")
    #include "hsnr64/tiles.hpp"
#endif

namespace JanSordid::SDL_Example {

}

namespace FD = JanSordid::FantasyDragon;

namespace JanSordid::SDL_Example
{
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

    enum class GameStateID : std::uint8_t {
        MainMenu = 0,
        Editor,
        Settings,
        Game
    };

    struct GlobalSettings {
        static int musicVolume;
        static int sfxVolume;
        static bool isFullscreen;
        static bool isEditorMode;
    };

    std::string GetAssetPath(const std::string& subPath);

    // =========================
    // EditorState
    // =========================
    class EditorState : public JanSordid::SDL::GameState<EditorGameBase>
    {
        using Base = JanSordid::SDL::GameState<EditorGameBase>;

        constexpr static Array<Color,8> BaseColors = {
            Color{ 0,0,0,255 }, Color{ 255,0,0,255 }, Color{ 0,255,0,255 }, Color{ 255,255,0,255 },
            Color{ 0,0,255,255 }, Color{ 255,0,255,255 }, Color{ 0,255,255,255 }, Color{ 255,255,255,255 },
        };

    public:
        struct FireballProjectile
        {
            FPoint pos{};
            FPoint startPos{};
            FPoint vel{};
            float  z = 0.0f;
            float  radius = 10.0f;
            float  lifetime = 1.2f;
            float  animTime = 0.0f;
            bool   alive = true;
        };
        struct ExplosionAnim
        {
            FPoint pos{};
            float  animTime = 0.0f;
            bool   alive = true;
        };
        struct HealAnim
        {
            FPoint pos{};
            float  animTime = 0.0f;
            bool   alive = true;
        };

        using Base::Base;
        void Init() override;
        void Destroy() override;
        bool Input( const Event & event ) override;
        void Update( u64 framesSinceStart, Duration timeSinceStart, f32 deltaT ) override;
        void Render( u64 framesSinceStart, Duration timeSinceStart, f32 deltaTNeeded ) override;
        constexpr Color clearColor() const noexcept override { return Color{ 100, 100, 100, 255 }; }
        void LoadMapFromFileAndRebuild(const char* path);

        void PlaySFX(const std::string& name, int loops = 0);

        using WorldState = Array<MapType, 3>;

    private:
        Owned<Font>    _font;
        Owned<Texture> _tileSet;

        Owned<Texture> _texFireball;
        Owned<Texture> _texExplosion;
        Owned<Texture> _texHeal;
        Owned<Texture> _uiFrame;
        Owned<Texture> _uiHpFill;
        Owned<Texture> _uiManaFill;
        Owned<Texture> _enemyHpFrame;
        Owned<Texture> _enemyHpFill;

        std::map<std::string, Mix_Chunk*> _sfx;
        int _beeChannel = -1;

        const bool _doGenerateEmptyMap = true;
        WorldState _worldState1;
        WorldState _worldState2;
        WorldState* _currState = &_worldState1;
        WorldState* _nextState = &_worldState2;
        std::string _currentGameMapPath;

        int _activeLayer = 0;

        Point  _tileSetSize;
        Point  _tileSize;
        Point  _tileCount;
        FPoint _camera;

        Player _player;
        std::vector<Bee> _bees;
        std::vector<Mushroom> _mushrooms;

        FD::Magic::MagicSystem _magic;

        std::vector<FireballProjectile> _fireballs;
        std::vector<ExplosionAnim> _explosions;
        std::vector<HealAnim> _heals;
        FD::Magic::MagicResult _debugTemplate = FD::Magic::MagicResult::Fireball;

        Point  _pickedIdx          = Point{ 0, 0 };
        Point  _pickedSize         = Point{ 1, 1 };
        Point  _selectionStart     = Point{ 0, 0 };
        bool   _isSelectingPalette = false;
        bool   _flipH = false;
        bool   _flipV = false;
        int    _rotSteps = 0;

        i32    _mapScale     = 2;
        i32    _paletteScale = 1;
        bool   _isPainting   = false;
        bool   _isPanning    = false;
        bool   _showGrid     = false;
        bool   _showPalette  = false;

        constexpr static Duration UpdateDeltaTime = 16ms;
        Duration _nextUpdateTime = {};

        FRect _chestHitbox;
        bool _levelFinished = false;
        float _finishTimer = 0.0f;
        float _manaRegenAccu = 0.0f;

        void ResetLevel();
    };

    // =========================
    // MainMenuState
    // =========================
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

    // =========================
    // SettingsState
    // =========================
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
        bool DrawSlider(const char* label, float y, float mouseX, float mouseY, bool isMouseDown, int& volumeRef);
    };
}
