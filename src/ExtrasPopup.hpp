#pragma once

#include "Writer.hpp"

#include <Geode/Geode.hpp>

class ExtrasPopup : public geode::Popup {
public:
    static ExtrasPopup* create(BuildInfo const& info);

protected:
    bool init(BuildInfo const& info);

private:
    BuildInfo m_info;
    CCMenuItemToggler* m_hidePlayer = nullptr;
    CCMenuItemToggler* m_lockCamera = nullptr;
    CCMenuItemToggler* m_loop = nullptr;
    CCMenuItemToggler* m_endLevel = nullptr;
    CCMenuItemToggler* m_blackBackground = nullptr;

    CCMenuItemToggler* addOption(char const* text, char const* key, bool fallback, float y);
    void onToggle(cocos2d::CCObject*);
    void onApply(cocos2d::CCObject*);
};
