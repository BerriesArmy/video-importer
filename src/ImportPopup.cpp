#include "ImportPopup.hpp"

#include "Config.hpp"
#include "ExtrasPopup.hpp"
#include "Planner.hpp"

#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>

#include <atomic>
#include <thread>

using namespace geode::prelude;

struct ImportJob {
    std::atomic<int> state{0};
    std::string error;
    BuiltLevel built;
};

namespace {

constexpr char const* idleText = "Pick a GIF, an image or a folder of frames";

}

ImportPopup* ImportPopup::create() {
    auto popup = new ImportPopup();
    if (popup->init()) {
        popup->autorelease();
        return popup;
    }
    delete popup;
    return nullptr;
}

bool ImportPopup::init() {
    if (!Popup::init(360.f, 220.f)) return false;

    this->setTitle("Import video");

    m_status = CCLabelBMFont::create(idleText, "chatFont.fnt");
    m_status->setScale(.8f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Center, ccp(0.f, 45.f));

    auto hint = CCLabelBMFont::create("Grid size and frame rate are in the mod settings", "chatFont.fnt");
    hint->setScale(.6f);
    hint->setOpacity(160);
    m_mainLayer->addChildAtPosition(hint, Anchor::Bottom, ccp(0.f, 28.f));

    m_fileButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("GIF or image"), this, menu_selector(ImportPopup::onPickFile)
    );
    m_folderButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Frame folder"), this, menu_selector(ImportPopup::onPickFolder)
    );
    m_buttonMenu->addChildAtPosition(m_fileButton, Anchor::Center, ccp(-80.f, -5.f));
    m_buttonMenu->addChildAtPosition(m_folderButton, Anchor::Center, ccp(80.f, -5.f));

    this->scheduleUpdate();
    return true;
}

void ImportPopup::onPickFile(CCObject*) {
    file::FilePickOptions options;
    options.filters.push_back({"Images and GIFs", {"*.gif", "*.png", "*.jpg", "*.jpeg", "*.bmp"}});

    Ref<ImportPopup> self(this);
    async::spawn(
        file::pick(file::PickMode::OpenFile, options),
        [self](Result<std::optional<std::filesystem::path>> result) {
            if (!result.isOk()) return;
            auto picked = result.unwrap();
            if (!picked) return;
            self->startJob(*picked);
        }
    );
}

void ImportPopup::onPickFolder(CCObject*) {
    Ref<ImportPopup> self(this);
    async::spawn(
        file::pick(file::PickMode::OpenFolder, file::FilePickOptions{}),
        [self](Result<std::optional<std::filesystem::path>> result) {
            if (!result.isOk()) return;
            auto picked = result.unwrap();
            if (!picked) return;
            self->startJob(*picked);
        }
    );
}

void ImportPopup::setBusy(bool busy) {
    m_fileButton->setEnabled(!busy);
    m_folderButton->setEnabled(!busy);
}

void ImportPopup::fail(std::string const& message) {
    Notification::create(message, NotificationIcon::Error)->show();
    m_status->setString(idleText);
    this->setBusy(false);
    m_job.reset();
}

void ImportPopup::startJob(std::filesystem::path source) {
    if (m_job) return;
    if (!LevelEditorLayer::get()) {
        this->fail("Open a level in the editor first");
        return;
    }

    ImportConfig config = readConfig();
    auto job = std::make_shared<ImportJob>();
    m_job = job;
    m_nextChunk = 0;
    this->setBusy(true);
    m_status->setString("Reading frames...");

    std::thread([job, config, source = std::move(source)] {
        try {
            Clip clip = loadClip(source, config.load);
            if (!clip.error.empty()) {
                job->error = clip.error;
                job->state = 2;
                return;
            }
            Plan plan = makePlan(clip, config.threshold);
            job->built = buildLevel(plan, config.layout);
            job->state = 1;
        } catch (std::exception const& e) {
            job->error = e.what();
            job->state = 2;
        }
    }).detach();
}

void ImportPopup::update(float) {
    if (!m_job) return;

    int state = m_job->state.load();
    if (state == 0) return;

    if (state == 2) {
        this->fail(m_job->error);
        return;
    }

    auto editor = LevelEditorLayer::get();
    if (!editor) {
        this->fail("The editor is no longer open");
        return;
    }

    auto& chunks = m_job->built.chunks;
    if (m_nextChunk < chunks.size()) {
        editor->createObjectsFromString(chunks[m_nextChunk], true, true);
        ++m_nextChunk;
        m_status->setString(fmt::format("Placing objects {}/{}", m_nextChunk, chunks.size()).c_str());
        return;
    }

    BuildInfo info = m_job->built.info;
    m_job.reset();
    ExtrasPopup::create(info)->show();
    this->onClose(nullptr);
}
