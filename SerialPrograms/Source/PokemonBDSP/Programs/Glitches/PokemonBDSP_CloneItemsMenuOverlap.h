/*  Clone Items (Menu Overlap Method)
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_CloneItemsMenuOverlap_H
#define PokemonAutomation_PokemonBDSP_CloneItemsMenuOverlap_H

#include "CommonFramework/Options/StaticTextOption.h"
#include "CommonFramework/Options/SimpleIntegerOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/Options/GoHomeWhenDoneOption.h"
#include "NintendoSwitch/Options/TimeExpressionOption.h"
#include "NintendoSwitch/Framework/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


class CloneItemsMenuOverlap_Descriptor : public RunnableSwitchProgramDescriptor{
public:
    CloneItemsMenuOverlap_Descriptor();
};



class CloneItemsMenuOverlap : public SingleSwitchProgramInstance{
public:
    CloneItemsMenuOverlap(const CloneItemsMenuOverlap_Descriptor& descriptor);

    virtual std::unique_ptr<StatsTracker> make_stats() const override;
    virtual void program(SingleSwitchProgramEnvironment& env, BotBaseContext& context) override;

private:
    struct Stats;

    bool trigger_encounter(ProgramEnvironment& env, BotBaseContext& context, ConsoleHandle& console);
    void swap_party(BotBaseContext& context, ConsoleHandle& console);
    void mash_B_to_battle(ProgramEnvironment& env, BotBaseContext& context, ConsoleHandle& console);
    void detach_items(BotBaseContext& context, ConsoleHandle& console);

private:
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;
    SimpleIntegerOption<uint16_t> BATCHES;

    SimpleIntegerOption<uint16_t> SAVE_INTERVAL;

    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationsOption NOTIFICATIONS;

    SectionDividerOption m_advanced_options;
    TimeExpressionOption<uint16_t> EXIT_BATTLE_TIMEOUT;
};




}
}
}
#endif
