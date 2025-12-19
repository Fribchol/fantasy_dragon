#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace JanSordid::FantasyDragon::Magic
{
    using u8  = std::uint8_t;
    using f32 = float;

    enum class MagicResult : u8
    {
        None,
        Fireball,
        Ice,
        Heal,
        Fail
    };

    struct MagicSystem
    {
        // Canvas in Screen-Pixeln (z.B. rotes Quadrat in der Mitte)
        struct CanvasRect
        {
            int x = 0;
            int y = 0;
            int w = 256;
            int h = 256;
        };

        // 64x64 Raster (0/1)
        struct Grid64
        {
            static constexpr int W = 64;
            static constexpr int H = 64;

            std::array<u8, W * H> px{};

            void Clear();
            void Set(int gx, int gy);
            u8   Get(int gx, int gy) const;
        };

        struct Template64
        {
            MagicResult spell = MagicResult::Fail;
            Grid64 mask{};
            int onCount = 0; // Anzahl gesetzter Pixel im Template
            std::string name;
        };

        // Templates laden (aus .txt)
        bool LoadTemplatesFromFolder(const char* folderPath);

        // Optional: manuell Template hinzufügen (z.B. fürs Debuggen)
        void AddTemplate(MagicResult spell, const Grid64& mask, const char* name);

        // --- Lebenszyklus ---
        void BeginCast(int& mana);
        void Cancel();
        void Update(f32 dt);

        // --- Canvas Setup ---
        void SetCanvasRect(int x, int y, int w, int h);
        const CanvasRect& GetCanvasRect() const;

        // --- Raster Zugriff ---
        void ClearDrawing();
        const Grid64& GetDrawing() const;

        // --- Input (Screen-Koordinaten) ---
        void OnMouseDown(int x, int y);
        void OnMouseMove(int x, int y, bool pressed);
        void OnMouseUp();

        // --- Abfrage ---
        bool IsActive() const;
        f32  TimeLeft() const;
        MagicResult ConsumeResult();

    private:
        // Helfer
        bool ScreenToGrid(int sx, int sy, int& outGX, int& outGY) const;
        void DrawLine(int x0, int y0, int x1, int y1);

        bool active = false;
        f32  timer  = 0.0f;

        MagicResult result = MagicResult::None;

        CanvasRect canvas{};
        Grid64 drawing{};

        bool drawingActive = false;
        int  lastGX = -1;
        int  lastGY = -1;

        MagicResult EvaluateDrawing() const;
        static bool LoadTemplateTxt(const char* filePath, Template64& out);

        std::vector<Template64> templates{};

    };
}
