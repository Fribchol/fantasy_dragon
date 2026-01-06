#include "magic_debug_render.hpp"

namespace JanSordid::FantasyDragon::Magic
{
    void MagicDebugRender::RenderOverlay(SDL_Renderer* renderer,
                                         const MagicSystem& magic,
                                         int screenW,
                                         int screenH,
                                         MagicResult showTemplate)
    {
        if (!renderer) return;
        if (!magic.IsActive()) return;

        // 1) Graues Overlay (halbtransparent)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_FRect full = { 0.0f, 0.0f, (float)screenW, (float)screenH };
        SDL_RenderFillRect(renderer, &full);

        // 2) Rotes Quadrat (Canvas)
        const auto& c = magic.GetCanvasRect();
        SDL_FRect canvas = { (float)c.x, (float)c.y, (float)c.w, (float)c.h };

        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderRect(renderer, &canvas);

        // Zellgröße (wir brauchen sie für Template + Zeichnung)
        const float cellW = (float)c.w / (float)MagicSystem::Grid64::W;
        const float cellH = (float)c.h / (float)MagicSystem::Grid64::H;

        // 2.5) Template als Geisterbild im roten Quadrat
        if (const auto* templ = magic.GetTemplateMask(showTemplate))
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 70); // halbtransparent

            for (int gy = 0; gy < MagicSystem::Grid64::H; ++gy)
            {
                for (int gx = 0; gx < MagicSystem::Grid64::W; ++gx)
                {
                    if (!templ->Get(gx, gy)) continue;

                    SDL_FRect px = {
                        (float)c.x + gx * cellW,
                        (float)c.y + gy * cellH,
                        cellW,
                        cellH
                    };
                    SDL_RenderFillRect(renderer, &px);
                }
            }
        }

        // 3) Gezeichnete Pixel (Raster 64x64 als kleine Rechtecke)
        const auto& g = magic.GetDrawing();

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int gy = 0; gy < MagicSystem::Grid64::H; ++gy)
        {
            for (int gx = 0; gx < MagicSystem::Grid64::W; ++gx)
            {
                if (!g.Get(gx, gy)) continue;

                SDL_FRect px = {
                    (float)c.x + gx * cellW,
                    (float)c.y + gy * cellH,
                    cellW,
                    cellH
                };
                SDL_RenderFillRect(renderer, &px);
            }
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
