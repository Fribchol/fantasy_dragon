#include "fantasydragon_mapeditor.hpp"
#include <sstream>

// Versuch HSNR64 Offset einzubinden
#if __has_include("hsnr64/offset.hpp")

    #include "hsnr64/offset.hpp"
#endif

// =========================================================
// Namespaces öffnen (Wichtig für Owned, u64, etc.)
// =========================================================
using namespace JanSordid;
using namespace JanSordid::Core;
using namespace JanSordid::SDL;

// Hilfsfunktionen sichtbar machen
using JanSordid::SDL::toI;
using JanSordid::SDL::toF;
using JanSordid::SDL::toWH;
using JanSordid::SDL::toFRect;
using JanSordid::SDL::EntireFRect;

namespace JanSordid::SDL_Example
{
    void EditorState::Init()
    {
       // Schriftart laden
       if( !_font )
          _font.reset( TTF_OpenFont( BasePathFont "RobotoSlab-Bold.ttf", (int)(9 * _game.scalingFactor()) ) );

       if( !_tileSet )
       {
          // KORREKTUR: Hier laden wir dein NEUES Bild!
          // Stelle sicher, dass die Datei "tiles_fantasydragon.png" im Ordner assets/graphics/ liegt.
          Owned<Surface> surf( IMG_Load( BasePathGraphic "tiles_fantasydragon.png" ) );

          if(!surf) {
              // Falls Datei nicht gefunden, erstelle Lila-Platzhalter (damit man sieht, dass was fehlt)
              print("FEHLER: tiles_fantasydragon.png konnte nicht geladen werden!\n");
              surf.reset( SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA8888) );
          }

          _tileSet.reset( SDL_CreateTextureFromSurface( renderer(), surf.get() ) );

          _tileSetSize = { surf->w, surf->h };

          // ANALYSE DEINES BILDES:

          _tileCount   = { 32, 32 };

          // Schutz vor Division durch Null
          if (_tileCount.x == 0) _tileCount.x = 1;
          if (_tileCount.y == 0) _tileCount.y = 1;

          _tileSize    = _tileSetSize / _tileCount;

          SDL_SetTextureScaleMode( _tileSet.get(), SDL_ScaleMode::SDL_SCALEMODE_NEAREST );
       }

       if( _doGenerateEmptyMap )
       {
          // Indices für dein neues Tileset berechnen:
          // Index = y * Breite + x
          // Dirt (Mitte vom Gras-Block): Zeile 1, Spalte 1 -> 1 * 16 + 1 = 17
          // Wand (Mitte vom Stein-Block): Zeile 4, Spalte 1 -> 4 * 16 + 1 = 65
          const int
             Dirt     = 17,
             HardWall = 65;

          for( auto & row : *_currState )
          {
             row.fill( Dirt );
             *row.begin() = HardWall;
             *row.rbegin() = HardWall;
          }
          (*_currState->begin()).fill( HardWall );
          (*_currState->rbegin()).fill( HardWall );
       }

