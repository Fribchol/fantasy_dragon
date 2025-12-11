#include "enemy.hpp"
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <iostream>

#ifndef BasePathGraphic
#define BasePathGraphic "asset/graphics/"
#endif

namespace JanSordid::SDL_Example
{
    static Owned<Texture> LoadTex(SDL_Renderer* r, const char* file) {
        auto* s = IMG_Load(file);
        if(!s) { SDL_Log("Fehler beim Laden: %s", file); return nullptr; }
        return Owned<Texture>(SDL_CreateTextureFromSurface(r, s));
    }

    // Runder Schatten Algorithmus
    static SDL_Texture* CreateEnemyShadow(SDL_Renderer* r) {
        SDL_Surface* s = SDL_CreateSurface(32, 16, SDL_PIXELFORMAT_RGBA8888);
        if (!s) return nullptr;

        SDL_FillSurfaceRect(s, nullptr, SDL_MapRGBA(SDL_GetPixelFormatDetails(s->format), nullptr, 0,0,0,0));

        for(int y=0; y<16; ++y) {
            for(int x=0; x<32; ++x) {
                float dx = (x - 16.0f) / 16.0f;
                float dy = (y - 8.0f) / 8.0f;
                if(dx*dx + dy*dy <= 1.0f) {
                    SDL_Rect p = {x,y,1,1};
                    SDL_FillSurfaceRect(s, &p, SDL_MapRGBA(SDL_GetPixelFormatDetails(s->format), nullptr, 0, 0, 0, 100));
                }
            }
        }
        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
        SDL_DestroySurface(s);
        return t;
    }

    void Bee::Init(SDL_Renderer* renderer, float startX, float startY) {
        position = { startX, startY };
        texFly    = LoadTex(renderer, BasePathGraphic "Bee-Fly-Sheet.png");
        texAttack = LoadTex(renderer, BasePathGraphic "Bee-Attack-Sheet.png");
        texHit    = LoadTex(renderer, BasePathGraphic "Bee-Hit-Sheet.png");
        shadowTexture.reset(CreateEnemyShadow(renderer));
    }

    void Bee::TakeDamage(int amount) {
        if (state == BeeState::Dead || state == BeeState::Hit) return;

        hp -= amount;
        if (hp <= 0) {
            state = BeeState::Dead;
        } else {
            state = BeeState::Hit;
            currentFrame = 0;
            animTimer = 0;
        }
    }

    FRect Bee::GetHitbox() const {
        return { position.x, position.y, size.x, size.y };
    }

    FRect Bee::GetAttackBox() const {
        float reach = 20.0f;
        float xOff = facingRight ? size.x : -reach;
        return { position.x + xOff, position.y, reach, size.y };
    }

    void Bee::Update(float dt, Player& player) {
        if (state == BeeState::Dead) return;

        if (attackCooldown > 0) attackCooldown -= dt;

        float distX = (player.position.x + player.size.x/2) - (position.x + size.x/2);
        float distY = (player.position.y + player.size.y/2) - (position.y + size.y/2);
        float dist = std::sqrt(distX*distX + distY*distY);

        if (state != BeeState::Hit && state != BeeState::Attack) {
            if (distX > 0) facingRight = true;
            else facingRight = false;
        }

        switch (state) {
            case BeeState::Fly:
                if (dist > 30.0f) {
                    float speed = 60.0f;
                    velocity.x = (distX / dist) * speed;
                    velocity.y = (distY / dist) * speed;
                } else {
                    velocity = {0,0};
                    if (attackCooldown <= 0) {
                        state = BeeState::Attack;
                        currentFrame = 0;
                        animTimer = 0;
                    }
                }
                z = 20.0f + std::sin(SDL_GetTicks() / 200.0f) * 5.0f;
                break;

            case BeeState::Attack:
                velocity = {0,0};
                if (currentFrame == 2 && attackCooldown <= 0) {
                    float zDiff = std::abs(player.z - z);
                    if (dist < 40.0f && zDiff < 30.0f) {
                        player.TakeDamage(10);
                        attackCooldown = 1.5f;
                    }
                }
                break;

            case BeeState::Hit:
                if (facingRight) velocity.x = -50; else velocity.x = 50;
                break;
        }

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;

        animTimer += dt;
        float frameTime = 0.1f;
        int maxFrames = 4;

        if (animTimer >= frameTime) {
            animTimer = 0;
            currentFrame++;

            if (currentFrame >= maxFrames) {
                if (state == BeeState::Attack) {
                    state = BeeState::Fly;
                    attackCooldown = 1.0f;
                } else if (state == BeeState::Hit) {
                    state = BeeState::Fly;
                    velocity = {0,0};
                } else {
                    currentFrame = 0;
                }
            }
        }
    }

    void Bee::Render(SDL_Renderer* r, FPoint cam, int scale) {
        if (state == BeeState::Dead) return;

        Texture* t = texFly.get();
        if (state == BeeState::Attack) t = texAttack.get();
        if (state == BeeState::Hit) t = texHit.get();
        if (!t) return;

        // --- SCHATTEN ---
        if (shadowTexture) {
            float shadowW = 20.0f * scale;
            float shadowH = 10.0f * scale;

            // HIER SIND DIE NEUEN VERSCHIEBUNGEN:
            // -----------------------------------
            float manualOffsetX = 15.0f * scale; // Nach Rechts schieben
            float manualOffsetY = 5.0f * scale;  // Nach Unten schieben
            // -----------------------------------

            // Zentrierung + Manueller Offset
            float centerX = ((size.x * scale - shadowW) / 2.0f) + manualOffsetX;

            FRect sRect = {
                (position.x * scale) + cam.x + centerX,
                // Boden Position + Biene Höhe + Offset nach unten
                (position.y * scale) + cam.y + (size.y * scale) - (shadowH / 2.0f) + manualOffsetY,
                shadowW,
                shadowH
            };

            // Kleiner werden je höher Z ist
            float scaleFactor = 1.0f - (z / 200.0f);
            if (scaleFactor < 0.5f) scaleFactor = 0.5f;

            // Skalierung anwenden
            sRect.w *= scaleFactor;
            sRect.h *= scaleFactor;

            // Nach Skalierung die Mitte korrigieren, damit er nicht wegdriftet
            sRect.x += (shadowW - sRect.w) / 2.0f;
            sRect.y += (shadowH - sRect.h) / 2.0f;

            SDL_RenderTexture(r, shadowTexture.get(), nullptr, &sRect);
        }

        // --- BIENE ---
        float w, h; SDL_GetTextureSize(t, &w, &h);
        float frameW = w / 4.0f;

        SDL_FRect src = { (float)currentFrame * frameW, 0, frameW, h };

        FRect dst = {
            (position.x * scale) + cam.x,
            (position.y * scale) + cam.y - (z * scale),
            frameW * scale,
            h * scale
        };

        SDL_FlipMode flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(r, t, &src, &dst, 0, nullptr, flip);
    }
}