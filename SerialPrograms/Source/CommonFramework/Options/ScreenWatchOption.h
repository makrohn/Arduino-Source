/*  Screen Watch Option
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_Options_ScreenWatchOption_H
#define PokemonAutomation_Options_ScreenWatchOption_H

#include "Common/Cpp/Options/GroupOption.h"
#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/FloatingPointOption.h"
#include "CommonFramework/VideoPipeline/VideoOverlaySession.h"

namespace PokemonAutomation{

struct VideoSnapshot;
class ScreenWatchOption;


class ScreenWatchDisplay : public ConfigOption{
public:
    ScreenWatchDisplay(ScreenWatchOption& option)
        : m_option(option)
    {}

    virtual ConfigWidget* make_QtWidget(QWidget& parent) override;

    ScreenWatchOption& m_option;
};
class ScreenWatchButtons : public ConfigOption{
public:
    ScreenWatchButtons(ScreenWatchOption& option)
        : m_option(option)
    {}

    virtual ConfigWidget* make_QtWidget(QWidget& parent) override;

    ScreenWatchOption& m_option;
};



class ScreenWatchOption : public GroupOption{
public:
    ~ScreenWatchOption();
    ScreenWatchOption(
        std::string label,
        double default_x, double default_y,
        double default_width, double default_height
    );

//    virtual void load_json(const JsonValue& json) override;
//    virtual JsonValue to_json() const override;

//    virtual void restore_defaults() override;

//    virtual ConfigWidget* make_QtWidget(QWidget& parent) override;

    double aspect_ratio();
    VideoSnapshot screenshot();

    VideoOverlaySession& overlay();

public:
    SimpleIntegerOption<uint64_t> MONITOR_INDEX;
    FloatingPointOption X;
    FloatingPointOption Y;
    FloatingPointOption WIDTH;
    FloatingPointOption HEIGHT;

private:
    ScreenWatchDisplay m_display;
    ScreenWatchButtons m_buttons;

    VideoOverlayOption m_overlay_option;
    VideoOverlaySession m_overlay;
};






}
#endif
