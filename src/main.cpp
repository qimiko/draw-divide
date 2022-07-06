#include <Windows.h>
#include <hackpro_ext.h>
#include <gd.h>
#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include "nodes.hpp"
#include <chrono>

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

void save_gv_values();

void __stdcall on_text_input(void* tb) {
	auto text = HackproGetTextBoxText(tb);

	try {
		target_fps = std::stoi(text);
	} catch (const std::exception&) {
		// generic catch all for things like null strings or bad values
		target_fps = 0;
	}
	save_gv_values();
}

void __stdcall on_checkbox_enable(void* cb) {
	enabled = true;
	save_gv_values();
}

void __stdcall on_checkbox_disable(void* cb) {
	enabled = false;
	save_gv_values();
}

bool has_mega_hack = false;
void* mh_checkbox;
void* mh_text_box;
void *mh_fps_checkbox;

int target_fps_bypass = 60;
bool fps_bypass_enabled = false;
bool show_fps = false;

void __stdcall on_fps_checkbox_enable(void* cb) {
	show_fps = true;
	save_gv_values();
}

void __stdcall on_fps_checkbox_disable(void* cb) {
	show_fps = false;
	save_gv_values();
}

enum class Corner {
	TopLeft = 0,
	TopRight = 1,
	BotLeft = 2,
	BotRight = 3
};

Corner fps_corner = Corner::TopLeft;

void load_gv_values() {
	auto* gm = GameManager::sharedState();
	enabled = gm->getGameVariableDefault("draw-divide_enabled", false);
	target_fps = gm->getIntGameVariableDefault("draw-divide_fps", 60);
	fps_bypass_enabled = gm->getGameVariableDefault("draw-divide_fps_bypass_enabled", false);
	target_fps_bypass = gm->getIntGameVariableDefault("draw-divide_fps_bypass", 60);
	show_fps = gm->getGameVariableDefault("draw-divide_show_fps", false);
	fps_corner = static_cast<Corner>(gm->getIntGameVariableDefault("draw-divide_fps_corner", static_cast<int>(fps_corner)));
}

void save_gv_values() {
	auto* gm = GameManager::sharedState();
	gm->setGameVariable("draw-divide_enabled", enabled);
	gm->setIntGameVariable("draw-divide_fps", target_fps);
	gm->setGameVariable("draw-divide_fps_bypass_enabled", fps_bypass_enabled);
	gm->setIntGameVariable("draw-divide_fps_bypass", target_fps_bypass);
	gm->setGameVariable("draw-divide_show_fps", show_fps);
	gm->setIntGameVariable("draw-divide_fps_corner", static_cast<int>(fps_corner));
}

// incredible
class OnLeaveNode : public cocos2d::CCNode {
	std::function<void()> m_func = []{};
public:
	~OnLeaveNode() {
		m_func();
	}

