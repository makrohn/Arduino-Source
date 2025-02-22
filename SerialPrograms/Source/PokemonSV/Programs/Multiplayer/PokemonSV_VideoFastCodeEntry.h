/*  Video Fast Code Entry
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSV_VideoFastCodeEntry_H
#define PokemonAutomation_PokemonSV_VideoFastCodeEntry_H

#include "Common/Cpp/Options/BooleanCheckBoxOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "CommonFramework/Options/ScreenWatchOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/NintendoSwitch_MultiSwitchProgram.h"
#include "NintendoSwitch/Programs/NintendoSwitch_FastCodeEntry.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{



class VideoFastCodeEntry_Descriptor : public MultiSwitchProgramDescriptor{
public:
    VideoFastCodeEntry_Descriptor();
};




class VideoFastCodeEntry : public MultiSwitchProgramInstance{
public:
    VideoFastCodeEntry();
    virtual void program(MultiSwitchProgramEnvironment& env, CancellableScope& scope) override;

private:
    enum class Mode{
        MANUAL,
        AUTOMATIC,
    };

    ScreenWatchOption SCREEN_WATCHER;
    EnumDropdownOption<Mode> MODE;
    BooleanCheckBoxOption SKIP_CONNECT_TO_CONTROLLER;
    FastCodeEntrySettingsOption SETTINGS;
    EventNotificationsOption NOTIFICATIONS;
};




}
}
}
#endif
