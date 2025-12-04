#include "sor/sdl_game.hpp"
#include "fantasydragon_mapeditor.hpp"

// Wir nutzen std::uint8_t für u8, um sicher zu sein
using u8 = std::uint8_t;
using JanSordid::Core::f32;

namespace JanSordid::FantasyDragon
{
    class DragonGame : public JanSordid::SDL::Game<>
    {
    public:
        DragonGame()
           : JanSordid::SDL::Game<>( "Fantasy Dragon", { 1280, 720 } )
        {
            // States registrieren (mit vollem Namen, um Fehler zu vermeiden)
            // 0 = MainMenu
            this->AddStates<JanSordid::SDL_Example::MainMenuState>( *this );
            // 1 = Editor
            this->AddStates<JanSordid::SDL_Example::EditorState>( *this );
            // 2 = Settings
            this->AddStates<JanSordid::SDL_Example::SettingsState>( *this );

            // Start im Main Menu
            // Wir casten explizit auf uint8_t
            this->PushState( (u8)JanSordid::SDL_Example::GameStateID::MainMenu );
        }

        void Update( const f32 deltaT ) override    { JanSordid::SDL::Game<>::Update( deltaT ); }
        void Render( const f32 deltaT ) override    { JanSordid::SDL::Game<>::Render( deltaT ); }

#if IMGUI
        void RenderUI( const f32 deltaTNeeded ) override { JanSordid::SDL::Game<>::RenderUI( deltaTNeeded ); }
#endif
    };
}

int main( int argc, char * argv [] )
{
    using namespace JanSordid;

    try
    {
        std::ios_base::sync_with_stdio( false );
        FantasyDragon::DragonGame game;
        return game.Run();
    }
    catch( ... )
    {
        return -1;
    }
}