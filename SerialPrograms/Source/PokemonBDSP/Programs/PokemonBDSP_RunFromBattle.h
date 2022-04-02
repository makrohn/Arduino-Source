/*  Run from Battle
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_RunFromBattle_H
#define PokemonAutomation_PokemonBDSP_RunFromBattle_H

#include "CommonFramework/Tools/ProgramEnvironment.h"
#include "CommonFramework/Tools/ConsoleHandle.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{

// Run from a wild battle.
// The cursor must be over the "Run" button when starting this function.
// It uses feedback to ensure battle ends by detecting a black screen.
// Return true if a black screen is detected, false if not detected after
// `exit_battle_time` of ticks have passed. 
bool run_from_battle(
    ProgramEnvironment& env, BotBaseContext& context, ConsoleHandle& console,
    uint16_t exit_battle_time
);


}
}
}
#endif
