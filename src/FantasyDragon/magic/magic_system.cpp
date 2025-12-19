#include "magic_system.hpp"

#include <algorithm> // std::clamp
#include <cstdlib>   // std::abs
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace JanSordid::FantasyDragon::Magic
{
    // ---------------- Grid64 ----------------

    void MagicSystem::Grid64::Clear()
    {
        px.fill(0);
    }

    void MagicSystem::Grid64::Set(int gx, int gy)
    {
        if (gx < 0 || gx >= W || gy < 0 || gy >= H) return;
        px[gy * W + gx] = 1;
    }

    u8 MagicSystem::Grid64::Get(int gx, int gy) const
    {
        if (gx < 0 || gx >= W || gy < 0 || gy >= H) return 0;
        return px[gy * W + gx];
    }

    // -------------- Canvas Setup -------------

    void MagicSystem::SetCanvasRect(int x, int y, int w, int h)
    {
        canvas.x = x;
        canvas.y = y;
        canvas.w = (w <= 0) ? 1 : w;
        canvas.h = (h <= 0) ? 1 : h;
    }

    const MagicSystem::CanvasRect& MagicSystem::GetCanvasRect() const
    {
        return canvas;
    }

    void MagicSystem::ClearDrawing()
    {
        drawing.Clear();
        drawingActive = false;
        lastGX = -1;
        lastGY = -1;
    }

    const MagicSystem::Grid64& MagicSystem::GetDrawing() const
    {
        return drawing;
    }

    bool MagicSystem::LoadTemplateTxt(const char* filePath, Template64& out)
    {
        std::ifstream in(filePath);
        if (!in.is_open()) return false;

        out.mask.Clear();
        out.onCount = 0;

        std::string line;
        int y = 0;

        while (std::getline(in, line) && y < Grid64::H)
        {
            // Erlaubt: kürzere Zeilen -> Rest gilt als '.'
            for (int x = 0; x < Grid64::W && x < (int)line.size(); ++x)
            {
                if (line[x] == '#')
                {
                    out.mask.Set(x, y);
                    out.onCount++;
                }
            }
            y++;
        }

        // Mindestanforderung: Template muss “irgendwas” enthalten
        return (out.onCount > 0);
    }

    void MagicSystem::AddTemplate(MagicResult spell, const Grid64& mask, const char* name)
    {
        Template64 t;
        t.spell = spell;
        t.mask = mask;
        t.name = (name ? name : "");

        int count = 0;
        for (int y = 0; y < Grid64::H; ++y)
            for (int x = 0; x < Grid64::W; ++x)
                if (t.mask.Get(x, y)) count++;

        t.onCount = count;
        templates.push_back(std::move(t));
    }

    bool MagicSystem::LoadTemplatesFromFolder(const char* folderPath)
    {
        templates.clear();

        if (!folderPath) return false;
        fs::path folder(folderPath);
        if (!fs::exists(folder) || !fs::is_directory(folder)) return false;

        auto loadOne = [&](const char* filename, MagicResult spell, const char* name) {
            Template64 t;
            t.spell = spell;
            t.name = name ? name : filename;

            fs::path p = folder / filename;
            if (!LoadTemplateTxt(p.string().c_str(), t)) return false;

            templates.push_back(std::move(t));
            return true;
        };

        bool ok = true;
        ok &= loadOne("fireball.txt", MagicResult::Fireball, "fireball");
        ok &= loadOne("heal.txt",     MagicResult::Heal,     "heal");
        return ok && !templates.empty();
    }

    MagicResult MagicSystem::EvaluateDrawing() const
    {
        if (templates.empty()) return MagicResult::Fail;

        // Zähle gezeichnete Pixel
        int drawnOn = 0;
        for (int y = 0; y < Grid64::H; ++y)
            for (int x = 0; x < Grid64::W; ++x)
                if (drawing.Get(x, y)) drawnOn++;

        if (drawnOn == 0) return MagicResult::Fail;

        // Scoring-Parameter
        const float extraPenalty = 1.2f;  // Strafe für “zu viel gemalt”
        const float minScore     = 0.72f; // Schwelle: je höher, desto strenger

        MagicResult bestSpell = MagicResult::Fail;
        float bestScore = 0.0f;

        for (const auto& t : templates)
        {
            int hits = 0;    // drawn=1 & templ=1
            int missing = 0; // drawn=0 & templ=1
            int extra = 0;   // drawn=1 & templ=0

            for (int y = 0; y < Grid64::H; ++y)
            {
                for (int x = 0; x < Grid64::W; ++x)
                {
                    const bool d = drawing.Get(x, y) != 0;
                    const bool m = t.mask.Get(x, y) != 0;

                    if (d && m) hits++;
                    else if (!d && m) missing++;
                    else if (d && !m) extra++;
                }
            }

            // Normalisierung: Treffer relativ zu “was man treffen sollte”
            // + Strafe für extra pixels
            const float denom = float(hits + missing) + float(extra) * extraPenalty;
            const float score = (denom <= 0.0f) ? 0.0f : (float(hits) / denom);

            if (score > bestScore)
            {
                bestScore = score;
                bestSpell = t.spell;
            }
        }

        if (bestScore >= minScore)
            return bestSpell;

        return MagicResult::Fail;
    }


    // -------------- Lifecycle ----------------

    void MagicSystem::BeginCast(int& mana)
    {
        if (active) return;

        // TODO: Mana-Kosten festlegen
        // mana -= X;

        active = true;
        timer  = 3.0f;
        result = MagicResult::None;

        ClearDrawing();
    }

    void MagicSystem::Cancel()
    {
        active = false;
        timer  = 0.0f;
        result = MagicResult::None;

        ClearDrawing();
    }

    void MagicSystem::Update(f32 dt)
    {
        if (!active) return;

        timer -= dt;
        if (timer <= 0.0f)
        {
            timer = 0.0f;

            result = EvaluateDrawing();

            active = false;
            drawingActive = false;
        }
    }

    // -------------- Input --------------------

    bool MagicSystem::ScreenToGrid(int sx, int sy, int& outGX, int& outGY) const
    {
        // nur innerhalb des Canvas zeichnen
        if (sx < canvas.x || sy < canvas.y || sx >= canvas.x + canvas.w || sy >= canvas.y + canvas.h)
            return false;

        // Screen -> [0..1)
        const float nx = float(sx - canvas.x) / float(canvas.w);
        const float ny = float(sy - canvas.y) / float(canvas.h);

        // -> Grid
        int gx = int(nx * Grid64::W);
        int gy = int(ny * Grid64::H);

        gx = std::clamp(gx, 0, Grid64::W - 1);
        gy = std::clamp(gy, 0, Grid64::H - 1);

        outGX = gx;
        outGY = gy;
        return true;
    }

    void MagicSystem::DrawLine(int x0, int y0, int x1, int y1)
    {
        // Bresenham (Integer)
        int dx = std::abs(x1 - x0);
        int sx = (x0 < x1) ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx + dy;

        for (;;)
        {
            drawing.Set(x0, y0);
            if (x0 == x1 && y0 == y1) break;

            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void MagicSystem::OnMouseDown(int sx, int sy)
    {
        if (!active) return;

        int gx = 0, gy = 0;
        if (!ScreenToGrid(sx, sy, gx, gy)) return;

        drawingActive = true;
        lastGX = gx;
        lastGY = gy;
        drawing.Set(gx, gy);
    }

    void MagicSystem::OnMouseMove(int sx, int sy, bool pressed)
    {
        if (!active) return;
        if (!pressed) return;
        if (!drawingActive) return;

        int gx = 0, gy = 0;
        if (!ScreenToGrid(sx, sy, gx, gy)) return;

        if (lastGX >= 0 && lastGY >= 0)
            DrawLine(lastGX, lastGY, gx, gy);
        else
            drawing.Set(gx, gy);

        lastGX = gx;
        lastGY = gy;
    }

    void MagicSystem::OnMouseUp()
    {
        drawingActive = false;
        lastGX = -1;
        lastGY = -1;
    }

    // -------------- Queries ------------------

    bool MagicSystem::IsActive() const
    {
        return active;
    }

    f32 MagicSystem::TimeLeft() const
    {
        return timer;
    }

    MagicResult MagicSystem::ConsumeResult()
    {
        MagicResult out = result;
        result = MagicResult::None;
        return out;
    }
}
