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

    enum class BeeState {
        Fly,
        Attack,
        Hit,
        Dead
    };

    struct Bee {
        FPoint position = { 300.0f, 200.0f };
        FPoint velocity = { 0.0f, 0.0f };
        float z = 20.0f;

        FPoint size = { 24.0f, 24.0f };

        int hp = 30;
        bool facingRight = false;

        BeeState state = BeeState::Fly;
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
}