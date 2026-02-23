#pragma once

#include "sor/sdl_game.hpp"
#include "sor/sdl_smartptr.hpp"
#include "player.hpp"
#include <array>
#include <memory>

namespace JanSordid::SDL_Example
{
    using JanSordid::SDL::Texture;
    using JanSordid::SDL::FPoint;
    using JanSordid::SDL::FRect;

    template <typename T> using Owned = std::unique_ptr<T>;

    // --- ÄNDERUNG: Neuer Zustand "Idle" ---
    enum class BeeState {
        Idle,   // Wartet friedlich (schwebt nur)
        Fly,    // Verfolgt den Spieler (Aggro)
        Attack, // Sticht zu
        Hit,    // Wurde getroffen
        Dead    // Tot
    };

    struct Bee {
        FPoint position = { 300.0f, 200.0f };
        FPoint velocity = { 0.0f, 0.0f };
        float z = 20.0f;

        FPoint size = { 24.0f, 24.0f };

        int maxHp = 30;
        int hp = 30;
        bool facingRight = false;

        // --- ÄNDERUNG: Startet jetzt im Idle-Modus ---
        BeeState state = BeeState::Idle;

        // --- ÄNDERUNG: Radius, ab dem der Spieler angegriffen wird ---
        float aggroRadius = 250.0f;

        float stateTimer = 0.0f;
        int currentFrame = 0;
        float animTimer = 0.0f;
        float attackCooldown = 0.0f;

        Owned<Texture> texFly;
        Owned<Texture> texAttack;
        Owned<Texture> texHit;
        Owned<Texture> shadowTexture;

        void Init(SDL_Renderer* renderer, float startX, float startY);
        void Update(float dt, Player& player);
        void Render(SDL_Renderer* renderer, FPoint camera, int scale);

        void TakeDamage(int amount);
        FRect GetHitbox() const;
        FRect GetAttackBox() const;
    };

    enum class MushroomState {
        Idle,
        Run,
        Attack,
        Hit,
        Dead
    };

    struct Mushroom {
        // position ist der Fuss-Punkt (Mitte X, Boden Y)
        FPoint position = { 400.0f, 220.0f };
        FPoint velocity = { 0.0f, 0.0f };
        float z = 0.0f;

        FPoint size = { 32.0f, 32.0f };

        int maxHp = 40;
        int hp = 40;
        bool facingRight = true;

        MushroomState state = MushroomState::Idle;
        float aggroRadius = 220.0f;
        float attackCooldown = 0.0f;

        int currentFrame = 0;
        float animTimer = 0.0f;
        float frameW = 80.0f;
        float frameH = 64.0f;
        bool deadFinished = false;

        Owned<Texture> texIdle;
        Owned<Texture> texRun;
        Owned<Texture> texAttack;
        Owned<Texture> texHit;
        Owned<Texture> texDie;
        Owned<Texture> shadowTexture;

        void Init(SDL_Renderer* renderer, float startX, float startY);
        void Update(float dt, Player& player);
        void Render(SDL_Renderer* renderer, FPoint camera, int scale);

        void TakeDamage(int amount);
        FRect GetHitbox() const;
        FRect GetAttackBox() const;
    };
}
