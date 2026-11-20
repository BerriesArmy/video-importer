#include "ImportPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

class $modify(VideoPauseLayer, EditorPauseLayer) {
    bool init(LevelEditorLayer* editor) {
        if (!EditorPauseLayer::init(editor)) return false;

        auto sprite = ButtonSprite::create("Video");
        sprite->setScale(.8f);
        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(VideoPauseLayer::onVideo)
        );
        auto menu = CCMenu::create();
        menu->addChild(button);

        auto size = CCDirector::get()->getWinSize();
        menu->setPosition({size.width * .74f, size.height * .62f});
        this->addChild(menu, 10);
        return true;
    }

    void onVideo(CCObject*) {
        ImportPopup::create()->show();
    }
};