	static auto* create(const std::function<void()>& func) {
		auto* node = new (std::nothrow) OnLeaveNode();
		if (node && node->init()) {
			node->m_func = func;
			node->autorelease();
		} else {
			CC_SAFE_DELETE(node);
		}
		return node;
	}
};

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

		auto* fps_toggler = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(MyMoreVideoOptionsLayer::on_toggle_fps_bypass));
		fps_toggler->setPosition({-135, 10});
		fps_toggler->toggle(fps_bypass_enabled);
		m_pButtonMenu->addChild(fps_toggler);

		auto* div_toggler = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(MyMoreVideoOptionsLayer::on_toggle_draw_divider));
		div_toggler->setPosition({-135, -39});
		div_toggler->toggle(enabled);
		m_pButtonMenu->addChild(div_toggler);


		auto* fps_label = cocos2d::CCLabelBMFont::create("FPS Bypass", "bigFont.fnt");
		fps_label->setPosition({171.5f, 170});
		fps_label->setScale(0.5f);
		fps_label->setAnchorPoint({0.f, 0.5f});
		m_pLayer->addChild(fps_label);

		auto* div_label = cocos2d::CCLabelBMFont::create("Draw Rate", "bigFont.fnt");
		div_label->setPosition({171.5f, 121});
		div_label->setScale(0.5f);
		div_label->setAnchorPoint({0.f, 0.5f});
		m_pLayer->addChild(div_label);

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

		addChild(OnLeaveNode::create([] {
			save_gv_values();
		}));

		{
			auto* toggler = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(MyMoreVideoOptionsLayer::on_show_fps));
			toggler->setPosition({-135, -88});
			toggler->toggle(show_fps);
			m_pButtonMenu->addChild(toggler);

			auto* label = cocos2d::CCLabelBMFont::create("Show FPS", "bigFont.fnt");
			label->setPosition({171.5f, 72});
			label->setScale(0.5f);
			label->setAnchorPoint({0.f, 0.5f});
			m_pLayer->addChild(label);
		}

		auto* unselected_spr = CCSprite::createWithSpriteFrameName("GJ_select_001.png");
		auto* selected_spr = CCSprite::createWithSpriteFrameName("playerSquare_001.png");
		unselected_spr->setScale(0.5f);
		selected_spr->setScale(0.5f);


		const float start_x = 32;
		const float start_y = -75;
		{
			auto* toggler = CCMenuItemToggler::create(unselected_spr, selected_spr, this, menu_selector(MyMoreVideoOptionsLayer::on_select_corner));
			toggler->setPosition({start_x, start_y});
			toggler->setTag(44400);
			m_pButtonMenu->addChild(toggler);
		}
		{
			auto* toggler = CCMenuItemToggler::create(unselected_spr, selected_spr, this, menu_selector(MyMoreVideoOptionsLayer::on_select_corner));
			toggler->setPosition({start_x + 50, start_y});
			toggler->toggle(true);
			toggler->setTag(44401);
			m_pButtonMenu->addChild(toggler);
		}
		{
			auto* toggler = CCMenuItemToggler::create(unselected_spr, selected_spr, this, menu_selector(MyMoreVideoOptionsLayer::on_select_corner));
			toggler->setPosition({start_x, start_y - 35});
			toggler->setTag(44402);
			m_pButtonMenu->addChild(toggler);
		}
		{
			auto* toggler = CCMenuItemToggler::create(unselected_spr, selected_spr, this, menu_selector(MyMoreVideoOptionsLayer::on_select_corner));
			toggler->setPosition({start_x + 50, start_y - 35});
			toggler->setTag(44403);
			m_pButtonMenu->addChild(toggler);
		}
		select_corner(fps_corner);

		return true;
	}

	void select_corner(const Corner corner) {
		// the wonders of having to use tags
		for (int i = 0; i <= 3; ++i) {
			auto* toggler = static_cast<CCMenuItemToggler*>(m_pButtonMenu->getChildByTag(44400 + i));
			if (toggler) {
				toggler->toggle(false);
			} else {
				return;
			}
		}
		static_cast<CCMenuItemToggler*>(m_pButtonMenu->getChildByTag(44400 + static_cast<int>(corner)))->toggle(true);
	}

	void on_select_corner(CCObject* obj) {
		for (int i = 0; i <= 3; ++i) {
			auto* toggler = static_cast<CCMenuItemToggler*>(m_pButtonMenu->getChildByTag(44400 + i));
			if (toggler) {
				toggler->toggle(false);
			}
		}
		fps_corner = static_cast<Corner>(obj->getTag() - 44400);
	}

	void on_show_fps(CCObject* obj) {
		auto* toggler = static_cast<CCMenuItemToggler*>(obj);
		show_fps = !toggler->isOn();
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

cocos2d::CCLabelBMFont* fps_label = nullptr;

bool PlayLayer_init(PlayLayer* self, GJGameLevel* lvl) {
	if (!matdash::orig<&PlayLayer_init>(self, lvl)) return false;

	if (show_fps) {
		const auto win_size = cocos2d::CCDirector::sharedDirector()->getWinSize();

		auto label = cocos2d::CCLabelBMFont::create("some fps", "bigFont.fnt");
		label->setOpacity(100);
		label->setScale(0.4f);
		label->setPosition({100, 100});
		const float pad = 5.f;
		switch (fps_corner) {
			case Corner::TopLeft:
				label->setAnchorPoint({0.f, 1.f});
				label->setPosition({pad, win_size.height - pad + 4.f});
				break;
			case Corner::TopRight:
				label->setAnchorPoint({1.f, 1.f});
				label->setPosition({win_size.width - pad, win_size.height - pad + 4.f});
				break;
			case Corner::BotLeft:
				label->setAnchorPoint({0.f, 0.f});
				label->setPosition({pad, pad});
				break;
			case Corner::BotRight:
				label->setAnchorPoint({1.f, 0.f});
				label->setPosition({win_size.width - pad, pad});
				break;
		}

		label->setZOrder(99);
		self->addChild(label);

		fps_label = label;
		self->addChild(OnLeaveNode::create([] {
			fps_label = nullptr;
		}));
	}
	

	return true;
}

std::chrono::time_point<std::chrono::high_resolution_clock> previous_frame, last_update;
float avg = 0.f;
int frame_count = 0;

matdash::cc::thiscall<void> PlayLayer_update(PlayLayer* self, float dt) {
	matdash::orig<&PlayLayer_update>(self, dt);

	const auto now = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<float> diff = now - previous_frame;
	avg += diff.count();
	frame_count++;

	if (fps_label && std::chrono::duration<float>(now - last_update).count() > 1.0f) {
		last_update = now;
		const auto fps = static_cast<int>(std::roundf(static_cast<float>(frame_count) / avg));
		avg = 0.f;
		frame_count = 0;

		std::stringstream stream;
		stream << fps << " rFPS";
		fps_label->setString(stream.str().c_str());
	}

	previous_frame = now;

	return {};
}

void mod_main(HMODULE) {
	auto cocos_handle = LoadLibraryA("libcocos2d.dll");

	matdash::add_hook<&CCDirector_drawScene>(GetProcAddress(cocos_handle, "?drawScene@CCDirector@cocos2d@@QAEXXZ"));

	matdash::add_hook<&MyMoreVideoOptionsLayer::init_>(gd::base + 0x1E2590);

	matdash::add_hook<&PlayLayer_init>(base + 0x1FB780);
	matdash::add_hook<&PlayLayer_update>(base + 0x2029C0);

	load_gv_values();

	if (InitialiseHackpro() && HackproIsReady()) {
		auto ext = HackproInitialiseExt("Draw Divide");

		mh_fps_checkbox = HackproAddCheckbox(ext, "Show FPS", on_fps_checkbox_enable, on_fps_checkbox_disable);
		HackproSetCheckbox(mh_fps_checkbox, show_fps);

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
