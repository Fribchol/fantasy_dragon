#include "fantasydragon_mapeditor.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

#if __has_include("hsnr64/offset.hpp")
    #include "hsnr64/offset.hpp"
#endif

using namespace JanSordid;
using namespace JanSordid::Core;
using namespace JanSordid::SDL;

using JanSordid::SDL::toI;
using JanSordid::SDL::toF;
using JanSordid::SDL::toWH;
using JanSordid::SDL::toFRect;
using JanSordid::SDL::EntireFRect;

namespace JanSordid::SDL_Example
{
    // Globale Variablen
    bool GlobalSettings::soundEnabled = true;
    bool GlobalSettings::isFullscreen = false;

    // --- Hilfsfunktionen ---
    void SaveMapToFile(const std::string& filename, const std::array<std::array<int, 40>, 20>& map) {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (const auto& row : map) {
                for (size_t i = 0; i < row.size(); ++i) file << row[i] << (i < row.size() - 1 ? " " : "");
                file << "\n";
            }
            file.close();
            SDL_Log("Map gespeichert.");
        }
    }

    bool LoadMapFromFile(const std::string& filename, std::array<std::array<int, 40>, 20>& map) {
        std::ifstream file(filename);
        if (file.is_open()) {
            for (auto& row : map) for (auto& cell : row) file >> cell;
            file.close();
            SDL_Log("Map geladen.");
            return true;
        }
        return false;
    }

    // --- Funktion zum Generieren einer Notfall-Textur (Falls Datei fehlt) ---
    SDL_Surface* GenerateFallbackTileset() {
        int w = 256; int h = 256;
        SDL_Surface* surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
        if (!surf) return nullptr;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                bool gridX = (x / 16) % 2 == 0;
                bool gridY = (y / 16) % 2 == 0;

                Uint32 color;
                if (gridX ^ gridY) color = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), nullptr, 255, 0, 255, 255); // Pink
                else               color = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), nullptr, 0, 255, 0, 255);   // Grün

                // Kachel 0 (Oben links) Blau
                if (y < 16 && x < 16) color = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), nullptr, 100, 100, 255, 255);
                // Kachel 1 (Rechts daneben) Grau
                if (y < 16 && x >= 16 && x < 32) color = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), nullptr, 128, 128, 128, 255);

                // REPARATUR: SDL_WriteSurfacePixel ersetzt durch FillSurfaceRect
                SDL_Rect pixelRect = { x, y, 1, 1 };
                SDL_FillSurfaceRect(surf, &pixelRect, color);
            }
        }
        return surf;
    }

    // =========================================================
    // EDITOR STATE
    // =========================================================
    void EditorState::Init() {
       SDL_Log("--- INIT EDITOR ---");
       // Warnung behoben: expliziter Cast zu int
       if( !_font ) _font.reset( TTF_OpenFont( BasePathFont "RobotoSlab-Bold.ttf", (int)(9 * _game.scalingFactor()) ) );

       if( !_tileSet ) {
          // KORREKTUR: Der spezifische Dateiname
          const char* filename = BasePathGraphic "tiles_fantasydragon.png";
          SDL_Log("Versuche Tileset zu laden: %s", filename);

          Owned<Surface> surf(IMG_Load(filename));

          if(surf) {
              SDL_Log("Erfolg: Tileset geladen (%dx%d)", surf->w, surf->h);
          } else {
              SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "FEHLER: Konnte '%s' nicht laden! Generiere Notfall-Textur. Fehler: %s", filename, SDL_GetError());
              surf.reset( GenerateFallbackTileset() );
          }

          _tileSet.reset( SDL_CreateTextureFromSurface( renderer(), surf.get() ) );
          _tileSetSize = { surf->w, surf->h };

          // 16x16 Pixel Raster
          const int PIXEL_SIZE = 16;
          _tileSize = { PIXEL_SIZE, PIXEL_SIZE };
          _tileCount = _tileSetSize / _tileSize;
          if (_tileCount.x == 0) _tileCount.x = 1; if (_tileCount.y == 0) _tileCount.y = 1;

          SDL_SetTextureScaleMode( _tileSet.get(), SDL_ScaleMode::SDL_SCALEMODE_NEAREST );
       }

       if( _doGenerateEmptyMap ) {
          // Wir nutzen die ersten beiden Kacheln oben links für die Standard-Map
          const int FillTile = 0;
          const int BorderTile = 1;

          for( auto & row : *_currState ) { row.fill( FillTile ); *row.begin() = BorderTile; *row.rbegin() = BorderTile; }
          (*_currState->begin()).fill( BorderTile ); (*_currState->rbegin()).fill( BorderTile );
       }

       // --- Kamera Zentrieren ---
       int winW, winH;
       SDL_GetWindowSize(window(), &winW, &winH);
       _mapScale = 2;

       float mapPixelW = (float)( (*_currState)[0].size() * 16 * _mapScale );
       float mapPixelH = (float)( (*_currState).size() * 16 * _mapScale );

       _camera.x = -((winW / 2.0f) - (mapPixelW / 2.0f));
       _camera.y = -((winH / 2.0f) - (mapPixelH / 2.0f));

       _paletteScale = 1;
       _showPalette = true;
       _showGrid = true;
    }

    void EditorState::Destroy() {}

    bool EditorState::Input( const Event & evt ) {
       if (evt.type == SDL_EVENT_KEY_DOWN) {
           if (evt.key.scancode == SDL_SCANCODE_ESCAPE) { _game.ReplaceState( (u8)GameStateID::MainMenu ); return true; }
           if (evt.key.scancode == SDL_SCANCODE_TAB && evt.key.repeat == 0) _showPalette = !_showPalette;
           if (evt.key.scancode == SDL_SCANCODE_F8) SaveMapToFile("map_saved.txt", *_currState);
           if (evt.key.scancode == SDL_SCANCODE_F9) LoadMapFromFile("map_saved.txt", *_currState);
           if (evt.key.scancode == SDL_SCANCODE_F1) _mapScale = 1;
           if (evt.key.scancode == SDL_SCANCODE_F2) _mapScale = 2;
           if (evt.key.scancode == SDL_SCANCODE_F6 && evt.key.repeat == 0) _showGrid = !_showGrid;
       }
       if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
            FPoint m = { evt.button.x, evt.button.y };
            bool clickedPalette = false;
            if (_showPalette) {
                const FPoint paletteSize = toF(_tileSetSize * _paletteScale);
                if (m.x < paletteSize.x && m.y < paletteSize.y) {
                    clickedPalette = true;
                    Point p = toI(m) / (_tileSize * _paletteScale);
                    if(p.x < _tileCount.x && p.y < _tileCount.y) _pickedIdx = p;
                }
            }
            if (!clickedPalette) {
                Point p = toI(m - _camera) / (_tileSize * _mapScale);
                if(p.y >= 0 && (size_t)p.y < _currState->size() && p.x >= 0 && (size_t)p.x < (*_currState)[0].size())
                    (*_currState)[p.y][p.x] = _pickedIdx.x + _pickedIdx.y * _tileCount.x;
                _isPainting = true;
            }
       }
       if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP) { _isPainting = false; _isPanning = false; }
       if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_RIGHT) _isPanning = true;
       if (evt.type == SDL_EVENT_MOUSE_MOTION) {
           if(_isPainting) {
               FPoint m = { evt.motion.x, evt.motion.y };
               bool overPalette = _showPalette && (m.x < toF(_tileSetSize*_paletteScale).x && m.y < toF(_tileSetSize*_paletteScale).y);
               if(!overPalette) {
                   Point p = toI(m - _camera) / (_tileSize * _mapScale);
                   if(p.y >= 0 && (size_t)p.y < _currState->size() && p.x >= 0 && (size_t)p.x < (*_currState)[0].size())
                        (*_currState)[p.y][p.x] = _pickedIdx.x + _pickedIdx.y * _tileCount.x;
               }
           }
           if(_isPanning) _camera += FPoint{evt.motion.xrel, evt.motion.yrel};
       }
       return true;
    }

    void EditorState::Update( u64, Duration, f32 ) {}

    void EditorState::Render( u64 frames, Duration, f32 deltaTNeeded ) {
       const WorldState & curr = *_currState;
       FPoint mapTS = toF( _tileSize * _mapScale );

       // Map Background
       SDL_SetRenderDrawColor( renderer(), 30, 30, 30, 255 );
       FRect mapBG = toFRect( _camera, FPoint{(f32)curr[0].size(), (f32)curr.size()} * mapTS );
       SDL_RenderFillRect(renderer(), &mapBG);

       for( size_t y = 0; y < curr.size(); ++y ) {
          for( size_t x = 0; x < curr[y].size(); ++x ) {
             int idx = curr[y][x];
             Point tIdx = { idx % _tileCount.x, idx / _tileCount.x };
             FRect srcR = toFRect( toF(tIdx * _tileSize), toF(_tileSize) );
             FRect dstR = toFRect( FPoint{(f32)x, (f32)y} * mapTS + _camera, mapTS );
             SDL_RenderTexture( renderer(), _tileSet.get(), &srcR, &dstR );
          }
       }
       SDL_SetRenderDrawColor( renderer(), 255, 0, 0, 255 );
       SDL_RenderRect( renderer(), &mapBG );

       if(_showGrid) {
           SDL_SetRenderDrawColor( renderer(), 255, 255, 255, 50 ); SDL_SetRenderDrawBlendMode(renderer(), SDL_BLENDMODE_BLEND);
           for(size_t y=0; y<curr.size(); ++y) for(size_t x=0; x<curr[y].size(); ++x) {
                FRect gridR = toFRect(FPoint{(f32)x, (f32)y}*mapTS+_camera, mapTS);
                SDL_RenderRect(renderer(), &gridR);
           }
       }
       if(_showPalette) {
           FRect r = toFRect(FPoint{0,0}, toF(_tileSize*_paletteScale*_tileCount));
           SDL_SetRenderDrawColor(renderer(), 10, 10, 20, 240); SDL_SetRenderDrawBlendMode(renderer(), SDL_BLENDMODE_BLEND); SDL_RenderFillRect(renderer(), &r);
           SDL_RenderTexture(renderer(), _tileSet.get(), EntireFRect, &r);
           SDL_SetRenderDrawColor(renderer(), 255, 255, 0, 255);
           FRect pickR = toFRect(toF(_tileSize*_paletteScale*_pickedIdx), toF(_tileSize*_paletteScale));
           SDL_RenderRect(renderer(), &pickR);
       }
       std::ostringstream oss;
       oss << "Editor Mode\n[ESC] Main Menu\n[TAB] Palette\n[F8] Save [F9] Load";
       if(_font) {
           Owned<Surface> s(TTF_RenderText_Blended_Wrapped(_font.get(), oss.str().c_str(), 0, {255,255,255,255}, 800));
           if(s) {
               Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
               FPoint pos = { 10.0f, 10.0f };
               if(_showPalette) pos.x = (f32)(_tileSetSize.x * _paletteScale) + 20.0f;
               FRect textR = toFRect(pos, FPoint{(f32)s->w, (f32)s->h});
               SDL_RenderTexture(renderer(), t.get(), EntireFRect, &textR);
           }
       }
    }

    // MAIN MENU & SETTINGS
    void MainMenuState::Init() {
        if (!_fontTitle) _fontTitle.reset(TTF_OpenFont(BasePathFont "RobotoSlab-Bold.ttf", 60));
        if (!_fontMenu)  _fontMenu.reset(TTF_OpenFont(BasePathFont "RobotoSlab-Bold.ttf", 30));
    }
    bool MainMenuState::DrawButton(const char* text, float y, float mouseX, float mouseY, bool isClicked) {
        Color c = { 200, 200, 200, 255 }; bool hovered = false;
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        Owned<Surface> s(TTF_RenderText_Blended(_fontMenu.get(), text, 0, c));
        if (s) {
            float w = (float)s->w; float h = (float)s->h; float x = (winW / 2.0f) - (w / 2.0f);
            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) {
                s.reset(TTF_RenderText_Blended(_fontMenu.get(), text, 0, {255, 255, 0, 255})); hovered = true;
            }
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
            FRect btnR = toFRect(FPoint{x, y}, FPoint{w, h});
            SDL_RenderTexture(renderer(), t.get(), EntireFRect, &btnR);
        }
        return hovered && isClicked;
    }
    bool MainMenuState::Input(const Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            float mx = event.button.x; float my = event.button.y;
            int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
            float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (4 * spacing) / 2.0f + 50.0f;
            if (DrawButton("Spiel starten", startY, mx, my, true)) _game.ReplaceState((u8)GameStateID::Editor);
            else if (DrawButton("Map Creator", startY + spacing, mx, my, true)) _game.ReplaceState((u8)GameStateID::Editor);
            else if (DrawButton("Settings", startY + spacing*2, mx, my, true)) _game.PushState((u8)GameStateID::Settings);
            else if (DrawButton("Beenden", startY + spacing*3, mx, my, true)) { SDL_Event quit; quit.type = SDL_EVENT_QUIT; SDL_PushEvent(&quit); }
        }
        return true;
    }
    void MainMenuState::Render(u64, Duration, f32) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        Owned<Surface> s(TTF_RenderText_Blended(_fontTitle.get(), "FANTASY DRAGON", 0, {255, 255, 255, 255}));
        if(s) {
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
            float tx = (winW / 2.0f) - (s->w / 2.0f);
            FRect titleR = toFRect(FPoint{tx, winH * 0.15f}, FPoint{(f32)s->w, (f32)s->h});
            SDL_RenderTexture(renderer(), t.get(), EntireFRect, &titleR);
        }
        float mx, my; SDL_GetMouseState(&mx, &my);
        float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (4 * spacing) / 2.0f + 50.0f;
        DrawButton("Spiel starten", startY, mx, my, false);
        DrawButton("Map Creator", startY + spacing, mx, my, false);
        DrawButton("Settings",      startY + spacing*2, mx, my, false);
        DrawButton("Beenden",       startY + spacing*3, mx, my, false);
    }

    void SettingsState::Init() { if (!_font) _font.reset(TTF_OpenFont(BasePathFont "RobotoSlab-Bold.ttf", 30)); }
    bool SettingsState::DrawButton(const char* text, float y, float mouseX, float mouseY, bool isClicked) {
        Color c = { 200, 200, 200, 255 }; bool hovered = false;
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        Owned<Surface> s(TTF_RenderText_Blended(_font.get(), text, 0, c));
        if(s) {
            float w = (float)s->w; float h = (float)s->h; float x = (winW / 2.0f) - (w / 2.0f);
            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) {
                s.reset(TTF_RenderText_Blended(_font.get(), text, 0, {255, 255, 0, 255})); hovered = true;
            }
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
            FRect btnR = toFRect(FPoint{x, y}, FPoint{w, h});
            SDL_RenderTexture(renderer(), t.get(), EntireFRect, &btnR);
        }
        return hovered && isClicked;
    }
    bool SettingsState::Input(const Event& event) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            float mx = event.button.x; float my = event.button.y;
            int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
            float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (3 * spacing) / 2.0f;
            std::string soundText = std::string("Ton: ") + (GlobalSettings::soundEnabled ? "AN" : "AUS");
            if (DrawButton(soundText.c_str(), startY, mx, my, true)) GlobalSettings::soundEnabled = !GlobalSettings::soundEnabled;
            std::string screenText = std::string("Modus: ") + (GlobalSettings::isFullscreen ? "Vollbild" : "Fenster");
            if (DrawButton(screenText.c_str(), startY + spacing, mx, my, true)) {
                GlobalSettings::isFullscreen = !GlobalSettings::isFullscreen;
                SDL_SetWindowFullscreen(window(), GlobalSettings::isFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
            }
            if (DrawButton("Zurueck", startY + spacing*2, mx, my, true)) _game.PopState();
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) _game.PopState();
        return true;
    }
    void SettingsState::Render(u64, Duration, f32) {
        float mx, my; SDL_GetMouseState(&mx, &my);
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (3 * spacing) / 2.0f;
        std::string soundText = std::string("Ton: ") + (GlobalSettings::soundEnabled ? "AN" : "AUS");
        DrawButton(soundText.c_str(), startY, mx, my, false);
        std::string screenText = std::string("Modus: ") + (GlobalSettings::isFullscreen ? "Vollbild" : "Fenster");
        DrawButton(screenText.c_str(), startY + spacing, mx, my, false);
        DrawButton("Zurueck", startY + spacing*2, mx, my, false);
    }
}