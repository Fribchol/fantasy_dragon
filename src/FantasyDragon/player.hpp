#pragma once

#include "sor/sdl_game.hpp"
#include "sor/sdl_smartptr.hpp"
#include <array>
#include <memory>
#include <cstdint>

// --- FIX: WIR HOLEN DIE ECHTE DEFINITION (160) ---
#include "hsnr64/tiles.hpp"

namespace JanSordid::SDL_Example
{
    // --- FIX: KEINE HARTE ZAHL MEHR ---
    // Wir sagen: "MapType ist das, was in tiles.hpp steht (JanSordid::HSNR64::MapType)"
    // Damit ist es automatisch 160 breit!
    using MapType = JanSordid::HSNR64::MapType;

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

        // --- KAMPF STATS ---
        int hp = 100;
        float hitTimer = 0.0f; // Für Unverwundbarkeit
        // -------------------

        FPoint size = { 16.0f, 16.0f };
        FPoint spriteOffset = { -17.0f, -28.0f };

        bool facingRight = true;
        bool isAttacking = false;

        PlayerAnim currentAnim = PlayerAnim::Idle;
        float animTimer = 0.0f;
        int currentFrame = 0;

        Owned<Texture> spriteSheet;
        Owned<Texture> shadowTexture;

        // Haupt-Funktionen
        void Init(SDL_Renderer* renderer);

        // Update nutzt jetzt automatisch die große Map (160)
        void Update(float dt, const MapType& map);

        void Input(const Event& evt);
        void Render(SDL_Renderer* renderer, FPoint camera, int scale);

        // --- KAMPF FUNKTIONEN ---
        void TakeDamage(int amount);
        FRect GetAttackHitbox() const;
        // ------------------------

    private:
        bool CheckCollision(const FRect& rect, const MapType& map);
    };
}