#pragma once

#include "magic_system.hpp"
#include <SDL3/SDL.h>

namespace JanSordid::FantasyDragon::Magic
{
    struct MagicDebugRender
    {
        // Zeichnet: graues Overlay + rotes Quadrat + gezeichnete Pixel
        static void RenderOverlay(SDL_Renderer* renderer,
                          const MagicSystem& magic,
                          int screenW,
                          int screenH,
                          MagicResult showTemplate);

    };
}