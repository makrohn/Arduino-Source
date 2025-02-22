/*  Stats Reset
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Exceptions.h"
#include "CommonFramework/Options/LanguageOCROption.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/InferenceInfra/InferenceRoutines.h"
#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Tools/VideoResolutionCheck.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonSV/PokemonSV_Settings.h"
#include "PokemonSV/Inference/Battles/PokemonSV_BattleMenuDetector.h"
#include "PokemonSV/Inference/Dialogs/PokemonSV_DialogDetector.h"
#include "PokemonSV/Inference/Boxes/PokemonSV_BoxDetection.h"
#include "PokemonSV/Inference/Boxes/PokemonSV_IVCheckerReader.h"
#include "PokemonSV/Programs/PokemonSV_GameEntry.h"
#include "PokemonSV/Programs/PokemonSV_Navigation.h"
#include "PokemonSV/Programs/PokemonSV_BasicCatcher.h"
#include "PokemonSV/Programs/Boxes/PokemonSV_BoxRoutines.h"
#include "PokemonSV/Programs/Eggs/PokemonSV_EggRoutines.h"
#include "PokemonSV_StatsReset.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{

using namespace Pokemon;
 
StatsReset_Descriptor::StatsReset_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonSV:StatsReset",
        STRING_POKEMON + " SV", "Stats Reset",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSV/StatsReset.md",
        "Repeatedly catch the Treasures of Ruin until you get the stats you want.",
        FeedbackType::REQUIRED, true,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}
struct StatsReset_Descriptor::Stats : public StatsTracker {
    Stats()
        : resets(m_stats["Resets"])
        , balls(m_stats["Balls Thrown"])
        , catches(m_stats["Catches"])
        , matches(m_stats["Matches"])
        , errors(m_stats["Errors"])
    {
        m_display_order.emplace_back("Resets");
        m_display_order.emplace_back("Balls Thrown");
        m_display_order.emplace_back("Catches");
        m_display_order.emplace_back("Matches");
        m_display_order.emplace_back("Errors", true);
    }
    std::atomic<uint64_t>& resets;
    std::atomic<uint64_t>& balls;
    std::atomic<uint64_t>& catches;
    std::atomic<uint64_t>& matches;
    std::atomic<uint64_t>& errors;
};
std::unique_ptr<StatsTracker> StatsReset_Descriptor::make_stats() const {
    return std::unique_ptr<StatsTracker>(new Stats());
}
StatsReset::StatsReset()
    : TARGET(
        "<b>Target:</b><br>The Pokemon you are resetting for.<br>"
        "Treasures of Ruin: Stand in front of the unsealed vaults of one of the Ruinous Quartet.<br>"
        "Generic: You are standing in front of a Pokemon that requires an A press to initiate battle.<br>",
        {
            {Target::TreasuresOfRuin, "treasures-of-ruin", "Treasures of Ruin"},
            {Target::Generic, "generic", "Generic"},
        },
        LockWhileRunning::LOCKED,
        Target::TreasuresOfRuin
    )
    ,LANGUAGE(
        "<b>Game Language:</b><br>This field is required so we can read IVs.",
        IV_READER().languages(),
        LockWhileRunning::LOCKED,
        false
    )
    ,BALL_SELECT(
        "<b>Ball Select:</b>",
        LockWhileRunning::UNLOCKED,
        "poke-ball"
    )
    , GO_HOME_WHEN_DONE(false)
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        & NOTIFICATION_PROGRAM_FINISH,
        & NOTIFICATION_ERROR_FATAL,
        })
{
    PA_ADD_OPTION(TARGET);
    PA_ADD_OPTION(LANGUAGE); //This is required
    PA_ADD_OPTION(BALL_SELECT);
    PA_ADD_OPTION(FILTERS); //Note: None of these can be shiny, and the quartet will have some perfect IVs.
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(NOTIFICATIONS);
}

void StatsReset::program(SingleSwitchProgramEnvironment& env, BotBaseContext& context) {
    //This will only work for Pokemon that you press A to talk to.
    //Regular static spawns will have the same stats, resetting won't work.
    //Won't apply to the former titan pokemon or the box legend either, as their IVs are locked.
    //So this really only applies to the ruinous quartet.
    assert_16_9_720p_min(env.logger(), env.console);
    StatsReset_Descriptor::Stats& stats = env.current_stats<StatsReset_Descriptor::Stats>();

    //Autosave must be off, settings like Tera farmer.
    bool stats_matched = false;
    while (!stats_matched) {
        AdvanceDialogWatcher advance_detector(COLOR_YELLOW);
        pbf_press_button(context, BUTTON_A, 10, 50);
        int retD = wait_until(env.console, context, Milliseconds(4000), { advance_detector });
        if (retD < 0) {
            env.log("Dialog detected.");
        }
        switch (TARGET) {
        case Target::TreasuresOfRuin:
            //~30 seconds to start battle?
            pbf_mash_button(context, BUTTON_A, 3250);
            context.wait_for_all_requests();
            break;
        case Target::Generic:
            //Mash A to initiate battle
            pbf_mash_button(context, BUTTON_A, 90);
            context.wait_for_all_requests();
            break;
        default:
            throw InternalProgramError(&env.logger(), PA_CURRENT_FUNCTION, "Unknown Target");
        }
        NormalBattleMenuWatcher battle_menu(COLOR_YELLOW);
        int ret = wait_until(
            env.console, context,
            std::chrono::seconds(15),
            { battle_menu }
        );
        if(ret != 0) {
            stats.errors++;
            env.update_stats();
            throw OperationFailedException(env.console, "Failed to enter battle. Are you facing the Pokemon or in a menu?");
        }
        bool battle_ended = false;
        while (!battle_ended) {
            //Navigate to correct ball and repeatedly throw it until caught
            pbf_press_button(context, BUTTON_X, 20, 100);
            context.wait_for_all_requests();

            BattleBallReader reader(env.console, LANGUAGE);
            int quantity = move_to_ball(reader, env.console, context, BALL_SELECT.slug());
            if (quantity == 0) {
                env.console.log("Unable to find appropriate ball, out of balls, or either Pokemon fainted.");
                send_program_status_notification(
                    env, NOTIFICATION_STATUS_UPDATE,
                    "Unable to find appropriate ball, out of balls, or either Pokemon fainted."
                );
                break;
            }
            if (quantity < 0) {
                stats.errors++;
                env.update_stats();
                env.console.log("Unable to read ball quantity.", COLOR_RED);
            }
            //Throw ball/Wild pokemon's turn/wait for catch animation
            pbf_mash_button(context, BUTTON_A, 150);
            stats.balls++;
            env.update_stats();
            //pbf_wait(context, 4000);
            pbf_mash_button(context, BUTTON_B, 1000);
            context.wait_for_all_requests();

            AdvanceDialogWatcher summary(COLOR_MAGENTA);
            int ret2 = wait_until(
                env.console, context,
                std::chrono::seconds(25),
                { summary, battle_menu }
            );
            if (ret2 == 0) {
                env.log("Dialog detected, assuming caught.");
                stats.catches++;
                env.update_stats();
                battle_ended = true;
            }
            //ret2 == 1 battle menu, continue
        }
        if (battle_ended) {
            //Close all the dex entry and caught menus
            pbf_mash_button(context, BUTTON_B, 100);
            context.wait_for_all_requests();

            //Open box and navigate to last party slot
            enter_box_system_from_overworld(env.program_info(), env.console, context);
            context.wait_for(std::chrono::milliseconds(400));
            move_box_cursor(env.program_info(), env.console, context, BoxCursorLocation::PARTY, 5, 0);

            //Check the IVs of the newly caught Pokemon - *must be on IV panel*
            EggHatchAction action = EggHatchAction::Keep;
            check_baby_info(env.program_info(), env.console, context, LANGUAGE, FILTERS, action);

            switch (action) {
            case EggHatchAction::StopProgram:
                //Correct stats found, end program
                stats_matched = true;
                env.console.log("Program stop requested...");
                send_program_status_notification(
                    env, NOTIFICATION_PROGRAM_FINISH,
                    "Match found!"
                );
                break;
            case EggHatchAction::Keep:
                //Correct stats found, end program
                stats_matched = true;
                env.console.log("Match found!");
                stats.matches++;
                env.update_stats();
                send_program_status_notification(
                    env, NOTIFICATION_PROGRAM_FINISH,
                    "Match found!"
                );
                break;
            case EggHatchAction::Release:
                stats_matched = false;
                battle_ended = false; //Set this back and use the reset below
                env.console.log("Stats did not match table settings.");
                send_program_status_notification(
                    env, NOTIFICATION_STATUS_UPDATE,
                    "Stats did not match table settings."
                );
                break;
            default:
                env.console.log("Invalid state.");
                stats.errors++;
                env.update_stats();
                throw OperationFailedException(env.console, "Invalid state.");
                break;
            }
        }
        if (!battle_ended) {
            //Reset game
            stats.resets++;
            env.update_stats();
            pbf_press_button(context, BUTTON_HOME, 20, GameSettings::instance().GAME_TO_HOME_DELAY);
            reset_game_from_home(env, env.console, context, 5 * TICKS_PER_SECOND);
        }
    }
    env.update_stats();
    GO_HOME_WHEN_DONE.run_end_of_program(context);
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}
    
}
}
}
