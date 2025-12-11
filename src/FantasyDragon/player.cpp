#include "player.hpp"
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <cmath>

#ifndef BasePathGraphic
#define BasePathGraphic "assets/graphics/"
#endif

namespace JanSordid::SDL_Example
{
    SDL_Texture* CreateShadowTexture(SDL_Renderer* r) {
        SDL_Surface* s = SDL_CreateSurface(32, 16, SDL_PIXELFORMAT_RGBA8888);
        if(!s) return nullptr;
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

    void Player::Init(SDL_Renderer* renderer) {
        const char* filename = BasePathGraphic "adventurer-v1.5-Sheet.png";

        auto* surfRaw = IMG_Load(filename);
        Owned<SDL_Surface> surf(surfRaw);

        if(!surf) {
             SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Spieler Sprite fehlt: %s", filename);
        } else {
             spriteSheet.reset(SDL_CreateTextureFromSurface(renderer, surf.get()));
        }

        shadowTexture.reset(CreateShadowTexture(renderer));

        position = { 100.0f, 150.0f };
        velocity = { 0.0f, 0.0f };
        z = 0.0f;
        velZ = 0.0f;
    }

    void Player::Input(const Event& evt) {
        if (isAttacking) return;

        const float JUMP_FORCE = 400.0f;

        if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.repeat == 0) {
            if (evt.key.scancode == SDL_SCANCODE_SPACE) {
                if (z <= 0.1f) {
                    velZ = JUMP_FORCE;
                    currentAnim = PlayerAnim::Jump;
                    currentFrame = 0;
                    animTimer = 0;
                }
            }

            if (evt.key.scancode == SDL_SCANCODE_KP_4 ) {
                isAttacking = true; currentAnim = PlayerAnim::Attack1; currentFrame = 0; animTimer = 0;
            }
            if (evt.key.scancode == SDL_SCANCODE_KP_8 ) {
                isAttacking = true; currentAnim = PlayerAnim::Attack2; currentFrame = 0; animTimer = 0;
            }
            if (evt.key.scancode == SDL_SCANCODE_KP_6 ) {
                isAttacking = true; currentAnim = PlayerAnim::Attack3; currentFrame = 0; animTimer = 0;
            }
        }
    }

