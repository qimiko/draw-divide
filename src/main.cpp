#include <Windows.h>
#include <hackpro_ext.h>
#include <gd.h>
#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include "nodes.hpp"

// making it easier to port to some other framework later on ;)
using namespace gd;

// to get around protected modifier
class CCDirectorVisible : public cocos2d::CCDirector {
public:
	void calculateDeltaTime() {
		CCDirector::calculateDeltaTime();
	};

	void setNextScene() {
		CCDirector::setNextScene();
	}
};

double frame_remainder = 0;
int target_fps = 60;
int frame_counter = 0;
bool enabled = false;

void CCDirector_drawScene(cocos2d::CCDirector* self) {
	if (!enabled) {
		return matdash::orig<&CCDirector_drawScene>(self);
	}

	// scary floats
	// getAnimationInterval is 1/fps bypass
	// 1/((1/fps bypass) * target) = fps bypass/target
	const double thing = 1.f / (self->getAnimationInterval() * static_cast<double>(target_fps));

	frame_counter++;

	// run full scene draw (glClear, visit) each time the counter is full
	if (static_cast<double>(frame_counter) + frame_remainder >= thing) {
		frame_remainder += static_cast<double>(frame_counter) - thing;
		frame_counter = 0;
		return matdash::orig<&CCDirector_drawScene>(self);
	}

	// otherwise, we only run updates

	// upcast to remove protection
	auto visible_director = static_cast<CCDirectorVisible*>(self);

	// this line seems to create a speedhack
	// visible_director->calculateDeltaTime();

	if (!self->isPaused()) {
		self->getScheduler()->update(self->getDeltaTime());
	}

	if (self->getNextScene()) {
		visible_director->setNextScene();
	}
}

void __stdcall on_text_input(void* tb) {
	auto text = HackproGetTextBoxText(tb);

	try {
		target_fps = std::stoi(text);
	} catch (const std::exception&) {
		// generic catch all for things like null strings or bad values
		target_fps = 0;
	}
}

void __stdcall on_checkbox_enable(void* cb) {
	enabled = true;
}

void __stdcall on_checkbox_disable(void* cb) {
	enabled = false;
}

bool has_mega_hack = false;
void* mh_checkbox;
void* mh_text_box;


int target_fps_bypass = 60;
bool fps_bypass_enabled = false;

class MyMoreVideoOptionsLayer : public FLAlertLayer {
public:
	bool init_() {
		if (!matdash::orig<&MyMoreVideoOptionsLayer::init_>(this)) return false;

		auto* check_off_sprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
		auto* check_on_sprite = cocos2d::CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

		check_off_sprite->setScale(0.8f);
		check_on_sprite->setScale(0.8f);

		// freaking macro
		using namespace cocos2d;

		auto* toggler1 = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(MyMoreVideoOptionsLayer::on_toggle_fps_bypass));
		toggler1->setPosition({-135, 10});
		toggler1->toggle(enabled);
		m_pButtonMenu->addChild(toggler1);

		auto* toggler2 = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(MyMoreVideoOptionsLayer::on_toggle_draw_divider));
		toggler2->setPosition({-135, -38});
		toggler2->toggle(fps_bypass_enabled);
		m_pButtonMenu->addChild(toggler2);


		auto* label1 = cocos2d::CCLabelBMFont::create("FPS Bypass", "bigFont.fnt");
		label1->setPosition({171.5f, 170});
		label1->setScale(0.5f);
		label1->setAnchorPoint({0.f, 0.5f});
		m_pLayer->addChild(label1);

		auto* label2 = cocos2d::CCLabelBMFont::create("Draw Rate", "bigFont.fnt");
		label2->setPosition({171.5f, 121});
		label2->setScale(0.5f);
		label2->setAnchorPoint({0.f, 0.5f});
		m_pLayer->addChild(label2);

		auto* fps_input = NumberInputNode::create(CCSize(48, 28), 1.5f, "bigFont.fnt");
		fps_input->set_value(target_fps_bypass);
		fps_input->input_node->setMaxLabelScale(0.7f);
		fps_input->setPosition({331, 170});
		fps_input->callback = [this](NumberInputNode& node) {
			target_fps_bypass = node.get_value();
			if (target_fps_bypass <= 0) target_fps_bypass = 1;
			if (fps_bypass_enabled) {
				update_fps_bypass();
			}
		};
		m_pLayer->addChild(fps_input);

		auto* div_input = NumberInputNode::create(CCSize(48, 28), 1.5f, "bigFont.fnt");
		div_input->set_value(target_fps);
		div_input->input_node->setMaxLabelScale(0.7f);
		div_input->setPosition({331, 121});
		div_input->callback = [](NumberInputNode& node) {
			target_fps = node.get_value();
			if (target_fps <= 0) target_fps = 1;
			if (has_mega_hack) {
				HackproSetTextBoxText(mh_text_box, std::to_string(target_fps).c_str());
			}
		};
		m_pLayer->addChild(div_input);

		return true;
	}

	void update_fps_bypass() {
		auto* app = cocos2d::CCApplication::sharedApplication();
		app->setAnimationInterval(1.0 / static_cast<double>(target_fps_bypass));
	}

	void on_toggle_fps_bypass(CCObject* obj) {
		auto* toggler = static_cast<CCMenuItemToggler*>(obj);
		auto* app = cocos2d::CCApplication::sharedApplication();
		if (!toggler->isOn()) {
			app->toggleVerticalSync(false);
			auto* vsync_toggler = static_cast<CCMenuItemToggler*>(m_pButtonMenu->getChildByTag(30));
			if (vsync_toggler) {
				vsync_toggler->toggle(false);
			}
			update_fps_bypass();
		} else {
			app->setAnimationInterval(1.0 / 60.0);
		}
		fps_bypass_enabled = !toggler->isOn();
	}

	void on_toggle_draw_divider(CCObject* obj) {
		auto* toggler = static_cast<CCMenuItemToggler*>(obj);
		enabled = !toggler->isOn();
		if (has_mega_hack) {
			HackproSetCheckbox(mh_checkbox, enabled);
		}
	}
};

void mod_main(HMODULE) {
	auto cocos_handle = LoadLibraryA("libcocos2d.dll");

	matdash::add_hook<&CCDirector_drawScene>(GetProcAddress(cocos_handle, "?drawScene@CCDirector@cocos2d@@QAEXXZ"));

	matdash::add_hook<&MyMoreVideoOptionsLayer::init_>(gd::base + 0x1E2590);

	if (InitialiseHackpro() && HackproIsReady()) {
		auto ext = HackproInitialiseExt("Draw Divide");

		auto tb = HackproAddTextBox(ext, on_text_input);
		HackproSetTextBoxPlaceholder(tb, "FPS");
		HackproSetTextBoxText(tb, std::to_string(target_fps).c_str());

		auto cb = HackproAddCheckbox(ext, "Enable", on_checkbox_enable, on_checkbox_disable);
		HackproSetCheckbox(cb, enabled);

		mh_text_box = tb;
		mh_checkbox = cb;

		HackproCommitExt(ext);
		has_mega_hack = true;
	}
}
