/*  Egg Fetcher
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Exceptions.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Tools/VideoResolutionCheck.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonSV/Programs/Eggs/PokemonSV_EggRoutines.h"
#include "PokemonSV/Programs/PokemonSV_Navigation.h"
#include "PokemonSV_EggFetcher.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{

using namespace Pokemon;


EggFetcher_Descriptor::EggFetcher_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonSV:EggFetcher",
        STRING_POKEMON + " SV", "Egg Fetcher",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSV/EggFetcher.md",
        "Automatically fetch eggs from a picnic.",
        FeedbackType::REQUIRED, false,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}
struct EggFetcher_Descriptor::Stats : public StatsTracker{
    Stats()
        : m_sandwiches(m_stats["Sandwiches"])
        , m_attempts(m_stats["Fetch Attempts"])
        , m_eggs(m_stats["Eggs"])
        , m_errors(m_stats["Errors"])
    {
        m_display_order.emplace_back("Sandwiches");
        m_display_order.emplace_back("Fetch Attempts");
        m_display_order.emplace_back("Eggs");
        m_display_order.emplace_back("Errors", true);
    }
    std::atomic<uint64_t>& m_sandwiches;
    std::atomic<uint64_t>& m_attempts;
    std::atomic<uint64_t>& m_eggs;
    std::atomic<uint64_t>& m_errors;
};
std::unique_ptr<StatsTracker> EggFetcher_Descriptor::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}



EggFetcher::EggFetcher()
    : GO_HOME_WHEN_DONE(false)
    , EGGS_TO_FETCH(
        "<b>Fetch this many eggs:</b>",
        LockWhileRunning::LOCKED,
        900
    )
    , MAX_NUM_SANDWICHES(
        "<b>Max number of sandwiches to make:</b>",
        LockWhileRunning::UNLOCKED,
        10, 1
    )
    , EGG_SANDWICH_TYPE(
        "<b>Sandwich:</b><br>Which sandwich to get egg power.<br>"
        "Great Peanut Butter Sandwich: Use recipe No. 17. Must have enough ingredients to make it and ALL the other unlocked sandwich recipes for reliable recipe detection.<br>"
        "Two Sweet Herbs and Lettuce: use the Lettuce and two sweet herbs. Must provide Sweet Herb location on the condiments list.",
        {
            {EggSandwichType::GREAT_PEANUT_BUTTER, "great-peanut-butter", "Great Peanut Butter Sandwich"},
            {EggSandwichType::TWO_SWEET_HERBS, "two-sweet-herbs", "Two Sweet Herbs and Lettuce"},
        },
        LockWhileRunning::UNLOCKED,
        EggSandwichType::GREAT_PEANUT_BUTTER
    )
    , SWEET_HERB_INDEX_BACKWARDS(
        "<b>Sweet Herb Location:</b><br>If chosen the Two Sweet Herbs as the sandwich, where the Sweet Herb is on the condiments list.",
        {
            {0, "0", "Last on list"},
            {1, "1", "2nd last on list"},
            {2, "2", "3rd last on list"},
            {3, "3", "4th last on list"},
            {4, "4", "5th last on list (the location if you have all types of herbs)"},
        },
        LockWhileRunning::UNLOCKED,
        4
    )
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(EGGS_TO_FETCH);
    PA_ADD_OPTION(MAX_NUM_SANDWICHES);
    PA_ADD_OPTION(EGG_SANDWICH_TYPE);
    PA_ADD_OPTION(SWEET_HERB_INDEX_BACKWARDS);
    PA_ADD_OPTION(NOTIFICATIONS);
}


void EggFetcher::program(SingleSwitchProgramEnvironment& env, BotBaseContext& context){
    assert_16_9_720p_min(env.logger(), env.console);

    EggFetcher_Descriptor::Stats& stats = env.current_stats<EggFetcher_Descriptor::Stats>();

    //  Connect the controller.
    pbf_press_button(context, BUTTON_LCLICK, 10, 0);

    size_t num_eggs_collected = 0;

    try{
        for(uint16_t i = 0; i < MAX_NUM_SANDWICHES; i++){
            send_program_status_notification(env, NOTIFICATION_STATUS_UPDATE);

            picnic_at_zero_gate(env.program_info(), env.console, context);
            // Now we are at picnic. We are at one end of picnic table while the egg basket is at the other end
    
            bool can_make_sandwich = eat_egg_sandwich_at_picnic(env.program_info(), env.realtime_dispatcher(), env.console, context,
                EGG_SANDWICH_TYPE, SWEET_HERB_INDEX_BACKWARDS.current_value());
            if (can_make_sandwich == false){
                throw UserSetupError(env.console, "No sandwich recipe or ingredients. Cannot open and select the sandwich recipe.");
            }
            stats.m_sandwiches++;
            env.update_stats();

            // move past the table and collect eggs
            auto basket_check_callback = [&](size_t new_eggs){
                stats.m_attempts++;
                stats.m_eggs += new_eggs;
                env.update_stats();
            };

            const size_t basket_wait_seconds = (EGG_SANDWICH_TYPE == EggSandwichType::GREAT_PEANUT_BUTTER ? 180 : 120);
            collect_eggs_after_sandwich(env.program_info(), env.console, context, basket_wait_seconds,
                EGGS_TO_FETCH, num_eggs_collected, basket_check_callback);

            leave_picnic(env.program_info(), env.console, context);
            
            // Reset position to flying spot:
            reset_position_at_zero_gate(env.program_info(), env.console, context);

            if (num_eggs_collected >= EGGS_TO_FETCH){
                break;
            }
        }
    } catch(OperationFailedException& e){
        stats.m_errors++;
        env.update_stats();
        throw e;
    }

    env.update_stats();
    GO_HOME_WHEN_DONE.run_end_of_program(context);
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}






}
}
}