    bool Player::CheckCollision(const FRect& rect, const MapType& map) {
        int minX = (int)(rect.x / 16.0f);
        int maxX = (int)((rect.x + rect.w - 0.1f) / 16.0f);
        int minY = (int)(rect.y / 16.0f);
        int maxY = (int)((rect.y + rect.h - 0.1f) / 16.0f);

        if (minX < 0) minX = 0; if (maxX >= 40) maxX = 39;
        if (minY < 0) minY = 0; if (maxY >= 20) maxY = 19;

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (map[y][x] != 0) return true;
            }
        }
        return false;
    }

    void Player::Update(float dt, const MapType& map) {
        const float GRAVITY = 980.0f;
        const float MOVE_SPEED_X = 120.0f;
        const float MOVE_SPEED_Y = 80.0f;

        const bool* state = SDL_GetKeyboardState(nullptr);

        velocity = { 0.0f, 0.0f };

        if (!isAttacking) {
            if (state[SDL_SCANCODE_A]) { velocity.x = -MOVE_SPEED_X; facingRight = false; }
            if (state[SDL_SCANCODE_D]) { velocity.x =  MOVE_SPEED_X; facingRight = true; }
            if (state[SDL_SCANCODE_W]) { velocity.y = -MOVE_SPEED_Y; }
            if (state[SDL_SCANCODE_S]) { velocity.y =  MOVE_SPEED_Y; }

            // Animation Auswahl
            if (z > 0.1f) {
                currentAnim = PlayerAnim::Jump;
            } else {
                if (velocity.x != 0 || velocity.y != 0) currentAnim = PlayerAnim::Run;
                else if (state[SDL_SCANCODE_Q] && velocity.y == 0) currentAnim = PlayerAnim::Crouch;
                else currentAnim = PlayerAnim::Idle;
            }
        }

        // Physik X
        position.x += velocity.x * dt;
        FRect hitBoxX = { position.x, position.y, size.x, size.y / 2.0f };
        if (CheckCollision(hitBoxX, map)) position.x -= velocity.x * dt;

        // Physik Y
        position.y += velocity.y * dt;
        FRect hitBoxY = { position.x, position.y, size.x, size.y / 2.0f };
        if (CheckCollision(hitBoxY, map)) position.y -= velocity.y * dt;

        // Physik Z
        velZ -= GRAVITY * dt;
        z += velZ * dt;
        if (z <= 0.0f) { z = 0.0f; velZ = 0.0f; }

        // Grenzen
        if (position.x < 0) position.x = 0; if (position.y < 0) position.y = 0;
        if (position.x > 40*16 - size.x) position.x = 40*16 - size.x;
        if (position.y > 20*16 - size.y) position.y = 20*16 - size.y;

        // --- ANIMATION UPDATE ---
        animTimer += dt;

        float frameTime = 0.1f;
        int startCol = 0;
        int frameCount = 4;
        bool loop = true;

        switch(currentAnim) {
            case PlayerAnim::Idle:
                startCol = 0; frameCount = 4; frameTime = 0.15f;
                break;

            case PlayerAnim::Run:
                startCol = 1; frameCount = 5; frameTime = 0.1f;
                break;

            case PlayerAnim::Crouch:
                startCol = 4; frameCount = 3;
                break;

            case PlayerAnim::Jump:
                startCol = 0;
                frameCount = 10; frameTime = 0.12f;// 7 Frames (Reihe 2) + 2 Frames (Reihe 3) = 9 Total
                loop = false;
                break;

            case PlayerAnim::Attack1:
                startCol = 0; frameCount = 5; frameTime = 0.16f; loop = false;
                break;
            case PlayerAnim::Attack2:
                startCol = 0; frameCount = 4; frameTime = 0.16f; loop = false;
                break;
            case PlayerAnim::Attack3:
                startCol = 5; frameCount = 7; frameTime = 0.16f; loop = false;
                break;
        }

        // Sicherheitscheck
        if (currentFrame < startCol || currentFrame >= startCol + frameCount) {
            currentFrame = startCol;
        }

        if (animTimer >= frameTime) {
            animTimer = 0;
            currentFrame++;

            if (currentFrame >= startCol + frameCount) {
                if (loop) {
                    currentFrame = startCol;
                } else {
                    currentFrame = startCol + frameCount - 1;
                    if (isAttacking) { isAttacking = false; }
                }
            }
        }
    }

    void Player::Render(SDL_Renderer* renderer, FPoint camera, int scale) {
        if (!spriteSheet) return;

        // Schatten
        if (shadowTexture) {
            float shadowW = 20.0f * scale; float shadowH = 10.0f * scale;
            JanSordid::SDL::FRect shadowRect = {
                (position.x * scale) + camera.x + (size.x * scale / 2.0f) - (shadowW / 2.0f),
                (position.y * scale) + camera.y + (size.y * scale / 2.0f),
                shadowW, shadowH
            };
            float scaleFactor = 1.0f - (z / 200.0f); if (scaleFactor < 0.5f) scaleFactor = 0.5f;
            shadowRect.w *= scaleFactor; shadowRect.h *= scaleFactor;
            shadowRect.x += (shadowW - shadowRect.w) / 2.0f; shadowRect.y += (shadowH - shadowRect.h) / 2.0f;
            SDL_RenderTexture(renderer, shadowTexture.get(), nullptr, &shadowRect);
        }

        // Spieler
        int spriteW = 50;
        int spriteH = 37;

        // Diese Variablen berechnen wir jetzt dynamisch:
        int row = 0;
        int col = currentFrame; // Standard: Spalte = aktueller Frame

        switch(currentAnim) {
            case PlayerAnim::Idle:    row = 0; break;
            case PlayerAnim::Run:     row = 1; break;
            case PlayerAnim::Crouch:  row = 0; break;

            case PlayerAnim::Jump:
                // --- SPEZIAL LOGIK FÜR JUMP ÜBER 2 REIHEN ---
                // Annahme: Eine Reihe hat 7 Bilder (Index 0 bis 6)
                if (currentFrame < 7) {
                    // Die ersten 7 Bilder sind in Reihe 2
                    row = 2;
                    col = currentFrame;
                } else {
                    // Ab Bild 8 (Index 7) sind wir in Reihe 3
                    row = 3;
                    col = currentFrame - 7; // Reset X auf Anfang der neuen Zeile
                }
                break;

            case PlayerAnim::Attack1: row = 6; break;
            case PlayerAnim::Attack2: row = 7; break;
            // case PlayerAnim::Attack3: row = 11; break;

            case PlayerAnim::Attack3:
                // --- SPEZIAL LOGIK FÜR ATTACK3 ÜBER 2 REIHEN ---
                    // Annahme: Eine Reihe hat 7 Bilder (Index 0 bis 6)
                        if (currentFrame < 7) {
                            // ab bild 5 Bilder sind in Reihe 7
                            row = 7;
                            col = currentFrame;
                        } else {
                            // Ab Bild 8 (Index 7) sind wir in Reihe 8
                            row = 8;
                            col = currentFrame - 7; // Reset X auf Anfang der neuen Zeile
                        }
            break;




        }

        // WICHTIG: Hier nutzen wir jetzt 'col' statt 'currentFrame' für die X-Position
        SDL_FRect srcR = {
            (float)(col * spriteW),
            (float)(row * spriteH),
            (float)spriteW,
            (float)spriteH
        };

        FPoint screenPos;
        screenPos.x = position.x + spriteOffset.x;
        screenPos.y = position.y + spriteOffset.y - z;

        JanSordid::SDL::FRect dstR = {
            (screenPos.x * scale) + camera.x,
            (screenPos.y * scale) + camera.y,
            (float)spriteW * scale,
            (float)spriteH * scale
        };

        SDL_FlipMode flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        SDL_RenderTextureRotated(renderer, spriteSheet.get(), &srcR, &dstR, 0.0, nullptr, flip);
    }
}