#include "fantasydragon_mapeditor.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <cmath>

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
    // GLOBALE SETTINGS INIT
    int GlobalSettings::musicVolume = 64;
    bool GlobalSettings::isFullscreen = false;
    bool GlobalSettings::isEditorMode = true;

    static std::string g_PendingGameMap = "";

    // --- Hilfsfunktionen für Datei I/O ---
    void SaveMapToFile(const std::string& filename, const MapType& map) {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (const auto& row : map) {
                for (size_t i = 0; i < row.size(); ++i) file << row[i] << (i < row.size() - 1 ? " " : "");
                file << "\n";
            }
            file.close();
            SDL_Log("Map gespeichert: %s", filename.c_str());
        }
    }

    bool LoadMapFromFile(const std::string& filename, MapType& map) {
        std::ifstream file(filename);
        if (file.is_open()) {
            for (auto& row : map) for (auto& cell : row) file >> cell;
            file.close();
            SDL_Log("Map geladen: %s", filename.c_str());
            return true;
        }
        return false;
    }

    void SDLCALL OnMapSave(void* userdata, const char* const* filelist, int filter) {
        if (!filelist || !filelist[0]) return;
        auto* map = static_cast<MapType*>(userdata);
        if(map) SaveMapToFile(filelist[0], *map);
    }

    void SDLCALL OnMapLoad(void* userdata, const char* const* filelist, int filter) {
        if (!filelist || !filelist[0]) return;
        auto* map = static_cast<MapType*>(userdata);
        if(map) LoadMapFromFile(filelist[0], *map);
    }

    void SDLCALL OnSelectMapForGame(void* userdata, const char* const* filelist, int filter) {
        if (!filelist || !filelist[0]) return;
        g_PendingGameMap = std::string(filelist[0]);
        GlobalSettings::isEditorMode = false;
        auto* game = static_cast<EditorGameBase*>(userdata);
        if(game) game->ReplaceState((u8)GameStateID::Editor);
    }

    SDL_Surface* GenerateFallbackTileset() {
        int w = 256; int h = 256;
        SDL_Surface* surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
        if (!surf) return nullptr;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                bool gridX = (x / 16) % 2 == 0; bool gridY = (y / 16) % 2 == 0;
                Uint32 color = (gridX ^ gridY) ? SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), nullptr, 255, 0, 255, 255)
                                               : SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), nullptr, 0, 255, 0, 255);
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
       SDL_Log("--- INIT STATE (EditorMode: %d) ---", GlobalSettings::isEditorMode);
       if( !_font ) _font.reset( TTF_OpenFont( BasePathFont "RobotoSlab-Bold.ttf", (int)(9 * _game.scalingFactor()) ) );

       if( !_tileSet ) {
          const char* filename = BasePathGraphic "tiles_fantasydragon.png";
          Owned<Surface> surf(IMG_Load(filename));
          if(!surf) {
              SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Bild fehlt: %s", filename);
              surf.reset( GenerateFallbackTileset() );
          }
          _tileSet.reset( SDL_CreateTextureFromSurface( renderer(), surf.get() ) );
          _tileSetSize = { surf->w, surf->h };

          const int PIXEL_SIZE = 16;
          _tileSize = { PIXEL_SIZE, PIXEL_SIZE };
          _tileCount = _tileSetSize / _tileSize;
          if (_tileCount.x == 0) _tileCount.x = 1; if (_tileCount.y == 0) _tileCount.y = 1;

          SDL_SetTextureScaleMode( _tileSet.get(), SDL_ScaleMode::SDL_SCALEMODE_NEAREST );
       }

       if( _doGenerateEmptyMap ) {
          const int FillTile = 0; const int BorderTile = 0;
          for( auto & row : *_currState ) { row.fill( FillTile ); *row.begin() = BorderTile; *row.rbegin() = BorderTile; }
          (*_currState->begin()).fill( BorderTile ); (*_currState->rbegin()).fill( BorderTile );
       }

       if (!GlobalSettings::isEditorMode) {
           _player.Init(renderer());
           _bee.Init(renderer(), 300, 200);
           _mapScale = 2;
       } else {
           _mapScale = 2;
       }

       int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
       float mapPixelW = (float)( (*_currState)[0].size() * 16 * _mapScale );
       float mapPixelH = (float)( (*_currState).size() * 16 * _mapScale );
       _camera.x = -((winW / 2.0f) - (mapPixelW / 2.0f));
       _camera.y = -((winH / 2.0f) - (mapPixelH / 2.0f));

       _paletteScale = 1;

       if (GlobalSettings::isEditorMode) {
           _showPalette = true;
           _showGrid = true;
       } else {
           _showPalette = false;
           _showGrid = false;
           if (!g_PendingGameMap.empty()) {
               LoadMapFromFile(g_PendingGameMap, *_currState);
               g_PendingGameMap = "";
           }
       }

       _pickedSize = { 1, 1 };
       _pickedIdx = { 0, 0 };
       _isSelectingPalette = false;
    }

    void EditorState::Destroy() {}

    bool EditorState::Input( const Event & evt ) {
       const char* defaultPath = "C:\\Users\\frieb\\CLionProjects\\fantasy_dragon\\asset\\map\\";

       if (evt.type == SDL_EVENT_KEY_DOWN) {
           if (evt.key.scancode == SDL_SCANCODE_ESCAPE) { _game.ReplaceState( (u8)GameStateID::MainMenu ); return true; }

           if (GlobalSettings::isEditorMode) {
               if (evt.key.scancode == SDL_SCANCODE_TAB && evt.key.repeat == 0) _showPalette = !_showPalette;
               if (evt.key.scancode == SDL_SCANCODE_F8) SDL_ShowSaveFileDialog(OnMapSave, _currState, window(), nullptr, 0, defaultPath);
               if (evt.key.scancode == SDL_SCANCODE_F9) SDL_ShowOpenFileDialog(OnMapLoad, _currState, window(), nullptr, 0, defaultPath, false);
               if (evt.key.scancode == SDL_SCANCODE_F1) _mapScale = 1;
               if (evt.key.scancode == SDL_SCANCODE_F2) _mapScale = 2;
               if (evt.key.scancode == SDL_SCANCODE_F6 && evt.key.repeat == 0) _showGrid = !_showGrid;
           }
       }

       if (!GlobalSettings::isEditorMode) {
           _player.Input(evt);
       }

       if (GlobalSettings::isEditorMode && evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
            FPoint m = { (f32)evt.button.x, (f32)evt.button.y };
            bool clickedInsidePalette = false;

            if (_showPalette) {
                const FPoint paletteSize = toF(_tileSetSize * _paletteScale);
                if (m.x < paletteSize.x && m.y < paletteSize.y) {
                    clickedInsidePalette = true;
                    Point p = toI(m) / (_tileSize * _paletteScale);
                    if(p.x < _tileCount.x && p.y < _tileCount.y) {
                        _isSelectingPalette = true;
                        _selectionStart = p;
                        _pickedIdx = p;
                        _pickedSize = { 1, 1 };
                    }
                }
            }

            if (!clickedInsidePalette) {
                _isPainting = true;
                Point p = toI(m - _camera) / (_tileSize * _mapScale);
                if(p.y >= 0 && (size_t)p.y < _currState->size() && p.x >= 0 && (size_t)p.x < (*_currState)[0].size()) {
                     for(int py = 0; py < _pickedSize.y; ++py) {
                         for(int px = 0; px < _pickedSize.x; ++px) {
                             int targetX = p.x + px; int targetY = p.y + py;
                             if(targetY >= 0 && (size_t)targetY < _currState->size() && targetX >= 0 && (size_t)targetX < (*_currState)[0].size()) {
                                 int tileIdxX = _pickedIdx.x + px; int tileIdxY = _pickedIdx.y + py;
                                 if (tileIdxX < _tileCount.x && tileIdxY < _tileCount.y) {
                                     (*_currState)[targetY][targetX] = tileIdxX + tileIdxY * _tileCount.x;
                                 }
                             }
                         }
                     }
                }
            }
       }

       if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP) { _isPainting = false; _isPanning = false; _isSelectingPalette = false; }
       if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_RIGHT) _isPanning = true;

       if (evt.type == SDL_EVENT_MOUSE_MOTION) {
           FPoint m = { (f32)evt.motion.x, (f32)evt.motion.y };
           if (GlobalSettings::isEditorMode && _isSelectingPalette && _showPalette) {
                Point currP = toI(m) / (_tileSize * _paletteScale);
                if (currP.x >= _tileCount.x) currP.x = _tileCount.x - 1; if (currP.y >= _tileCount.y) currP.y = _tileCount.y - 1;
                if (currP.x < 0) currP.x = 0; if (currP.y < 0) currP.y = 0;
                int x1 = std::min(_selectionStart.x, currP.x); int y1 = std::min(_selectionStart.y, currP.y);
                int x2 = std::max(_selectionStart.x, currP.x); int y2 = std::max(_selectionStart.y, currP.y);
                _pickedIdx = { x1, y1 }; _pickedSize = { x2 - x1 + 1, y2 - y1 + 1 };
           }
           if(GlobalSettings::isEditorMode && _isPainting) {
               bool overPalette = _showPalette && (m.x < toF(_tileSetSize*_paletteScale).x && m.y < toF(_tileSetSize*_paletteScale).y);
               if(!overPalette && !_isSelectingPalette) {
                   Point p = toI(m - _camera) / (_tileSize * _mapScale);
                   for(int py = 0; py < _pickedSize.y; ++py) {
                         for(int px = 0; px < _pickedSize.x; ++px) {
                             int targetX = p.x + px; int targetY = p.y + py;
                             if(targetY >= 0 && (size_t)targetY < _currState->size() && targetX >= 0 && (size_t)targetX < (*_currState)[0].size()) {
                                 int tileIdxX = _pickedIdx.x + px; int tileIdxY = _pickedIdx.y + py;
                                 if (tileIdxX < _tileCount.x && tileIdxY < _tileCount.y) (*_currState)[targetY][targetX] = tileIdxX + tileIdxY * _tileCount.x;
                             }
                         }
                   }
               }
           }
           if(_isPanning) _camera += FPoint{evt.motion.xrel, evt.motion.yrel};
       }
       return true;
    }

    void EditorState::Update( u64, Duration, f32 deltaT ) {
        if (!GlobalSettings::isEditorMode) {
            _player.Update(deltaT, *_currState);
            _bee.Update(deltaT, _player);

            if (_player.isAttacking && _player.currentFrame >= 2 && _player.currentFrame <= 4) {
                FRect swordBox = _player.GetAttackHitbox();
                FRect beeBox = _bee.GetHitbox();
                if (SDL_HasRectIntersectionFloat(&swordBox, &beeBox)) {
                    if (_bee.z < 40) {
                        _bee.TakeDamage(10);
                    }
                }
            }

            int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
            float targetCamX = -((_player.position.x * _mapScale) - (winW / 2.0f));
            float targetCamY = -((_player.position.y * _mapScale) - (winH / 2.0f));
            _camera.x += (targetCamX - _camera.x) * 5.0f * deltaT;
            _camera.y += (targetCamY - _camera.y) * 5.0f * deltaT;
        }
    }

    void EditorState::Render( u64 frames, Duration, f32 deltaTNeeded ) {
       const WorldState & curr = *_currState;
       FPoint mapTS = toF( _tileSize * _mapScale );

       SDL_SetRenderDrawColor( renderer(), 30, 30, 30, 255 );
       FRect mapBG = toFRect( _camera, FPoint{(f32)curr[0].size(), (f32)curr.size()} * mapTS );
       SDL_RenderFillRect(renderer(), &mapBG);

       for( size_t y = 0; y < curr.size(); ++y ) {
          for( size_t x = 0; x < curr[y].size(); ++x ) {
             int idx = curr[y][x];
             if (idx == 0) continue;
             Point tIdx = { idx % _tileCount.x, idx / _tileCount.x };
             FRect srcR = toFRect( toF(tIdx * _tileSize), toF(_tileSize) );
             FRect dstR = toFRect( FPoint{(f32)x, (f32)y} * mapTS + _camera, mapTS );
             SDL_RenderTexture( renderer(), _tileSet.get(), &srcR, &dstR );
          }
       }

       if (!GlobalSettings::isEditorMode) {
           if (_player.position.y < _bee.position.y) {
               _player.Render(renderer(), _camera, _mapScale);
               _bee.Render(renderer(), _camera, _mapScale);
           } else {
               _bee.Render(renderer(), _camera, _mapScale);
               _player.Render(renderer(), _camera, _mapScale);
           }
       }

       if(GlobalSettings::isEditorMode) {
           SDL_SetRenderDrawColor( renderer(), 255, 0, 0, 255 );
           SDL_RenderRect( renderer(), &mapBG );
       }

       if(GlobalSettings::isEditorMode && _showGrid) {
           SDL_SetRenderDrawColor( renderer(), 255, 255, 255, 50 ); SDL_SetRenderDrawBlendMode(renderer(), SDL_BLENDMODE_BLEND);
           for(size_t y=0; y<curr.size(); ++y) for(size_t x=0; x<curr[y].size(); ++x) {
                FRect gridR = toFRect(FPoint{(f32)x, (f32)y}*mapTS+_camera, mapTS);
                SDL_RenderRect(renderer(), &gridR);
           }
       }

       if(GlobalSettings::isEditorMode) {
           float mx, my; SDL_GetMouseState(&mx, &my);
           FPoint m = { mx, my };
           bool overPalette = _showPalette && (m.x < toF(_tileSetSize*_paletteScale).x && m.y < toF(_tileSetSize*_paletteScale).y);

           if (!overPalette && !_isSelectingPalette) {
               Point p = toI(m - _camera) / (_tileSize * _mapScale);
               if(p.y >= 0 && (size_t)p.y < curr.size() && p.x >= 0 && (size_t)p.x < curr[0].size()) {
                   SDL_SetTextureAlphaMod(_tileSet.get(), 150);
                   for(int py = 0; py < _pickedSize.y; ++py) {
                       for(int px = 0; px < _pickedSize.x; ++px) {
                           int tileIdxX = _pickedIdx.x + px; int tileIdxY = _pickedIdx.y + py;
                           if (tileIdxX < _tileCount.x && tileIdxY < _tileCount.y) {
                               FRect srcR = toFRect( toF(Point{tileIdxX, tileIdxY} * _tileSize), toF(_tileSize) );
                               FRect dstR = toFRect( FPoint{(f32)(p.x + px), (f32)(p.y + py)} * mapTS + _camera, mapTS );
                               SDL_RenderTexture( renderer(), _tileSet.get(), &srcR, &dstR );
                           }
                       }
                   }
                   SDL_SetTextureAlphaMod(_tileSet.get(), 255);
               }
           }
       }

       if(GlobalSettings::isEditorMode && _showPalette) {
           FRect r = toFRect(FPoint{0,0}, toF(_tileSize*_paletteScale*_tileCount));
           SDL_SetRenderDrawColor(renderer(), 10, 10, 20, 240); SDL_SetRenderDrawBlendMode(renderer(), SDL_BLENDMODE_BLEND); SDL_RenderFillRect(renderer(), &r);
           SDL_RenderTexture(renderer(), _tileSet.get(), EntireFRect, &r);
           SDL_SetRenderDrawColor(renderer(), 255, 255, 0, 255);
           FPoint selectSize = toF(_tileSize * _paletteScale * _pickedSize);
           FRect pickR = toFRect(toF(_tileSize * _paletteScale * _pickedIdx), selectSize);
           SDL_RenderRect(renderer(), &pickR);
       }

       if(_font && GlobalSettings::isEditorMode) {
           std::ostringstream oss;
           oss << "Editor Mode\n[ESC] Main Menu\n[TAB] Palette\n [F1] Map verkleinern\n [F2] Map vergrößern\n [F6] Grid aus/an \n[F8] Save [F9] Load";
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

    // =========================================================
    // MAIN MENU STATE
    // =========================================================
    void MainMenuState::Init() {
        if (!_fontTitle) _fontTitle.reset(TTF_OpenFont(BasePathFont "RobotoSlab-Bold.ttf", 60));
        if (!_fontMenu)  _fontMenu.reset(TTF_OpenFont(BasePathFont "RobotoSlab-Bold.ttf", 30));

        if (!_background) {
            const char* filename = BasePathGraphic "menu_background.png";
            auto* surf = IMG_Load(filename);
            if (surf) {
                _background.reset(SDL_CreateTextureFromSurface(renderer(), surf));
                SDL_DestroySurface(surf);
            }
        }

        // --- AUDIO FIX ---
        // 1. SDL Audio Subsystem initialisieren (WICHTIG!)
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            SDL_Log("SDL_Init(AUDIO) fehlgeschlagen: %s", SDL_GetError());
        }

        // 2. Mixer öffnen mit 'nullptr' Spec für Auto-Detect
        if (Mix_OpenAudio(0, nullptr) == 0) { // 0 Success in SDL3 Mixer
            const char* musicFile = BasePathAudio "hauptmenu_sound.wav"; // WAV nehmen, da MP3 oft DLLs fehlt
            _bgMusic = Mix_LoadMUS(musicFile);
            if (_bgMusic) {
                Mix_VolumeMusic(GlobalSettings::musicVolume);
                Mix_PlayMusic(_bgMusic, -1);
                SDL_Log("Musik gestartet: %s", musicFile);
            } else {
                SDL_Log("Fehler beim Laden der Musik: %s -> %s", musicFile, SDL_GetError());
            }
        } else {
            SDL_Log("Mixer OpenAudio fehlgeschlagen: %s", SDL_GetError());
        }
    }

    void MainMenuState::Destroy() {
        if (_bgMusic) {
            Mix_HaltMusic();
            Mix_FreeMusic(_bgMusic);
            _bgMusic = nullptr;
        }
        Mix_CloseAudio();
        Mix_Quit();
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
        const char* defaultPath = "C:\\Users\\frieb\\CLionProjects\\fantasy_dragon\\asset\\map\\";
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            float mx = (float)event.button.x; float my = (float)event.button.y;
            int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
            float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (4 * spacing) / 2.0f + 50.0f;

            if (DrawButton("Spiel starten", startY, mx, my, true)) {
                SDL_ShowOpenFileDialog(OnSelectMapForGame, &_game, window(), nullptr, 0, defaultPath, false);
            }
            else if (DrawButton("Map Creator", startY + spacing, mx, my, true)) {
                GlobalSettings::isEditorMode = true; _game.ReplaceState((u8)GameStateID::Editor);
            }
            else if (DrawButton("Settings", startY + spacing*2, mx, my, true)) _game.PushState((u8)GameStateID::Settings);
            else if (DrawButton("Beenden", startY + spacing*3, mx, my, true)) { SDL_Event quit; quit.type = SDL_EVENT_QUIT; SDL_PushEvent(&quit); }
        }
        return true;
    }

    void MainMenuState::Render(u64, Duration, f32) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);

        if (_background) {
            SDL_RenderTexture(renderer(), _background.get(), nullptr, nullptr);
        } else {
            SDL_SetRenderDrawColor(renderer(), 30, 30, 40, 255);
            SDL_RenderClear(renderer());
        }

        Owned<Surface> s(TTF_RenderText_Blended(_fontTitle.get(), " ", 0, {255, 255, 255, 255}));
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

    // =========================================================
    // SETTINGS STATE
    // =========================================================
    void SettingsState::Init() {
        if (!_font) _font.reset(TTF_OpenFont(BasePathFont "RobotoSlab-Bold.ttf", 30));

        if (!_background) {
            const char* filename = BasePathGraphic "menu_settings.png";
            auto* surf = IMG_Load(filename);
            if (surf) {
                _background.reset(SDL_CreateTextureFromSurface(renderer(), surf));
                SDL_DestroySurface(surf);
            }
        }
    }

    bool SettingsState::DrawSlider(const char* label, float y, float mouseX, float mouseY, bool isMouseDown) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);

        float sliderWidth = 300.0f;
        float sliderHeight = 10.0f;
        float sliderX = (winW / 2.0f) - (sliderWidth / 2.0f);
        float sliderY = y + 40.0f;

        Owned<Surface> s(TTF_RenderText_Blended(_font.get(), label, 0, {200, 200, 200, 255}));
        if(s) {
            float tw = (float)s->w; float th = (float)s->h;
            FRect tRect = { (winW / 2.0f) - (tw/2.0f), y, tw, th };
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
            SDL_RenderTexture(renderer(), t.get(), nullptr, &tRect);
        }

        FRect track = { sliderX, sliderY, sliderWidth, sliderHeight };
        SDL_SetRenderDrawColor(renderer(), 100, 100, 100, 255);
        SDL_RenderFillRect(renderer(), &track);

        float pct = (float)GlobalSettings::musicVolume / 128.0f;
        float knobX = sliderX + (pct * sliderWidth);
        FRect knob = { knobX - 10.0f, sliderY - 5.0f, 20.0f, 20.0f };

        SDL_SetRenderDrawColor(renderer(), 255, 200, 0, 255);
        SDL_RenderFillRect(renderer(), &knob);

        if (isMouseDown) {
            if (mouseX >= sliderX && mouseX <= sliderX + sliderWidth &&
                mouseY >= sliderY - 20.0f && mouseY <= sliderY + 30.0f)
            {
                float newPct = (mouseX - sliderX) / sliderWidth;
                if (newPct < 0) newPct = 0;
                if (newPct > 1) newPct = 1;

                int newVol = (int)(newPct * 128.0f);
                if (newVol != GlobalSettings::musicVolume) {
                    GlobalSettings::musicVolume = newVol;
                    Mix_VolumeMusic(newVol);
                    return true;
                }
            }
        }
        return false;
    }

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
        float mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        bool isDown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);

        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        float centerY = winH / 2.0f; float spacing = 80.0f; float startY = centerY - (3 * spacing) / 2.0f;

        if (isDown) {
            std::string volTxt = "Lautstärke: " + std::to_string((int)((GlobalSettings::musicVolume / 128.0f) * 100)) + "%";
            DrawSlider(volTxt.c_str(), startY, mx, my, true);
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            float clickX = (float)event.button.x;
            float clickY = (float)event.button.y;

            std::string screenText = std::string("Modus: ") + (GlobalSettings::isFullscreen ? "Vollbild" : "Fenster");
            if (DrawButton(screenText.c_str(), startY + spacing, clickX, clickY, true)) {
                GlobalSettings::isFullscreen = !GlobalSettings::isFullscreen;
                SDL_SetWindowFullscreen(window(), GlobalSettings::isFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
            }
            if (DrawButton("Zurück", startY + spacing*2, clickX, clickY, true)) _game.PopState();
        }

        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) _game.PopState();
        return true;
    }

    void SettingsState::Render(u64, Duration, f32) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);

        if (_background) {
            SDL_RenderTexture(renderer(), _background.get(), nullptr, nullptr);
        } else {
            SDL_SetRenderDrawColor(renderer(), 40, 30, 30, 255);
            SDL_RenderClear(renderer());
        }

        float mx, my; SDL_GetMouseState(&mx, &my);
        float centerY = winH / 2.0f; float spacing = 80.0f; float startY = centerY - (3 * spacing) / 2.0f;

        std::string volTxt = "Lautstärke: " + std::to_string((int)((GlobalSettings::musicVolume / 128.0f) * 100)) + "%";
        DrawSlider(volTxt.c_str(), startY, mx, my, false);

        std::string screenText = std::string("Modus: ") + (GlobalSettings::isFullscreen ? "Vollbild" : "Fenster");
        DrawButton(screenText.c_str(), startY + spacing, mx, my, false);

        DrawButton("Zurück", startY + spacing*2, mx, my, false);
    }
}