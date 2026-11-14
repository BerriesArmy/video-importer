#include "ImportPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

class $modify(VideoPauseLayer, EditorPauseLayer) {
    bool init(LevelEditorLayer* editor) {
        if (!EditorPauseLayer::init(editor)) return false;

        auto button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Video"), this, menu_selector(VideoPauseLayer::onVideo)
        );
        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setPosition({55.f, 28.f});
        this->addChild(menu, 10);
        return true;
    }

    void onVideo(CCObject*) {
        ImportPopup::create()->show();
    }
};
