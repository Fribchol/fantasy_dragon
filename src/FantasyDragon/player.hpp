#pragma once

#include "sor/sdl_game.hpp"
#include "sor/sdl_smartptr.hpp"
#include <array>
#include <memory>
#include <cstdint>

#include "hsnr64/tiles.hpp"

namespace JanSordid::SDL_Example
{
    constexpr int MAP_WIDTH = 480;
    constexpr int MAP_HEIGHT = 20;
    using MapType = std::array<std::array<int, MAP_WIDTH>, MAP_HEIGHT>;

    // Wir definieren WorldState hier, damit der Player die 3 Layer kennt
    using WorldState = std::array<MapType, 3>;

    using u8  = std::uint8_t;
    using f32 = float;

    using JanSordid::SDL::Texture;
    using JanSordid::SDL::FPoint;
    using JanSordid::SDL::FRect;
    using JanSordid::SDL::Event;

    template <typename T> using Owned = std::unique_ptr<T>;

    enum class PlayerAnim : int {
        Idle = 0,
        Run = 1,
        Jump = 2,
        Attack1 = 4,
        Attack2 = 5,
        Attack3 = 6,
        Crouch = 7
    };

    struct Player {
        FPoint position = { 100.0f, 150.0f };
        FPoint velocity = { 0.0f, 0.0f };

        float z = 0.0f;
        float velZ = 0.0f;

        int hp = 100;
        float hitTimer = 0.0f;

        FPoint size = { 16.0f, 16.0f };
        FPoint spriteOffset = { -17.0f, -28.0f };

        bool facingRight = true;
        bool isAttacking = false;

        PlayerAnim currentAnim = PlayerAnim::Idle;
        float animTimer = 0.0f;
        int currentFrame = 0;

        Owned<Texture> spriteSheet;
        Owned<Texture> shadowTexture;

        void Init(SDL_Renderer* renderer);
        // Geändert: Nimmt jetzt WorldState (alle Layer)
        void Update(float dt, const WorldState& world);
        void Input(const Event& evt);
        void Render(SDL_Renderer* renderer, FPoint camera, int scale, bool healTint = false);

        void TakeDamage(int amount);
        FRect GetAttackHitbox() const;

    private:
        // Geändert: Prüft Kollision gegen eine spezifische Map (den Collision Layer)
        bool CheckCollision(const FRect& rect, const MapType& map);
    };
}
