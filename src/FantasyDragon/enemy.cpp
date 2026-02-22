#include "enemy.hpp"
// Player muss inkludiert sein, da wir ihn im Update benutzen
#include "player.hpp"

#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <iostream>

namespace JanSordid::SDL_Example
{
    // Helper Funktion zum Laden
    static Owned<Texture> LoadTex(SDL_Renderer* r, const char* file) {
        auto* s = IMG_Load(file);
        if(!s) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Enemy-Bild fehlt: %s", file);
            return nullptr;
        }
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

    namespace EnemyCommon
    {
        static FRect MakeHitbox(const FPoint& pos, const FPoint& size) {
            return { pos.x, pos.y, size.x, size.y };
        }

        static void ResetAnim(int& frame, float& timer) {
            frame = 0;
            timer = 0.0f;
        }

        static bool AdvanceAnim(float dt, float frameTime, int frameCount, int& frame, float& timer, bool loop) {
            timer += dt;
            if (timer < frameTime) return false;
            timer = 0.0f;
            frame++;
            if (frame < frameCount) return false;
            if (loop) {
                frame = 0;
            } else {
                frame = frameCount - 1;
            }
            return true;
        }

        static void RenderShadowAirborne(SDL_Renderer* r,
                                         SDL_Texture* shadow,
                                         const FPoint& pos,
                                         const FPoint& size,
                                         float z,
                                         FPoint cam,
                                         int scale,
                                         float shadowW = 20.0f,
                                         float shadowH = 10.0f,
                                         float manualOffsetX = 15.0f,
                                         float manualOffsetY = 5.0f)
        {
            if (!shadow) return;
            float w = shadowW * scale;
            float h = shadowH * scale;
            float centerX = ((size.x * scale - w) / 2.0f) + (manualOffsetX * scale);

            FRect sRect = {
                (pos.x * scale) + cam.x + centerX,
                (pos.y * scale) + cam.y + (size.y * scale) - (h / 2.0f) + (manualOffsetY * scale),
                w,
                h
            };

            float scaleFactor = 1.0f - (z / 200.0f);
            if (scaleFactor < 0.5f) scaleFactor = 0.5f;

            sRect.w *= scaleFactor;
            sRect.h *= scaleFactor;
            sRect.x += (w - sRect.w) / 2.0f;
            sRect.y += (h - sRect.h) / 2.0f;

            SDL_RenderTexture(r, shadow, nullptr, &sRect);
        }

        static void RenderShadowGrounded(SDL_Renderer* r,
                                         SDL_Texture* shadow,
                                         const FPoint& footPos,
                                         float z,
                                         FPoint cam,
                                         int scale,
                                         float shadowW = 22.0f,
                                         float shadowH = 8.0f)
        {
            if (!shadow) return;
            float w = shadowW * scale;
            float h = shadowH * scale;

            FRect sRect = {
                (footPos.x * scale) + cam.x - (w * 0.5f),
                (footPos.y * scale) + cam.y - (h * 0.5f),
                w,
                h
            };

            float scaleFactor = 1.0f - (z / 200.0f);
            if (scaleFactor < 0.7f) scaleFactor = 0.7f;
            sRect.w *= scaleFactor;
            sRect.h *= scaleFactor;
            sRect.x += (w - sRect.w) * 0.5f;
            sRect.y += (h - sRect.h) * 0.5f;

            SDL_RenderTexture(r, shadow, nullptr, &sRect);
        }
    }

    void Bee::Init(SDL_Renderer* renderer, float startX, float startY) {
        position = { startX, startY };
        maxHp = 30;
        hp = maxHp; // 3 Leben (1 Schlag = 10 Schaden)

        // --- ÄNDERUNG: Startet friedlich ---
        state = BeeState::Idle;

        // --- FIX: PFADE HARTKODIERT ---
        texFly    = LoadTex(renderer, "asset/graphic/Bee-Fly-Sheet.png");
        texAttack = LoadTex(renderer, "asset/graphic/Bee-Attack-Sheet.png");
        texHit    = LoadTex(renderer, "asset/graphic/Bee-Hit-Sheet.png");

        shadowTexture.reset(CreateEnemyShadow(renderer));
    }

    void Bee::TakeDamage(int amount) {
        if (state == BeeState::Dead || state == BeeState::Hit) return;

        hp -= amount;
        if (hp <= 0) {
            state = BeeState::Dead;
        } else {
            state = BeeState::Hit;
            EnemyCommon::ResetAnim(currentFrame, animTimer);
        }
    }

