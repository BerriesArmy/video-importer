#include "ExtrasPopup.hpp"

using namespace geode::prelude;

ExtrasPopup* ExtrasPopup::create(BuildInfo const& info) {
    auto popup = new ExtrasPopup();
    if (popup->init(info)) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool ExtrasPopup::init(BuildInfo const& info) {
    if (!Popup::init(340.f, 260.f)) return false;

    m_info = info;
    this->setTitle("Video placed");

    auto note = CCLabelBMFont::create("Choose what else to add", "chatFont.fnt");
    note->setScale(.8f);
    m_mainLayer->addChildAtPosition(note, Anchor::Center, ccp(0.f, 80.f));

    m_hidePlayer = this->addOption("Hide the player", "extra-hide-player", false, 50.f);
    m_lockCamera = this->addOption("Keep video on screen", "extra-lock-camera", true, 20.f);
    m_loop = this->addOption("Loop the video", "extra-loop", false, -10.f);
    m_endLevel = this->addOption("End level when video ends", "extra-end-level", false, -40.f);
    m_blackBackground = this->addOption("Black background", "extra-black-background", false, -70.f);

    auto apply = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Apply"), this, menu_selector(ExtrasPopup::onApply)
    );
    m_buttonMenu->addChildAtPosition(apply, Anchor::Bottom, ccp(0.f, 28.f));
    return true;
}

CCMenuItemToggler* ExtrasPopup::addOption(char const* text, char const* key, bool fallback, float y) {
    auto toggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ExtrasPopup::onToggle), .6f);
    toggler->toggle(Mod::get()->getSavedValue<bool>(key, fallback));
    m_buttonMenu->addChildAtPosition(toggler, Anchor::Center, ccp(-135.f, y));

    auto label = CCLabelBMFont::create(text, "bigFont.fnt");
    label->setScale(.45f);
    label->setAnchorPoint({0.f, .5f});
    m_mainLayer->addChildAtPosition(label, Anchor::Center, ccp(-115.f, y));
    return toggler;
}

void ExtrasPopup::onToggle(CCObject*) {}

void ExtrasPopup::onApply(CCObject*) {
    Extras extras;
    extras.hidePlayer = m_hidePlayer->isToggled();
    extras.lockCamera = m_lockCamera->isToggled();
    extras.loop = m_loop->isToggled();
    extras.endLevel = m_endLevel->isToggled();
    extras.blackBackground = m_blackBackground->isToggled();

    auto mod = Mod::get();
    mod->setSavedValue<bool>("extra-hide-player", extras.hidePlayer);
    mod->setSavedValue<bool>("extra-lock-camera", extras.lockCamera);
    mod->setSavedValue<bool>("extra-loop", extras.loop);
    mod->setSavedValue<bool>("extra-end-level", extras.endLevel);
    mod->setSavedValue<bool>("extra-black-background", extras.blackBackground);

    std::string objects = buildExtras(extras, m_info);
    if (!objects.empty()) {
        if (auto editor = LevelEditorLayer::get()) {
            editor->createObjectsFromString(objects, true, true);
        }
    }

    Notification::create("Extras added", NotificationIcon::Success)->show();
    this->onClose(nullptr);
}
