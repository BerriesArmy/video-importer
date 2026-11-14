#pragma once

#include <Geode/Geode.hpp>

#include <filesystem>
#include <memory>

struct ImportJob;

class ImportPopup : public geode::Popup {
public:
    static ImportPopup* create();

protected:
    bool init();
    void update(float dt) override;

private:
    std::shared_ptr<ImportJob> m_job;
    size_t m_nextChunk = 0;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    CCMenuItemSpriteExtra* m_fileButton = nullptr;
    CCMenuItemSpriteExtra* m_folderButton = nullptr;

    void onPickFile(cocos2d::CCObject*);
    void onPickFolder(cocos2d::CCObject*);
    void startJob(std::filesystem::path source);
    void setBusy(bool busy);
    void fail(std::string const& message);
};
