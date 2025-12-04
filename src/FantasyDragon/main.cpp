#include "sor/sdl_game.hpp"
#include "fantasydragon_mapeditor.hpp"

using JanSordid::Core::f32;
using JanSordid::Core::u8;

namespace JanSordid::FantasyDragon
{
    enum class GameStateID { Editor };

    class DragonGame : public JanSordid::SDL::Game<>
    {
    public:
        DragonGame()
           : JanSordid::SDL::Game<>( "Fantasy Dragon Editor", { 1980, 1020 } )
        {
            this->AddStates<JanSordid::SDL_Example::EditorState>( *this );
            this->PushState( (u8)GameStateID::Editor );
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