       _camera       = FPoint{ 0, 0 }; // Start bei 0,0 damit wir oben links sind
       _paletteScale = (i32)_game.scalingFactor();
    }

    void EditorState::Destroy()
    {
    }

    bool EditorState::Input( const Event & evt )
    {
       // Palette oben links definieren (Position & Größe)
       // Wir definieren das hier lokal, damit Input & Render synchron bleiben
       const FPoint palettePos = { 0.0f, 0.0f }; // Palette bei 0,0

       switch( evt.type )
       {
          case SDL_EVENT_KEY_DOWN:
          {
             const SDL_KeyboardEvent & key = evt.key;
             if( key.scancode == SDL_SCANCODE_F1 && key.repeat == 0 ) { _mapScale = 1; }
             else if( key.scancode == SDL_SCANCODE_F2 && key.repeat == 0 ) { _mapScale = 2; }
             else if( key.scancode == SDL_SCANCODE_F3 && key.repeat == 0 ) { _mapScale = 3; }
             else if( key.scancode == SDL_SCANCODE_F4 && key.repeat == 0 ) { _mapScale = 4; }
             else if( key.scancode == SDL_SCANCODE_F5 && key.repeat == 0 )
             {
                _paletteScale = _paletteScale == 1 ? 2 : 1;
             }
             else if( key.scancode == SDL_SCANCODE_F6 && key.repeat == 0 )
             {
                _showGrid = !_showGrid;
             }
             else if( key.scancode == SDL_SCANCODE_F8 && key.repeat == 0 )
             {
                const WorldState & curr = *_currState;
                print( "\n{{{{ \n" );
                for( const auto& row : curr )
                {
                   print( "\n\t{{ " );
                   for( const auto& cell : row )
                   {
                      print( "{},", cell );
                   }
                   print( " }}," );
                }
                print( "\n}}}};\n" );
             }
             break;
          }

          case SDL_EVENT_MOUSE_BUTTON_DOWN:
          {
             const SDL_MouseButtonEvent & me = evt.button;
             if( me.button == SDL_BUTTON_LEFT )
             {
                const FPoint mousePos = { me.x, me.y };
                const FPoint paletteSize = toF(_tileSetSize * _paletteScale);

                // Prüfen, ob Klick in der Palette war (Oben links)
                if( mousePos.x >= palettePos.x && mousePos.x < palettePos.x + paletteSize.x &&
                    mousePos.y >= palettePos.y && mousePos.y < palettePos.y + paletteSize.y )
                {
                   // Klick in Palette: Tile auswählen
                   const Point p = toI( mousePos - palettePos ) / (_tileSize * _paletteScale);
                   _pickedIdx = p;
                }
                else
                {
                   // Klick in Map: Malen
                   const Point p = toI( mousePos - _camera ) / (_tileSize * _mapScale);
                   if( p.y >= 0 && (size_t)p.y < _currState->size()
                    && p.x >= 0 && (size_t)p.x < (*_currState)[0].size() )
                   {
                      (*_currState)[p.y][p.x] = _pickedIdx.x + _pickedIdx.y * _tileCount.x;
                   }
                   _isPainting = true;
                }
             }
             else if( me.button == SDL_BUTTON_RIGHT )
             {
                _isPanning = true;
             }
             break;
          }

          case SDL_EVENT_MOUSE_BUTTON_UP:
          {
             const SDL_MouseButtonEvent & me = evt.button;
             if( me.button == SDL_BUTTON_LEFT )
             {
                _isPainting = false;
             }
             else if( me.button == SDL_BUTTON_RIGHT )
             {
                _isPanning = false;
             }
             break;
          }

          case SDL_EVENT_MOUSE_MOTION:
          {
             const SDL_MouseMotionEvent & me = evt.motion;
             if( _isPainting )
             {
                const FPoint mousePos = { me.x, me.y };
                const Point  p = toI( mousePos - _camera ) / _tileSize / _mapScale;
                if( p.y >= 0 && (size_t)p.y < _currState->size()
                 && p.x >= 0 && (size_t)p.x < (*_currState)[0].size() )
                {
                   (*_currState)[p.y][p.x] = _pickedIdx.x + _pickedIdx.y * _tileCount.x;
                }
             }
             else if( _isPanning )
             {
                _camera += FPoint{ me.xrel, me.yrel };
             }
             break;
          }
       }

       return true;
    }

    void EditorState::Update( const u64 framesSinceStart, const Duration timeSinceStart, const f32 deltaT )
    {
       if( timeSinceStart < _nextUpdateTime )
          return;

       const WorldState & curr = *_currState;
       WorldState & next = *_nextState;

       next = curr;

       _nextUpdateTime += UpdateDeltaTime;
       std::swap( _currState, _nextState );
    }

    void EditorState::Render( const u64 framesSinceStart, const Duration timeSinceStart, const f32 deltaTNeeded )
    {
       const WorldState & curr = *_currState;

       const FPoint mapTileSize = toF( _tileSize * _mapScale );
       const FPoint levelSize   = { (f32)curr[0].size(), (f32)curr.size() };


       /// Draw Map
       for( size_t y = 0; y < curr.size(); ++y )
       {
          for( size_t x = 0; x < curr[y].size(); ++x )
          {
             const int    index     = curr[y][x];
             const Point  tileIndex = { index % _tileCount.x, index / _tileCount.x };
             const FPoint pos       = FPoint{ (f32)x, (f32)y } * mapTileSize + _camera;
             const FRect  srcRect   = toFRect( toF( tileIndex * _tileSize ), toF( _tileSize ) );
             const FRect  dstRect   = toFRect( pos, mapTileSize );
             SDL_RenderTexture( renderer(), _tileSet.get(), &srcRect, &dstRect );
          }
       }

       // Map Border (Red)
       SDL_SetRenderDrawColor( renderer(), 255, 0, 0, SDL_ALPHA_OPAQUE );
       const FRect borderRect = toFRect( _camera, levelSize * mapTileSize );
       SDL_RenderRect( renderer(), &borderRect );


       /// Draw Grid
       if( _showGrid )
       {
          SDL_SetRenderDrawColor( renderer(), 0, 0, 0, 63 );
          SDL_SetRenderDrawBlendMode( renderer(), SDL_BLENDMODE_BLEND );

          for( size_t y = 0; y < curr.size(); ++y )
          {
             for( size_t x = 0; x < curr[y].size(); ++x )
             {
                const FPoint gridPos  = FPoint{ (f32)x, (f32)y } * mapTileSize + _camera;
                const FRect  gridRect = toFRect( gridPos, mapTileSize );
                SDL_RenderRect( renderer(), &gridRect );
             }
          }
       }


       /// Draw Palette Overlay
       {
          const Point paletteTileSize = _tileSize * _paletteScale;
          const FRect paletteRect     = toFRect( FPoint{0,0}, toF( paletteTileSize * _tileCount ) );

          // Background for Palette
          SDL_SetRenderDrawColor( renderer(), 20, 20, 20, 230 ); // Dark background
          SDL_RenderFillRect( renderer(), &paletteRect );

          // Draw the full tileset
          SDL_RenderTexture( renderer(), _tileSet.get(), EntireFRect, &paletteRect );

          // Draw selection cursor (blinking)
          const Point pickerPosition  = paletteTileSize * _pickedIdx;
          const FRect pickerRect      = toFRect( toF( pickerPosition ), toF( paletteTileSize ) );

          const size_t blink = (framesSinceStart & 0x3f) >> 3;
          if(blink < BaseColors.size()) {
             const Color & c = BaseColors[blink];
             SDL_SetRenderDrawColor( renderer(), c.r, c.g, c.b, c.a );
             SDL_RenderRect( renderer(), &pickerRect );
          }
       }

       /// Draw Help Text
       {
          std::ostringstream oss;
          oss << "--== Map Editor ==--\n"
              << "Zoom: " << _mapScale << "x";

          constexpr Color c = { 255, 255, 255, 255 };

          Owned<Surface> surf( TTF_RenderText_Blended_Wrapped( _font.get(), oss.str().c_str(), 0, c, (int)(400 * _game.scalingFactor()) ) );

          if(surf) {
              Owned<Texture> t2( SDL_CreateTextureFromSurface( renderer(), surf.get() ) );

              SDL_SetTextureColorMod( t2.get(), 255, 255, 255 );
              // Position text to the right of the palette
              const FPoint p = { (f32)_tileSetSize.x * _paletteScale + 20, 20 };
              const FRect dstRect = toFRect( p, FPoint{ (f32)surf->w, (f32)surf->h } );
              SDL_RenderTexture( renderer(), t2.get(), EntireFRect, &dstRect );
          }
       }
    }
}