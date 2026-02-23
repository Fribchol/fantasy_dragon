#include "fantasydragon_mapeditor.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>

// Für PI (Winkelberechnung)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    using Fireball = EditorState::FireballProjectile;

    int GlobalSettings::musicVolume = 64;
    int GlobalSettings::sfxVolume = 80;
    bool GlobalSettings::isFullscreen = false;
    bool GlobalSettings::isEditorMode = true;

    static std::string g_PendingGameMap = "";

    std::string GetAssetPath(const std::string& subPath) {
        return "asset/" + subPath;
    }

    void EditorState::PlaySFX(const std::string& name, int loops) {
        if (_sfx.count(name) == 0 || _sfx[name] == nullptr) {
            SDL_Log("FEHLER: Sound %s ist nicht geladen oder nicht im Ordner!", name.c_str());
            return;
        }

        // Anti-Spam Cooldown für den Hit-Sound
        if (name == "Player_Hit.wav" || name == "Player_Hit.mp3") {
            static auto lastHitTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHitTime).count() < 400) {
                return;
            }
            lastHitTime = now;
        }

        Mix_VolumeChunk(_sfx[name], GlobalSettings::sfxVolume);

        int channel = Mix_PlayChannel(-1, _sfx[name], loops);

        if (name == "Player_Death.wav" || name == "Player_Death.mp3") {
            SDL_Log(">>> SOUND: Death wurde auf Kanal %d abgespielt <<<", channel);
        } else if (name == "Player_Hit.wav" || name == "Player_Hit.mp3") {
            SDL_Log(">>> SOUND: Hit wurde auf Kanal %d abgespielt <<<", channel);
        }

        if (name == "bee.mp3") {
            _beeChannel = channel;
        }
    }

    void SaveMapToFile(const std::string& filename, const EditorState::WorldState& layers) {
        std::ofstream file(filename);
        if (file.is_open()) {
            for (int l = 0; l < 3; ++l) {
                file << "[Layer" << l << "]\n";
                for (const auto& row : layers[l]) {
                    for (size_t i = 0; i < row.size(); ++i) file << row[i] << (i < row.size() - 1 ? " " : "");
                    file << "\n";
                }
            }
            file.close();
            SDL_Log("Map (3 Layer) gespeichert: %s", filename.c_str());
        }
    }

    bool LoadMapFromFile(const std::string& filename, EditorState::WorldState& layers) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            int currentLayer = -1;
            int rowIdx = 0;
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                if (line.find("[Layer") != std::string::npos) { currentLayer++; rowIdx = 0; continue; }
                if (currentLayer >= 0 && currentLayer < 3 && rowIdx < (int)layers[0].size()) {
                    std::stringstream ss(line);
                    for (size_t i = 0; i < layers[0][0].size(); ++i) {
                        if (!(ss >> layers[currentLayer][rowIdx][i])) { layers[currentLayer][rowIdx][i] = 0; }
                    }
                    rowIdx++;
                }
            }
            file.close();
            SDL_Log("Map (3 Layer) geladen: %s", filename.c_str());
            return true;
        }
        return false;
    }

    void SDLCALL OnMapSave(void* userdata, const char* const* filelist, int filter) {
        if (!filelist || !filelist[0]) return;
        auto* layers = static_cast<EditorState::WorldState*>(userdata);
        if(layers) SaveMapToFile(filelist[0], *layers);
    }

    void SDLCALL OnMapLoad(void* userdata, const char* const* filelist, int filter) {
        if (!filelist || !filelist[0]) return;
        auto* layers = static_cast<EditorState::WorldState*>(userdata);
        if(layers) LoadMapFromFile(filelist[0], *layers);
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

    namespace
    {
        std::string FindTemplateFolderString() {
            return GetAssetPath("magic_templates");
        }

        constexpr int kFlipH = 1 << 30;
        constexpr int kFlipV = 1 << 29;
        constexpr int kRotMask = 3 << 27; // 2 bits
        constexpr int kRotShift = 27;
        constexpr int kTileIdMask = ~(kFlipH | kFlipV | kRotMask);

        static void MapSelectionToSource(int dx, int dy,
                                         int selW, int selH,
                                         int rotSteps, bool flipH, bool flipV,
                                         int& outX, int& outY)
        {
            const int r = rotSteps & 3;
            const bool swap = (r % 2) != 0;
            int effW = swap ? selH : selW;
            int effH = swap ? selW : selH;

            int rx = flipH ? (effW - 1 - dx) : dx;
            int ry = flipV ? (effH - 1 - dy) : dy;

            switch (r) {
                case 1: // 90
                    outX = (selW - 1 - ry);
                    outY = rx;
                    break;
                case 2: // 180
                    outX = (selW - 1 - rx);
                    outY = (selH - 1 - ry);
                    break;
                case 3: // 270
                    outX = ry;
                    outY = (selH - 1 - rx);
                    break;
                default: // 0
                    outX = rx;
                    outY = ry;
                    break;
            }
        }

        static bool BuildChestHitboxFromMap(const EditorState::WorldState& layers,
                                            int layerIndex,
                                            const Point& tileCount,
                                            FRect& outHitbox)
        {
            if (tileCount.x <= 0 || tileCount.y <= 0) return false;
            if (layerIndex < 0 || layerIndex >= (int)layers.size()) return false;

            const int tilesetCols = tileCount.x;
            const int chestX0 = 18;
            const int chestY0 = 19;
            const int chestX1 = 19;
            const int chestY1 = 20;

            const int chestId00 = chestX0 + chestY0 * tilesetCols;
            const int chestId01 = chestX0 + chestY1 * tilesetCols;
            const int chestId10 = chestX1 + chestY0 * tilesetCols;
            const int chestId11 = chestX1 + chestY1 * tilesetCols;

            const auto& map = layers[layerIndex];
            bool found = false;
            int minX = 999999, minY = 999999;
            int maxX = -1, maxY = -1;

            for (int y = 0; y < (int)map.size(); ++y) {
                for (int x = 0; x < (int)map[y].size(); ++x) {
                    const int id = map[y][x] & kTileIdMask;
                    if (id == chestId00 || id == chestId01 || id == chestId10 || id == chestId11) {
                        found = true;
                        if (x < minX) minX = x;
                        if (y < minY) minY = y;
                        if (x > maxX) maxX = x;
                        if (y > maxY) maxY = y;
                    }
                }
            }

            if (!found) return false;

            const float tileSize = 16.0f;
            outHitbox.x = (float)minX * tileSize;
            outHitbox.y = (float)minY * tileSize;
            outHitbox.w = (float)(maxX - minX + 1) * tileSize;
            outHitbox.h = (float)(maxY - minY + 1) * tileSize;
            return true;
        }

        static JanSordid::SDL::FRect CircleToRect(const JanSordid::SDL::FPoint& center, float radius) {
            return JanSordid::SDL::FRect{ center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f };
        }

        static void SpawnFireball(std::vector<Fireball>& list, const Player& player) {
            Fireball p;
            const float dir = player.facingRight ? 1.0f : -1.0f;
            p.pos = player.position;
            p.pos.x += dir * 20.0f;
            p.startPos = p.pos;
            p.vel = { dir * 420.0f, 0.0f };
            p.radius = 10.0f;
            p.lifetime = 1.2f;
            p.animTime = 0.0f;
            p.alive = true;
            list.push_back(p);
        }

        static void SpawnExplosion(std::vector<EditorState::ExplosionAnim>& list, const FPoint& pos) {
            EditorState::ExplosionAnim e;
            e.pos = pos;
            e.animTime = 0.0f;
            e.alive = true;
            list.push_back(e);
        }

        static void SpawnHeal(std::vector<EditorState::HealAnim>& list, const FPoint& pos) {
            EditorState::HealAnim h;
            h.pos = pos;
            h.animTime = 0.0f;
            h.alive = true;
            list.push_back(h);
        }

        static bool AreAllEnemiesDead(const std::vector<Bee>& bees, const std::vector<Mushroom>& mushrooms) {
            const bool beesDead = std::all_of(bees.begin(), bees.end(),
                                              [](const Bee& b) { return b.state == BeeState::Dead; });
            const bool mushDead = std::all_of(mushrooms.begin(), mushrooms.end(),
                                              [](const Mushroom& m) { return m.state == MushroomState::Dead; });
            return beesDead && mushDead;
        }

        void ApplyMagicResult(FD::Magic::MagicResult res,
                              Player& player,
                              std::vector<Bee>& bees,
                              std::vector<Fireball>& fireballs,
                              std::vector<EditorState::HealAnim>& heals,
                              EditorState* state)
        {
            switch (res) {
                case FD::Magic::MagicResult::Fireball:
                    SDL_Log(">>> CAST: FIREBALL <<<");
                    state->PlaySFX("Fireball_fly.mp3", 0);
                    SpawnFireball(fireballs, player);
                    break;
                case FD::Magic::MagicResult::Heal:
                    SDL_Log(">>> CAST: HEAL <<<");
                    player.hp = std::min(player.hp + 25, player.maxHp);
                    SpawnHeal(heals, FPoint{player.position.x + (player.size.x * 0.5f),
                                            player.position.y - player.size.y});
                    break;
                case FD::Magic::MagicResult::Fail:
                    SDL_Log(">>> CAST: FAIL (Nicht erkannt) <<<");
                    break;
                default: break;
            }
        }
    }

    void EditorState::Init() {
       int echteBreite = (int)(*_currState)[0][0].size();
       SDL_Log("--- INIT STATE ---");

       Mix_AllocateChannels(32);

       std::vector<std::string> sfxFiles = {
           "Bee_Sting.mp3", "Blade_swing_01.mp3", "Blade_swing_02.mp3", "Blade_swing_03.mp3",
           "Fireball_explosion.mp3", "Fireball_fly.mp3", "Mob_hit.mp3", "Mushroom_atk.mp3",
           "Player_Death.wav", "Player_Hit.wav", "jump.wav", "bee.mp3",
           "Player_Death.mp3", "Player_Hit.mp3", "jump.mp3"
       };
       for (const auto& f : sfxFiles) {
           std::string path = GetAssetPath(BasePathAudio + f);
           Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
           if (chunk) _sfx[f] = chunk;
       }

       std::string fontP = GetAssetPath(BasePathFont "RobotoSlab-Bold.ttf");
       if( !_font ) _font.reset( TTF_OpenFont( fontP.c_str(), (int)(9 * _game.scalingFactor()) ) );

       if( !_tileSet ) {
          std::string tileP = GetAssetPath(BasePathGraphic "tiles_fantasydragon.png");
          Owned<Surface> surf(IMG_Load(tileP.c_str()));
          if(!surf) {
              SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Bild fehlt: %s", tileP.c_str());
              surf.reset( GenerateFallbackTileset() );
          } else {
              SDL_Log("Bild geladen: %s", tileP.c_str());
          }
          _tileSet.reset( SDL_CreateTextureFromSurface( renderer(), surf.get() ) );
          _tileSetSize = { surf->w, surf->h };
          const int PIXEL_SIZE = 16;
          _tileSize = { PIXEL_SIZE, PIXEL_SIZE };
          _tileCount = _tileSetSize / _tileSize;
          if (_tileCount.x == 0) _tileCount.x = 1; if (_tileCount.y == 0) _tileCount.y = 1;
          SDL_SetTextureScaleMode( _tileSet.get(), SDL_ScaleMode::SDL_SCALEMODE_NEAREST );
       }

       auto LoadTex = [&](Owned<Texture>& target, const std::string& path) {
           auto* surf = IMG_Load(GetAssetPath(path).c_str());
           if (surf) { target.reset(SDL_CreateTextureFromSurface(renderer(), surf)); SDL_SetTextureBlendMode(target.get(), SDL_BLENDMODE_BLEND); }
       };

       LoadTex(_texFireball, "magic/Fire/Fire I/Fire_I_16x16.png");
       LoadTex(_texExplosion, "magic/Fire/Bomb/Fire_Bomb_Explosion_96x48.png");
       LoadTex(_texHeal, "magic/Holy/Buff/Holy_Cross_Front_48x64.png");
       LoadTex(_uiFrame, "gui/uf2_frame.png");
       LoadTex(_uiHpFill, "gui/uf2_fill_green.png");
       LoadTex(_uiManaFill, "gui/uf2_fill_blue.png");
       LoadTex(_enemyHpFrame, "gui/uf3_frame.png");
       LoadTex(_enemyHpFill, "gui/uf3_fill_red.png");

       if( _doGenerateEmptyMap ) {
          const int FillTile = 0;
          for(int l = 0; l < 3; ++l) {
              for( auto & row : (*_currState)[l] ) { row.fill( FillTile ); }
          }
       }

       if (!GlobalSettings::isEditorMode) {
           _player.Init(renderer());

           _bees.clear();
           _mushrooms.clear();

           auto SpawnBee = [&](float x, float y) {
               Bee b;
               b.Init(renderer(), x, y);
               _bees.push_back(std::move(b));
           };

           auto SpawnMushroom = [&](float x, float y) {
               Mushroom m;
               m.Init(renderer(), x, y);
               _mushrooms.push_back(std::move(m));
           };

           SpawnBee(1200.0f, 220.0f);
           SpawnMushroom(1400.0f, 240.0f);
           SpawnBee(3000.0f, 150.0f);
           SpawnBee(3080.0f, 250.0f);
           SpawnBee(3150.0f, 180.0f);
           SpawnMushroom(3300.0f, 240.0f);
           SpawnBee(6200.0f, 120.0f);
           SpawnBee(6280.0f, 280.0f);
           SpawnBee(6350.0f, 200.0f);
           SpawnBee(6420.0f, 150.0f);
           SpawnBee(6500.0f, 250.0f);
           SpawnMushroom(6600.0f, 240.0f);

           int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
           _magic.SetCanvasRect((winW - 256) / 2, (winH - 256) / 2, 256, 256);

           std::string templateFolder = FindTemplateFolderString();
           namespace fs = std::filesystem;
           if (fs::exists(templateFolder)) {
               _magic.LoadTemplatesFromFolder(templateFolder.c_str());
           }
           _mapScale = 2;
       } else {
           _mapScale = 2;
       }

       int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
       float mapPixelH = (float)( (*_currState)[0].size() * 16 * _mapScale );
       _camera.x = 50.0f;
       _camera.y = (winH / 2.0f) - (mapPixelH / 2.0f);
       _paletteScale = 2;

       if (GlobalSettings::isEditorMode) {
           _showPalette = true; _showGrid = true;
       } else {
           _showPalette = false; _showGrid = false;
           if (!g_PendingGameMap.empty()) {
               LoadMapFromFile(g_PendingGameMap, *_currState);
               _currentGameMapPath = g_PendingGameMap;
               g_PendingGameMap = "";
           }
       }
       _pickedSize = { 1, 1 }; _pickedIdx = { 0, 0 }; _isSelectingPalette = false;
       _activeLayer = 0;

       _chestHitbox = { 0.0f, 0.0f, 0.0f, 0.0f };
       BuildChestHitboxFromMap(*_currState, 1, _tileCount, _chestHitbox);
       _levelFinished = false;
       _finishTimer = 0.0f;
       _manaRegenAccu = 0.0f;
    }

    void EditorState::ResetLevel()
    {
        if (GlobalSettings::isEditorMode) return;

        if (_beeChannel != -1) {
            Mix_HaltChannel(_beeChannel);
            _beeChannel = -1;
        }

        _player.Init(renderer());
        _bees.clear();
        _mushrooms.clear();

        auto SpawnBee = [&](float x, float y) {
            Bee b;
            b.Init(renderer(), x, y);
            _bees.push_back(std::move(b));
        };

        auto SpawnMushroom = [&](float x, float y) {
            Mushroom m;
            m.Init(renderer(), x, y);
            _mushrooms.push_back(std::move(m));
        };

        SpawnBee(1200.0f, 220.0f);
        SpawnMushroom(1400.0f, 240.0f);
        SpawnBee(3000.0f, 150.0f);
        SpawnBee(3080.0f, 250.0f);
        SpawnBee(3150.0f, 180.0f);
        SpawnMushroom(3300.0f, 240.0f);
        SpawnBee(6200.0f, 120.0f);
        SpawnBee(6280.0f, 280.0f);
        SpawnBee(6350.0f, 200.0f);
        SpawnBee(6420.0f, 150.0f);
        SpawnBee(6500.0f, 250.0f);
        SpawnMushroom(6600.0f, 240.0f);

        _fireballs.clear();
        _explosions.clear();
        _heals.clear();
        _magic.Cancel();

        if (!_currentGameMapPath.empty()) {
            LoadMapFromFile(_currentGameMapPath, *_currState);
        }

        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        float mapPixelH = (float)( (*_currState)[0].size() * 16 * _mapScale );
        _camera.x = 50.0f;
        _camera.y = (winH / 2.0f) - (mapPixelH / 2.0f);

        _levelFinished = false;
        _finishTimer = 0.0f;
        _manaRegenAccu = 0.0f;
    }

    void EditorState::Destroy() {
        if (_beeChannel != -1) {
            Mix_HaltChannel(_beeChannel);
            _beeChannel = -1;
        }
        for (auto& pair : _sfx) {
            Mix_FreeChunk(pair.second);
        }
        _sfx.clear();
    }

    bool EditorState::Input( const Event & evt ) {
       const char* defaultPath = "asset\\map\\";
       if (evt.type == SDL_EVENT_KEY_DOWN) {
           if (evt.key.scancode == SDL_SCANCODE_ESCAPE) { _game.ReplaceState( (u8)GameStateID::MainMenu ); return true; }
           if (GlobalSettings::isEditorMode) {
               if (evt.key.scancode == SDL_SCANCODE_TAB && evt.key.repeat == 0) _showPalette = !_showPalette;
               if (evt.key.scancode == SDL_SCANCODE_F8) SDL_ShowSaveFileDialog(OnMapSave, _currState, window(), nullptr, 0, defaultPath);
               if (evt.key.scancode == SDL_SCANCODE_F9) SDL_ShowOpenFileDialog(OnMapLoad, _currState, window(), nullptr, 0, defaultPath, false);

               if (evt.key.scancode == SDL_SCANCODE_1) _activeLayer = 0;
               if (evt.key.scancode == SDL_SCANCODE_2) _activeLayer = 1;
               if (evt.key.scancode == SDL_SCANCODE_3) _activeLayer = 2;

               if (evt.key.scancode == SDL_SCANCODE_F1) _mapScale = 1;
               if (evt.key.scancode == SDL_SCANCODE_F2) _mapScale = 2;
               if (evt.key.scancode == SDL_SCANCODE_F6 && evt.key.repeat == 0) _showGrid = !_showGrid;
               if (evt.key.scancode == SDL_SCANCODE_H && evt.key.repeat == 0) _flipH = !_flipH;
               if (evt.key.scancode == SDL_SCANCODE_V && evt.key.repeat == 0) _flipV = !_flipV;
               if (evt.key.scancode == SDL_SCANCODE_R && evt.key.repeat == 0) _rotSteps = (_rotSteps + 3) & 3;
           }
       }

        if (!GlobalSettings::isEditorMode && !_levelFinished) {
            if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.repeat == 0) {
                if (evt.key.scancode == SDL_SCANCODE_E) _magic.BeginCast(_player.mana);
                if (evt.key.scancode == SDL_SCANCODE_ESCAPE && _magic.IsActive()) _magic.Cancel();
                if (evt.key.scancode == SDL_SCANCODE_1) _debugTemplate = FD::Magic::MagicResult::Fireball;
                if (evt.key.scancode == SDL_SCANCODE_2) _debugTemplate = FD::Magic::MagicResult::Heal;

                // Jump Sound
                if (evt.key.scancode == SDL_SCANCODE_SPACE) {
                   if (_sfx.count("jump.wav")) PlaySFX("jump.wav", 0);
                   else PlaySFX("jump.mp3", 0);
                }
            }

            // Blade Swing auf linke Maustaste
            if (!_magic.IsActive() && evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
                int r = rand() % 3;
                if (r == 0) PlaySFX("Blade_swing_01.mp3", 0);
                else if (r == 1) PlaySFX("Blade_swing_02.mp3", 0);
                else PlaySFX("Blade_swing_03.mp3", 0);
            }

            if (_magic.IsActive()) {
                if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) _magic.OnMouseDown((float)evt.button.x, (float)evt.button.y);
                if (evt.type == SDL_EVENT_MOUSE_MOTION) { const bool pressed = (evt.motion.state & SDL_BUTTON_LMASK) != 0; _magic.OnMouseMove((float)evt.motion.x, (float)evt.motion.y, pressed); }
                if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_LEFT) _magic.OnMouseUp();
                return true;
            }
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
                    if(p.x < _tileCount.x && p.y < _tileCount.y) { _isSelectingPalette = true; _selectionStart = p; _pickedIdx = p; _pickedSize = { 1, 1 }; }
                }
            }
            if (!clickedInsidePalette) {
                _isPainting = true;
                Point p = toI(m - _camera) / (_tileSize * _mapScale);
                auto& curLayerMap = (*_currState)[_activeLayer];
                if(p.y >= 0 && (size_t)p.y < curLayerMap.size() && p.x >= 0 && (size_t)p.x < curLayerMap[0].size()) {
                     const int effW = (_rotSteps & 1) ? _pickedSize.y : _pickedSize.x;
                     const int effH = (_rotSteps & 1) ? _pickedSize.x : _pickedSize.y;
                     for(int py = 0; py < effH; ++py) {
                         for(int px = 0; px < effW; ++px) {
                             int targetX = p.x + px; int targetY = p.y + py;
                             if(targetY >= 0 && (size_t)targetY < curLayerMap.size() && targetX >= 0 && (size_t)targetX < curLayerMap[0].size()) {
                                 int srcRX = 0, srcRY = 0;
                                 MapSelectionToSource(px, py, _pickedSize.x, _pickedSize.y, _rotSteps, _flipH, _flipV, srcRX, srcRY);
                                 const int srcX = _pickedIdx.x + srcRX;
                                 const int srcY = _pickedIdx.y + srcRY;
                                 if (srcX < _tileCount.x && srcY < _tileCount.y) {
                                     int tileId = srcX + srcY * _tileCount.x;
                                     if (_flipH) tileId |= kFlipH;
                                     if (_flipV) tileId |= kFlipV;
                                     const int tileRot = (_rotSteps + ((_rotSteps & 1) ? 2 : 0)) & 3;
                                     tileId |= ((tileRot & 3) << kRotShift);
                                     curLayerMap[targetY][targetX] = tileId;
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
                   auto& curLayerMap = (*_currState)[_activeLayer];
                   const int effW = (_rotSteps & 1) ? _pickedSize.y : _pickedSize.x;
                   const int effH = (_rotSteps & 1) ? _pickedSize.x : _pickedSize.y;
                   for(int py = 0; py < effH; ++py) {
                         for(int px = 0; px < effW; ++px) {
                             int targetX = p.x + px; int targetY = p.y + py;
                             if(targetY >= 0 && (size_t)targetY < curLayerMap.size() && targetX >= 0 && (size_t)targetX < curLayerMap[0].size()) {
                             int srcRX = 0, srcRY = 0;
                             MapSelectionToSource(px, py, _pickedSize.x, _pickedSize.y, _rotSteps, _flipH, _flipV, srcRX, srcRY);
                             const int srcX = _pickedIdx.x + srcRX;
                             const int srcY = _pickedIdx.y + srcRY;
                             if (srcX < _tileCount.x && srcY < _tileCount.y) {
                                 int tileId = srcX + srcY * _tileCount.x;
                                 if (_flipH) tileId |= kFlipH;
                                 if (_flipV) tileId |= kFlipV;
                                 const int tileRot = (_rotSteps + ((_rotSteps & 1) ? 2 : 0)) & 3;
                                 tileId |= ((tileRot & 3) << kRotShift);
                                 curLayerMap[targetY][targetX] = tileId;
                             }
                             }
                         }
                   }
               }
           }
           if(_isPanning) _camera += FPoint{(f32)evt.motion.xrel, (f32)evt.motion.yrel};
       }
       return true;
    }

    void EditorState::Update( u64, Duration, f32 deltaT ) {
        if (!GlobalSettings::isEditorMode) {
            if (_levelFinished) {
                _finishTimer += deltaT;
                if (_finishTimer > 3.0f) {
                    _game.ReplaceState((u8)GameStateID::MainMenu);
                }
                return;
            }

            int winW, winH;
            SDL_GetWindowSize(window(), &winW, &winH);
            bool beeVisible = false;
            for (const auto& b : _bees) {
                if (b.state == BeeState::Dead) continue;
                float screenX = b.position.x * _mapScale + _camera.x;
                if (screenX > -50.0f && screenX < (float)winW + 50.0f) {
                    beeVisible = true;
                    break;
                }
            }

            if (beeVisible) {
                if (_beeChannel == -1 || Mix_Playing(_beeChannel) == 0) {
                    PlaySFX("bee.mp3", -1);
                }
            } else {
                if (_beeChannel != -1) {
                    Mix_HaltChannel(_beeChannel);
                    _beeChannel = -1;
                }
            }

            if (_magic.IsActive()) { _magic.Update(deltaT); }
            else {
                if (_player.mana < _player.maxMana) {
                    _manaRegenAccu += deltaT;
                    while (_manaRegenAccu >= 1.0f && _player.mana < _player.maxMana) {
                        _player.mana += 1;
                        _manaRegenAccu -= 1.0f;
                    }
                } else {
                    _manaRegenAccu = 0.0f;
                }

                int oldHp = _player.hp;

                _player.Update(deltaT, *_currState);

                if (_player.IsDeathAnimFinished()) {
                    ResetLevel();
                    return;
                }

                if (_player.hp > 0 && !_player.IsDeathAnimFinished()) {
                    for (auto& bee : _bees) {
                        bool wasAttacking = (bee.state == BeeState::Attack);
                        bee.Update(deltaT, _player);
                        if (!wasAttacking && bee.state == BeeState::Attack) PlaySFX("Bee_Sting.mp3", 0);
                    }
                    for (auto& mushroom : _mushrooms) {
                        bool wasAttacking = (mushroom.state == MushroomState::Attack);
                        mushroom.Update(deltaT, _player);
                        if (!wasAttacking && mushroom.state == MushroomState::Attack) PlaySFX("Mushroom_atk.mp3", 0);
                    }
                }

                if (_player.hp < oldHp) {
                    if (_player.hp <= 0 && oldHp > 0) {
                        if (_sfx.count("Player_Death.wav")) PlaySFX("Player_Death.wav", 0);
                        else PlaySFX("Player_Death.mp3", 0);
                    } else if (_player.hp > 0) {
                        if (_sfx.count("Player_Hit.wav")) PlaySFX("Player_Hit.wav", 0);
                        else PlaySFX("Player_Hit.mp3", 0);
                    }
                }

                if (_player.hp <= 0) return;

                for (auto& f : _fireballs) {
                    if (!f.alive) continue;
                    f.lifetime -= deltaT;
                    if (f.lifetime <= 0.0f) { f.alive = false; continue; }
                    f.animTime += deltaT;
                    f.pos.x += f.vel.x * deltaT;
                    f.pos.y += f.vel.y * deltaT;

                    const float dx = f.pos.x - f.startPos.x;
                    const float dy = f.pos.y - f.startPos.y;
                    if (std::sqrt((dx * dx) + (dy * dy)) >= 240.0f) { f.alive = false; continue; }

                    const FRect fbBox  = CircleToRect(f.pos, f.radius);
                    for (auto& bee : _bees) {
                        if (bee.state == BeeState::Dead) continue;
                        FRect beeBox = bee.GetHitbox();
                        if (SDL_HasRectIntersectionFloat(&fbBox, &beeBox)) {
                            bee.TakeDamage(20);
                            PlaySFX("Fireball_explosion.mp3", 0);
                            SpawnExplosion(_explosions, f.pos);
                            f.alive = false;
                            break;
                        }
                    }
                    if (f.alive) {
                        for (auto& mushroom : _mushrooms) {
                            if (mushroom.state == MushroomState::Dead) continue;
                            FRect mushBox = mushroom.GetHitbox();
                            if (SDL_HasRectIntersectionFloat(&fbBox, &mushBox)) {
                                mushroom.TakeDamage(20);
                                PlaySFX("Fireball_explosion.mp3", 0);
                                SpawnExplosion(_explosions, f.pos);
                                f.alive = false;
                                break;
                            }
                        }
                    }
                }
                _fireballs.erase(std::remove_if(_fireballs.begin(), _fireballs.end(), [](const auto& f) { return !f.alive; }), _fireballs.end());

                for (auto& e : _explosions) {
                    if (!e.alive) continue;
                    e.animTime += deltaT;
                    if (e.animTime >= 13 * 0.04f) e.alive = false;
                }
                _explosions.erase(std::remove_if(_explosions.begin(), _explosions.end(), [](const auto& e) { return !e.alive; }), _explosions.end());

                for (auto& h : _heals) {
                    if (!h.alive) continue;
                    h.animTime += deltaT;
                    if (h.animTime >= 9 * 0.05f) h.alive = false;
                }
                _heals.erase(std::remove_if(_heals.begin(), _heals.end(), [](const auto& h) { return !h.alive; }), _heals.end());

                if (_player.isAttacking && _player.currentFrame >= 2 && _player.currentFrame <= 4) {
                    FRect swordBox = _player.GetAttackHitbox();
                    for (auto& bee : _bees) {
                        if (bee.state == BeeState::Dead) continue;
                        FRect beeBox = bee.GetHitbox();
                        if (SDL_HasRectIntersectionFloat(&swordBox, &beeBox)) {
                            if (bee.z < 40) { bee.TakeDamage(10); PlaySFX("Mob_hit.mp3", 0); }
                        }
                    }
                    for (auto& mushroom : _mushrooms) {
                        if (mushroom.state == MushroomState::Dead) continue;
                        FRect mushBox = mushroom.GetHitbox();
                        if (SDL_HasRectIntersectionFloat(&swordBox, &mushBox)) {
                            mushroom.TakeDamage(10); PlaySFX("Mob_hit.mp3", 0);
                        }
                    }
                    if (SDL_HasRectIntersectionFloat(&swordBox, &_chestHitbox)) {
                        if (AreAllEnemiesDead(_bees, _mushrooms)) _levelFinished = true;
                    }
                }
                float targetCamX = -((_player.position.x * _mapScale) - (winW / 2.0f));
                float targetCamY = -((_player.position.y * _mapScale) - (winH / 2.0f));
                _camera.x += (targetCamX - _camera.x) * 5.0f * deltaT;
                _camera.y += (targetCamY - _camera.y) * 5.0f * deltaT;
            }
            auto res = _magic.ConsumeResult();
            if (res != FD::Magic::MagicResult::None) ApplyMagicResult(res, _player, _bees, _fireballs, _heals, this);
        }
    }

   void EditorState::Render( u64 frames, Duration, f32 deltaTNeeded ) {
       const auto& refLayer = (*_currState)[0];
       const bool healActive = std::any_of(_heals.begin(), _heals.end(), [](const auto& h) { return h.alive; });
       FPoint mapTS = toF( _tileSize * _mapScale );

       SDL_SetRenderDrawColor( renderer(), 30, 30, 30, 255 );
       FRect mapBG = toFRect( _camera, FPoint{(f32)refLayer[0].size(), (f32)refLayer.size()} * mapTS );
       SDL_RenderFillRect(renderer(), &mapBG);

       const auto& backgroundLayer = (*_currState)[0];
       if(GlobalSettings::isEditorMode && _activeLayer != 0) SDL_SetTextureAlphaMod(_tileSet.get(), 100);
       for( size_t y = 0; y < backgroundLayer.size(); ++y ) {
          for( size_t x = 0; x < backgroundLayer[y].size(); ++x ) {
             int idx = backgroundLayer[y][x];
             int baseId = idx & kTileIdMask;
             if (baseId == 0) continue;
             Point tIdx = { baseId % _tileCount.x, baseId / _tileCount.x };
             FRect srcR = toFRect( toF(tIdx * _tileSize), toF(_tileSize) );
             FRect dstR = toFRect( FPoint{(f32)x, (f32)y} * mapTS + _camera, mapTS );
             SDL_FlipMode flip = SDL_FLIP_NONE;
             if (idx & kFlipH) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
             if (idx & kFlipV) flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);
             const float angle = (float)(((idx & kRotMask) >> kRotShift) * 90);
             SDL_RenderTextureRotated( renderer(), _tileSet.get(), &srcR, &dstR, angle, nullptr, flip );
          }
       }
       if(GlobalSettings::isEditorMode) SDL_SetTextureAlphaMod(_tileSet.get(), 255);

       const auto& layer1 = (*_currState)[1];
       for( size_t y = 0; y < layer1.size(); ++y ) {
          if(GlobalSettings::isEditorMode && _activeLayer != 1) SDL_SetTextureAlphaMod(_tileSet.get(), 100);
          for( size_t x = 0; x < layer1[y].size(); ++x ) {
             int idx = layer1[y][x];
             int baseId = idx & kTileIdMask;
             if (baseId != 0) {
                 Point tIdx = { baseId % _tileCount.x, baseId / _tileCount.x };
                 FRect srcR = toFRect( toF(tIdx * _tileSize), toF(_tileSize) );
                 FRect dstR = toFRect( FPoint{(f32)x, (f32)y} * mapTS + _camera, mapTS );
                 SDL_FlipMode flip = SDL_FLIP_NONE;
                 if (idx & kFlipH) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
                 if (idx & kFlipV) flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);
                 const float angle = (float)(((idx & kRotMask) >> kRotShift) * 90);
                 SDL_RenderTextureRotated( renderer(), _tileSet.get(), &srcR, &dstR, angle, nullptr, flip );
             }
          }
          if(GlobalSettings::isEditorMode) SDL_SetTextureAlphaMod(_tileSet.get(), 255);
          if (!GlobalSettings::isEditorMode) {
              float playerFootY = _player.position.y + _player.size.y;
              int playerTileRow = (int)(playerFootY / 16.0f);
              if (playerTileRow == (int)y) _player.Render(renderer(), _camera, _mapScale, healActive);
              for (auto& bee : _bees) if ((int)((bee.position.y + 16.0f) / 16.0f) == (int)y) bee.Render(renderer(), _camera, _mapScale);
              for (auto& mushroom : _mushrooms) if ((int)(mushroom.position.y / 16.0f) == (int)y) mushroom.Render(renderer(), _camera, _mapScale);
          }
       }

       const auto& layer2 = (*_currState)[2];
       if(GlobalSettings::isEditorMode && _activeLayer != 2) SDL_SetTextureAlphaMod(_tileSet.get(), 100);
       for( size_t y = 0; y < layer2.size(); ++y ) {
          for( size_t x = 0; x < layer2[y].size(); ++x ) {
             int idx = layer2[y][x];
             int baseId = idx & kTileIdMask;
             if (baseId != 0) {
                 Point tIdx = { baseId % _tileCount.x, baseId / _tileCount.x };
                 FRect srcR = toFRect( toF(tIdx * _tileSize), toF(_tileSize) );
                 FRect dstR = toFRect( FPoint{(f32)x, (f32)y} * mapTS + _camera, mapTS );
                 SDL_FlipMode flip = SDL_FLIP_NONE;
                 if (idx & kFlipH) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
                 if (idx & kFlipV) flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);
                 const float angle = (float)(((idx & kRotMask) >> kRotShift) * 90);
                 SDL_RenderTextureRotated( renderer(), _tileSet.get(), &srcR, &dstR, angle, nullptr, flip );
             }
          }
       }
       if(GlobalSettings::isEditorMode) SDL_SetTextureAlphaMod(_tileSet.get(), 255);

       if (!GlobalSettings::isEditorMode) {
           if (_enemyHpFrame && _enemyHpFill) {
               float frameW = 0.0f, frameH = 0.0f; float fillW = 0.0f, fillH = 0.0f;
               SDL_GetTextureSize(_enemyHpFrame.get(), &frameW, &frameH);
               SDL_GetTextureSize(_enemyHpFill.get(), &fillW, &fillH);
               const float uiScale = 0.6f;
               for (const auto& bee : _bees) {
                   if (bee.state == BeeState::Dead) continue;
                   const float hpRatio = std::clamp((float)bee.hp / (float)bee.maxHp, 0.0f, 1.0f);
                   const float centerX = (bee.position.x + (bee.size.x * 0.5f)) * (float)_mapScale + _camera.x;
                   const float topY = (bee.position.y * (float)_mapScale) + _camera.y - (bee.z * (float)_mapScale) - 18.0f;
                   FRect frameDst = { centerX - (frameW * uiScale * 0.5f), topY - (frameH * uiScale), frameW * uiScale, frameH * uiScale };
                   SDL_FRect fillSrc = { 0.0f, 0.0f, fillW * hpRatio, fillH };
                   SDL_FRect fillDst = { frameDst.x + (frameDst.w - fillW * uiScale) * 0.5f, frameDst.y + (frameDst.h - fillH * uiScale) * 0.5f, fillW * uiScale * hpRatio, fillH * uiScale };
                   SDL_RenderTexture(renderer(), _enemyHpFrame.get(), nullptr, &frameDst);
                   SDL_RenderTexture(renderer(), _enemyHpFill.get(), &fillSrc, &fillDst);
               }
               for (const auto& mushroom : _mushrooms) {
                   if (mushroom.state == MushroomState::Dead) continue;
                   const float hpRatio = std::clamp((float)mushroom.hp / (float)mushroom.maxHp, 0.0f, 1.0f);
                   const float centerX = (mushroom.position.x * (float)_mapScale) + _camera.x;
                   const float topY = ((mushroom.position.y - mushroom.frameH) * (float)_mapScale) + _camera.y - (mushroom.z * (float)_mapScale) - 2.0f;
                   FRect frameDst = { centerX - (frameW * uiScale * 0.5f), topY - (frameH * uiScale), frameW * uiScale, frameH * uiScale };
                   SDL_FRect fillSrc = { 0.0f, 0.0f, fillW * hpRatio, fillH };
                   SDL_FRect fillDst = { frameDst.x + (frameDst.w - fillW * uiScale) * 0.5f, frameDst.y + (frameDst.h - fillH * uiScale) * 0.5f, fillW * uiScale * hpRatio, fillH * uiScale };
                   SDL_RenderTexture(renderer(), _enemyHpFrame.get(), nullptr, &frameDst);
                   SDL_RenderTexture(renderer(), _enemyHpFill.get(), &fillSrc, &fillDst);
               }
           }

           for (const auto& f : _fireballs) {
               if (!f.alive) continue;
               const float dist = std::sqrt(std::pow(f.pos.x - f.startPos.x, 2) + std::pow(f.pos.y - f.startPos.y, 2));
               const int frame = std::min((int)((dist / 200.0f) * 15.0f), 14);
               JanSordid::SDL::FRect src = { (float)(frame * 16), 0.0f, 16.0f, 16.0f };
               JanSordid::SDL::FRect dst = { (f.pos.x * (float)_mapScale) + _camera.x - (16.0f * (float)_mapScale * 0.5f), (f.pos.y * (float)_mapScale) + _camera.y - (16.0f * (float)_mapScale * 0.5f), 16.0f * (float)_mapScale, 16.0f * (float)_mapScale };
               SDL_RenderTextureRotated(renderer(), _texFireball.get(), &src, &dst, std::atan2(f.vel.y, f.vel.x) * (180.0 / M_PI), nullptr, SDL_FLIP_NONE);
           }
           for (const auto& e : _explosions) {
               if (!e.alive) continue;
               const int frame = std::min((int)(e.animTime / 0.04f), 12);
               JanSordid::SDL::FRect src = { (float)((frame % 4) * 96), (float)((frame / 4) * 48), 96.0f, 48.0f };
               JanSordid::SDL::FRect dst = { (e.pos.x * (float)_mapScale) + _camera.x - (96.0f * (float)_mapScale * 0.5f), (e.pos.y * (float)_mapScale) + _camera.y - (48.0f * (float)_mapScale * 0.5f), 96.0f * (float)_mapScale, 48.0f * (float)_mapScale };
               SDL_RenderTexture(renderer(), _texExplosion.get(), &src, &dst);
           }
           for (const auto& h : _heals) {
               if (!h.alive) continue;
               const int frame = std::min((int)(h.animTime / 0.05f), 8);
               JanSordid::SDL::FRect src = { (float)((frame % 3) * 48), (float)((frame / 3) * 64), 48.0f, 64.0f };
               JanSordid::SDL::FRect dst = { (h.pos.x * (float)_mapScale) + _camera.x - (48.0f * (float)_mapScale * 0.5f), (h.pos.y * (float)_mapScale) + _camera.y - (64.0f * (float)_mapScale * 0.5f), 48.0f * (float)_mapScale, 64.0f * (float)_mapScale };
               SDL_RenderTexture(renderer(), _texHeal.get(), &src, &dst);
           }

           int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
           FD::Magic::MagicDebugRender::RenderOverlay(renderer(), _magic, winW, winH, _debugTemplate);
           if (_uiFrame && _uiHpFill && _uiManaFill) {
               float frameW, frameH; SDL_GetTextureSize(_uiFrame.get(), &frameW, &frameH);
               float uiScale = 2.0f; FRect frameDst = { 16.0f, (float)winH - 16.0f - frameH * uiScale, frameW * uiScale, frameH * uiScale };
               float hpW, hpH; SDL_GetTextureSize(_uiHpFill.get(), &hpW, &hpH);
               float manaW, manaH; SDL_GetTextureSize(_uiManaFill.get(), &manaW, &manaH);
               SDL_FRect hpSrc = { 0.0f, 0.0f, hpW * std::clamp((float)_player.hp / (float)_player.maxHp, 0.0f, 1.0f), hpH };
               SDL_FRect hpDst = { frameDst.x + (30.0f * uiScale) + 105.0f, frameDst.y + (30.0f * uiScale), (hpW * uiScale) * hpSrc.w / hpW, hpH * uiScale };
               SDL_FRect manaSrc = { 0.0f, 0.0f, manaW * std::clamp((float)_player.mana / (float)_player.maxMana, 0.0f, 1.0f), manaH };
               SDL_FRect manaDst = { frameDst.x + (30.0f * uiScale) + 110.0f, frameDst.y + (50.0f * uiScale), (manaW * uiScale) * manaSrc.w / manaW, manaH * uiScale };
               SDL_RenderTexture(renderer(), _uiFrame.get(), nullptr, &frameDst);
               SDL_RenderTexture(renderer(), _uiHpFill.get(), &hpSrc, &hpDst);
               SDL_RenderTexture(renderer(), _uiManaFill.get(), &manaSrc, &manaDst);
           }
           if (_levelFinished && _font) {
               Owned<Surface> s(TTF_RenderText_Blended(_font.get(), "LEVEL GESCHAFFT!", 0, {255, 215, 0, 255}));
               if(s) {
                   Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
                   FRect r = { (winW/2.0f) - (s->w/2.0f), (winH/2.0f) - (s->h/2.0f), (f32)s->w, (f32)s->h };
                   SDL_RenderTexture(renderer(), t.get(), EntireFRect, &r);
               }
           }
       }

       if(GlobalSettings::isEditorMode) {
           SDL_SetRenderDrawColor( renderer(), 255, 0, 0, 255 );
           SDL_RenderRect( renderer(), &mapBG );
           SDL_SetRenderDrawColor(renderer(), 0, 255, 255, 255);
           FRect debugChest = { (_chestHitbox.x * (float)_mapScale) + _camera.x, (_chestHitbox.y * (float)_mapScale) + _camera.y, _chestHitbox.w * (float)_mapScale, _chestHitbox.h * (float)_mapScale };
           SDL_RenderRect(renderer(), &debugChest);
           if(_showGrid) {
               SDL_SetRenderDrawColor( renderer(), 255, 255, 255, 50 ); SDL_SetRenderDrawBlendMode(renderer(), SDL_BLENDMODE_BLEND);
               for(size_t y=0; y<refLayer.size(); ++y) for(size_t x=0; x<refLayer[y].size(); ++x) {
                    FRect gridR = toFRect(FPoint{(f32)x, (f32)y}*mapTS+_camera, mapTS);
                    SDL_RenderRect(renderer(), &gridR);
               }
           }
           float mx, my; SDL_GetMouseState(&mx, &my); FPoint m = { mx, my };
           if (_showPalette && !(m.x < toF(_tileSetSize*_paletteScale).x && m.y < toF(_tileSetSize*_paletteScale).y)) {
               Point p = toI(m - _camera) / (_tileSize * _mapScale);
               if(p.y >= 0 && (size_t)p.y < layer1.size() && p.x >= 0 && (size_t)p.x < layer1[0].size()) {
                   SDL_SetTextureAlphaMod(_tileSet.get(), 150);
                   const int effW = (_rotSteps & 1) ? _pickedSize.y : _pickedSize.x;
                   const int effH = (_rotSteps & 1) ? _pickedSize.x : _pickedSize.y;
                   for(int py = 0; py < effH; ++py) {
                       for(int px = 0; px < effW; ++px) {
                           int srcRX = 0, srcRY = 0;
                           MapSelectionToSource(px, py, _pickedSize.x, _pickedSize.y, _rotSteps, _flipH, _flipV, srcRX, srcRY);
                           const int srcX = _pickedIdx.x + srcRX;
                           const int srcY = _pickedIdx.y + srcRY;
                           if (srcX < _tileCount.x && srcY < _tileCount.y) {
                               FRect srcR = toFRect( toF(Point{srcX, srcY} * _tileSize), toF(_tileSize) );
                               FRect dstR = toFRect( FPoint{(f32)(p.x + px), (f32)(p.y + py)} * mapTS + _camera, mapTS );
                               SDL_FlipMode flip = SDL_FLIP_NONE;
                               if (_flipH) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
                               if (_flipV) flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);
                               const int tileRot = (_rotSteps + ((_rotSteps & 1) ? 2 : 0)) & 3;
                               const float angle = (float)(tileRot * 90);
                               SDL_RenderTextureRotated( renderer(), _tileSet.get(), &srcR, &dstR, angle, nullptr, flip );
                           }
                       }
                   }
                   SDL_SetTextureAlphaMod(_tileSet.get(), 255);
               }
           }

           if(_showPalette) {
               FRect r = toFRect(FPoint{0,0}, toF(_tileSize*_paletteScale*_tileCount));
               SDL_SetRenderDrawColor(renderer(), 10, 10, 20, 240); SDL_SetRenderDrawBlendMode(renderer(), SDL_BLENDMODE_BLEND); SDL_RenderFillRect(renderer(), &r);
               SDL_RenderTexture(renderer(), _tileSet.get(), EntireFRect, &r);
               SDL_SetRenderDrawColor(renderer(), 255, 255, 0, 255);
               FPoint selectSize = toF(_tileSize * _paletteScale * _pickedSize);
               FRect pickR = toFRect(toF(_tileSize * _paletteScale * _pickedIdx), selectSize);
               SDL_RenderRect(renderer(), &pickR);
           }

           if(_font) {
               std::ostringstream oss;
               std::string layerName = (_activeLayer == 0) ? "1: HINTERGRUND" : (_activeLayer == 1) ? "2: SPIELEBENE" : "3: VORDERGRUND";
               oss << "Editor Mode - AKTIVER LAYER: " << layerName
                   << "\n[1,2,3] Layer wechseln"
                   << "\n[ESC] Main Menu"
                   << "\n[TAB] Palette"
                   << "\n[F1,F2] Zoom"
                   << "\n[F6] Grid"
                   << "\n[H] Flip H  [V] Flip V  [R] Rotieren"
                   << "\n[F8] Save [F9] Load";

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
    }

    void MainMenuState::Init() {
        std::string fontP = GetAssetPath(BasePathFont "RobotoSlab-Bold.ttf");
        if (!_fontTitle) _fontTitle.reset(TTF_OpenFont(fontP.c_str(), 60));
        if (!_fontMenu)  _fontMenu.reset(TTF_OpenFont(fontP.c_str(), 30));
        std::string bgP = GetAssetPath(BasePathGraphic "menu_background.png");
        if (!_background) { auto* surf = IMG_Load(bgP.c_str()); if (surf) { _background.reset(SDL_CreateTextureFromSurface(renderer(), surf)); SDL_DestroySurface(surf); } }
        std::string musicP = GetAssetPath(BasePathAudio "hauptmenu_sound.mp3");
        std::ifstream f(musicP);
        if (f.good()) {
            _bgMusic = Mix_LoadMUS(musicP.c_str());
            if (_bgMusic) { Mix_VolumeMusic(GlobalSettings::musicVolume); Mix_PlayMusic(_bgMusic, -1); SDL_Log("Musik gestartet: %s", musicP.c_str()); }
            else { SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Musik konnte nicht geladen werden (falsches Format?): %s", SDL_GetError()); }
        } else { SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Musikdatei nicht gefunden: %s", musicP.c_str()); }
    }
    void MainMenuState::Destroy() { if (_bgMusic) { Mix_HaltMusic(); Mix_FreeMusic(_bgMusic); _bgMusic = nullptr; } }
    bool MainMenuState::DrawButton(const char* text, float y, float mouseX, float mouseY, bool isClicked) {
        Color c = { 200, 200, 200, 255 }; bool hovered = false; int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        Owned<Surface> s(TTF_RenderText_Blended(_fontMenu.get(), text, 0, c));
        if (s) {
            float w = (float)s->w; float h = (float)s->h; float x = (winW / 2.0f) - (w / 2.0f);
            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) { s.reset(TTF_RenderText_Blended(_fontMenu.get(), text, 0, {255, 255, 0, 255})); hovered = true; }
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
            FRect btnR = toFRect(FPoint{x, y}, FPoint{w, h});
            SDL_RenderTexture(renderer(), t.get(), EntireFRect, &btnR);
        }
        return hovered && isClicked;
    }
    bool MainMenuState::Input(const Event& event) {
        const char* defaultPath = "asset\\map\\";
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            float mx = (float)event.button.x; float my = (float)event.button.y;
            int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
            float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (4 * spacing) / 2.0f + 50.0f;
            if (DrawButton("Spiel starten", startY, mx, my, true)) { SDL_ShowOpenFileDialog(OnSelectMapForGame, &_game, window(), nullptr, 0, defaultPath, false); }
            else if (DrawButton("Map Creator", startY + spacing, mx, my, true)) { GlobalSettings::isEditorMode = true; _game.ReplaceState((u8)GameStateID::Editor); }
            else if (DrawButton("Settings", startY + spacing*2, mx, my, true)) _game.PushState((u8)GameStateID::Settings);
            else if (DrawButton("Beenden", startY + spacing*3, mx, my, true)) { SDL_Event quit; quit.type = SDL_EVENT_QUIT; SDL_PushEvent(&quit); }
        }
        return true;
    }
    void MainMenuState::Render(u64, Duration, f32) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        if (_background) { SDL_RenderTexture(renderer(), _background.get(), nullptr, nullptr); } else { SDL_SetRenderDrawColor(renderer(), 30, 30, 40, 255); SDL_RenderClear(renderer()); }
        Owned<Surface> s(TTF_RenderText_Blended(_fontTitle.get(), "FANTASY DRAGON", 0, {255, 255, 255, 255}));
        if(s) {
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get()));
            float tx = (winW / 2.0f) - (s->w / 2.0f); FRect titleR = toFRect(FPoint{tx, winH * 0.15f}, FPoint{(f32)s->w, (f32)s->h});
            SDL_RenderTexture(renderer(), t.get(), EntireFRect, &titleR);
        }
        float mx, my; SDL_GetMouseState(&mx, &my);
        float centerY = winH / 2.0f; float spacing = 60.0f; float startY  = centerY - (4 * spacing) / 2.0f + 50.0f;
        DrawButton("Spiel starten", startY, mx, my, false);
        DrawButton("Map Creator", startY + spacing, mx, my, false);
        DrawButton("Settings",      startY + spacing*2, mx, my, false);
        DrawButton("Beenden",       startY + spacing*3, mx, my, false);
    }

    void SettingsState::Init() {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        std::string fontP = GetAssetPath(BasePathFont "RobotoSlab-Bold.ttf"); if (!_font) _font.reset(TTF_OpenFont(fontP.c_str(), 30));
        std::string bgP = GetAssetPath(BasePathGraphic "menu_settings.png"); if (!_background) { auto* surf = IMG_Load(bgP.c_str()); if (surf) { _background.reset(SDL_CreateTextureFromSurface(renderer(), surf)); SDL_DestroySurface(surf); } }
    }
    bool SettingsState::DrawSlider(const char* label, float y, float mouseX, float mouseY, bool isMouseDown, int& volumeRef) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        float sliderWidth = 300.0f; float sliderHeight = 10.0f; float sliderX = (winW / 2.0f) - (sliderWidth / 2.0f); float sliderY = y + 40.0f;
        Owned<Surface> s(TTF_RenderText_Blended(_font.get(), label, 0, {200, 200, 200, 255}));
        if(s) { float tw = (float)s->w; float th = (float)s->h; FRect tRect = { (winW / 2.0f) - (tw/2.0f), y, tw, th }; Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get())); SDL_RenderTexture(renderer(), t.get(), nullptr, &tRect); }
        FRect track = { sliderX, sliderY, sliderWidth, sliderHeight }; SDL_SetRenderDrawColor(renderer(), 100, 100, 100, 255); SDL_RenderFillRect(renderer(), &track);
        float pct = (float)volumeRef / 128.0f; float knobX = sliderX + (pct * sliderWidth); FRect knob = { knobX - 10.0f, sliderY - 5.0f, 20.0f, 20.0f }; SDL_SetRenderDrawColor(renderer(), 255, 200, 0, 255); SDL_RenderFillRect(renderer(), &knob);
        if (isMouseDown) {
            if (mouseX >= sliderX && mouseX <= sliderX + sliderWidth && mouseY >= sliderY - 20.0f && mouseY <= sliderY + 30.0f) {
                float newPct = (mouseX - sliderX) / sliderWidth; if (newPct < 0) newPct = 0; if (newPct > 1) newPct = 1;
                volumeRef = (int)(newPct * 128.0f);
                return true;
            }
        }
        return false;
    }
    bool SettingsState::DrawButton(const char* text, float y, float mouseX, float mouseY, bool isClicked) {
        Color c = { 200, 200, 200, 255 }; bool hovered = false; int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        Owned<Surface> s(TTF_RenderText_Blended(_font.get(), text, 0, c));
        if(s) {
            float w = (float)s->w; float h = (float)s->h; float x = (winW / 2.0f) - (w / 2.0f);
            if (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h) { s.reset(TTF_RenderText_Blended(_font.get(), text, 0, {255, 255, 0, 255})); hovered = true; }
            Owned<Texture> t(SDL_CreateTextureFromSurface(renderer(), s.get())); FRect btnR = toFRect(FPoint{x, y}, FPoint{w, h}); SDL_RenderTexture(renderer(), t.get(), EntireFRect, &btnR);
        }
        return hovered && isClicked;
    }
    bool SettingsState::Input(const Event& event) {
        float mx = 0, my = 0; SDL_GetMouseState(&mx, &my); bool isDown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH); float centerY = winH / 2.0f; float spacing = 80.0f; float startY = centerY - (4 * spacing) / 2.0f;
        if (isDown) {
            std::string musTxt = "Musik: " + std::to_string((int)((GlobalSettings::musicVolume / 128.0f) * 100)) + "%";
            if (DrawSlider(musTxt.c_str(), startY, mx, my, true, GlobalSettings::musicVolume)) {
                Mix_VolumeMusic(GlobalSettings::musicVolume);
            }
            std::string sfxTxt = "Sounds: " + std::to_string((int)((GlobalSettings::sfxVolume / 128.0f) * 100)) + "%";
            DrawSlider(sfxTxt.c_str(), startY + spacing, mx, my, true, GlobalSettings::sfxVolume);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            float clickX = (float)event.button.x; float clickY = (float)event.button.y;
            std::string screenText = std::string("Modus: ") + (GlobalSettings::isFullscreen ? "Vollbild" : "Fenster");
            if (DrawButton(screenText.c_str(), startY + spacing*2, clickX, clickY, true)) { GlobalSettings::isFullscreen = !GlobalSettings::isFullscreen; SDL_SetWindowFullscreen(window(), GlobalSettings::isFullscreen ? SDL_WINDOW_FULLSCREEN : 0); }
            if (DrawButton("Zurueck", startY + spacing*3, clickX, clickY, true)) _game.PopState();
        }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) _game.PopState();
        return true;
    }
    void SettingsState::Render(u64, Duration, f32) {
        int winW, winH; SDL_GetWindowSize(window(), &winW, &winH);
        if (_background) { SDL_RenderTexture(renderer(), _background.get(), nullptr, nullptr); } else { SDL_SetRenderDrawColor(renderer(), 40, 30, 30, 255); SDL_RenderClear(renderer()); }
        float mx, my; SDL_GetMouseState(&mx, &my); float centerY = winH / 2.0f; float spacing = 80.0f; float startY = centerY - (4 * spacing) / 2.0f;
        std::string musTxt = "Musik: " + std::to_string((int)((GlobalSettings::musicVolume / 128.0f) * 100)) + "%"; DrawSlider(musTxt.c_str(), startY, mx, my, false, GlobalSettings::musicVolume);
        std::string sfxTxt = "Sounds: " + std::to_string((int)((GlobalSettings::sfxVolume / 128.0f) * 100)) + "%"; DrawSlider(sfxTxt.c_str(), startY + spacing, mx, my, false, GlobalSettings::sfxVolume);
        std::string screenText = std::string("Modus: ") + (GlobalSettings::isFullscreen ? "Vollbild" : "Fenster"); DrawButton(screenText.c_str(), startY + spacing*2, mx, my, false);
        DrawButton("Zurück", startY + spacing*3, mx, my, false);
    }
}