    FRect Bee::GetHitbox() const {
        return EnemyCommon::MakeHitbox(position, size);
    }

    FRect Bee::GetAttackBox() const {
        float reach = 20.0f;
        float xOff = facingRight ? size.x : -reach;
        return { position.x + xOff, position.y, reach, size.y };
    }

    void Bee::Update(float dt, Player& player) {
        if (state == BeeState::Dead) return;

        if (attackCooldown > 0) attackCooldown -= dt;

        // Distanz zum Spieler berechnen
        float distX = (player.position.x + player.size.x/2) - (position.x + size.x/2);
        float distY = (player.position.y + player.size.y/2) - (position.y + size.y/2);
        float dist = std::sqrt(distX*distX + distY*distY);

        if (state != BeeState::Hit && state != BeeState::Attack) {
            if (distX > 0) facingRight = true;
            else facingRight = false;
        }

        switch (state) {
            // --- NEU: IDLE LOGIK ---
            case BeeState::Idle:
                // Schwebt nur hoch und runter (Sinus-Welle)
                z = 20.0f + std::sin(SDL_GetTicks() / 200.0f) * 5.0f;
                velocity = {0, 0};

                // Wenn Spieler nah genug ist -> ANGRIFF (Wechsel zu Fly)
                if (dist < aggroRadius) {
                    state = BeeState::Fly;
                }
                break;

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

                // Optional: Wenn Spieler zu weit wegrennt, wieder Idle?
                if (dist > aggroRadius * 2.0f) {
                    state = BeeState::Idle;
                }
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

        const float frameTime = 0.1f;
        const int maxFrames = 4;
        const bool loop = (state != BeeState::Attack && state != BeeState::Hit);
        const bool finished = EnemyCommon::AdvanceAnim(dt, frameTime, maxFrames, currentFrame, animTimer, loop);
        if (finished) {
            if (state == BeeState::Attack) {
                state = BeeState::Fly;
                attackCooldown = 1.0f;
            } else if (state == BeeState::Hit) {
                state = BeeState::Fly;
                velocity = {0,0};
            }
        }
    }

    void Bee::Render(SDL_Renderer* r, FPoint cam, int scale) {
        if (state == BeeState::Dead) return;

        Texture* t = texFly.get(); // Idle benutzt auch Fly-Texture
        if (state == BeeState::Attack) t = texAttack.get();
        if (state == BeeState::Hit) t = texHit.get();

        // Wenn Texture nicht geladen wurde (nullptr), hier abbrechen, sonst Absturz!
        if (!t) return;

        // --- SCHATTEN ---
        EnemyCommon::RenderShadowAirborne(r, shadowTexture.get(), position, size, z, cam, scale);

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

    void Mushroom::Init(SDL_Renderer* renderer, float startX, float startY) {
        position = { startX, startY };
        maxHp = 40;
        hp = maxHp;
        z = 0.0f;
        state = MushroomState::Idle;
        attackCooldown = 0.0f;
        EnemyCommon::ResetAnim(currentFrame, animTimer);
        deadFinished = false;

        texIdle   = LoadTex(renderer, "asset/enemy/Mushroom/Mushroom-Idle.png");
        texRun    = LoadTex(renderer, "asset/enemy/Mushroom/Mushroom-Run.png");
        texAttack = LoadTex(renderer, "asset/enemy/Mushroom/Mushroom-Attack.png");
        texHit    = LoadTex(renderer, "asset/enemy/Mushroom/Mushroom-Hit.png");
        texDie    = LoadTex(renderer, "asset/enemy/Mushroom/Mushroom-Die.png");

        shadowTexture.reset(CreateEnemyShadow(renderer));

        if (texIdle) {
            float w = 0.0f, h = 0.0f;
            SDL_GetTextureSize(texIdle.get(), &w, &h);
            frameH = h;
        }
    }

    void Mushroom::TakeDamage(int amount) {
        if (state == MushroomState::Dead || state == MushroomState::Hit) return;

        hp -= amount;
        if (hp <= 0) {
            state = MushroomState::Dead;
            EnemyCommon::ResetAnim(currentFrame, animTimer);
            deadFinished = false;
        } else {
            state = MushroomState::Hit;
            EnemyCommon::ResetAnim(currentFrame, animTimer);
        }
    }

    FRect Mushroom::GetHitbox() const {
        // Default: Fußpunkt als Referenz
        const float hbW = 24.0f;
        const float hbH = 20.0f;
        const float offX = -12.0f;
        const float offY = -20.0f;
        return { position.x + offX, position.y + offY, hbW, hbH };
    }

    FRect Mushroom::GetAttackBox() const {
        const float reach = 28.0f;
        const float hbY = position.y - 20.0f;
        const float hbH = 20.0f;
        const float rightX = position.x + 12.0f;
        const float leftX = position.x - 12.0f - reach;
        return { facingRight ? rightX : leftX, hbY, reach, hbH };
    }

    void Mushroom::Update(float dt, Player& player) {
        if (deadFinished) return;
        if (attackCooldown > 0) attackCooldown -= dt;

        float distX = (player.position.x + player.size.x/2) - position.x;
        float distY = (player.position.y + player.size.y/2) - position.y;
        float dist = std::sqrt(distX*distX + distY*distY);

        if (state != MushroomState::Hit && state != MushroomState::Attack && state != MushroomState::Dead) {
            facingRight = (distX >= 0);
        }

        switch (state) {
            case MushroomState::Idle:
                velocity = {0,0};
                if (dist < aggroRadius) {
                    state = MushroomState::Run;
                    EnemyCommon::ResetAnim(currentFrame, animTimer);
                }
                break;

            case MushroomState::Run:
                if (dist > 20.0f) {
                    float speed = 55.0f;
                    velocity.x = (distX / dist) * speed;
                    velocity.y = (distY / dist) * speed;
                } else {
                    velocity = {0,0};
                    if (attackCooldown <= 0) {
                        state = MushroomState::Attack;
                        EnemyCommon::ResetAnim(currentFrame, animTimer);
                    }
                }
                if (dist > aggroRadius * 2.0f) {
                    state = MushroomState::Idle;
                    EnemyCommon::ResetAnim(currentFrame, animTimer);
                }
                break;

            case MushroomState::Attack:
                velocity = {0,0};
                break;

            case MushroomState::Hit:
                velocity.x = facingRight ? -40.0f : 40.0f;
                velocity.y = 0.0f;
                break;

            case MushroomState::Dead:
                velocity = {0,0};
                break;
        }

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;

        const int framesIdle = 7;
        const int framesRun = 8;
        const int framesAttack = 10;
        const int framesHit = 5;
        const int framesDie = 15;
        const float frameTime = 0.09f;

        int frameCount = framesIdle;
        bool loop = true;
        if (state == MushroomState::Run) frameCount = framesRun;
        if (state == MushroomState::Attack) { frameCount = framesAttack; loop = false; }
        if (state == MushroomState::Hit) { frameCount = framesHit; loop = false; }
        if (state == MushroomState::Dead) { frameCount = framesDie; loop = false; }

        const bool finished = EnemyCommon::AdvanceAnim(dt, frameTime, frameCount, currentFrame, animTimer, loop);
        if (state == MushroomState::Attack) {
            const int damageFrame = 4;
            if (currentFrame == damageFrame && attackCooldown <= 0) {
                float zDiff = std::abs(player.z - z);
                if (dist < 24.0f && zDiff < 30.0f) {
                    player.TakeDamage(10);
                    attackCooldown = 1.2f;
                }
            }
            if (finished) {
                state = MushroomState::Run;
                EnemyCommon::ResetAnim(currentFrame, animTimer);
            }
        } else if (state == MushroomState::Hit && finished) {
            state = MushroomState::Run;
            velocity = {0,0};
            EnemyCommon::ResetAnim(currentFrame, animTimer);
        } else if (state == MushroomState::Dead && finished) {
            deadFinished = true;
        }
    }

    void Mushroom::Render(SDL_Renderer* r, FPoint cam, int scale) {
        if (deadFinished) return;
        Texture* t = texIdle.get();
        if (state == MushroomState::Run) t = texRun.get();
        if (state == MushroomState::Attack) t = texAttack.get();
        if (state == MushroomState::Hit) t = texHit.get();
        if (state == MushroomState::Dead) t = texDie.get();
        if (!t) return;

        EnemyCommon::RenderShadowGrounded(r, shadowTexture.get(), position, z, cam, scale, 22.0f, 8.0f);

        const int frameW = (int)this->frameW;
        float w, h; SDL_GetTextureSize(t, &w, &h);
        float frameH = h;

        SDL_FRect src = { (float)(currentFrame * frameW), 0.0f, (float)frameW, frameH };
        FRect dst = {
            (position.x * scale) + cam.x - ((float)frameW * scale * 0.5f),
            (position.y * scale) + cam.y - (frameH * scale) - (z * scale),
            frameW * scale,
            frameH * scale
        };

        SDL_FlipMode flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(r, t, &src, &dst, 0, nullptr, flip);
    }
}
