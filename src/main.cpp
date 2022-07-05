#include <Windows.h>
#include <hackpro_ext.h>
#define CC_DLL
#include <cocos2d.h>
#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>

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

void mod_main(HMODULE) {
	auto cocos_handle = LoadLibraryA("libcocos2d.dll");

	matdash::add_hook<&CCDirector_drawScene>(GetProcAddress(cocos_handle, "?drawScene@CCDirector@cocos2d@@QAEXXZ"));

	if (InitialiseHackpro() && HackproIsReady()) {
		auto ext = HackproInitialiseExt("Draw Divide");

		auto tb = HackproAddTextBox(ext, on_text_input);
		HackproSetTextBoxPlaceholder(tb, "FPS");
		HackproSetTextBoxText(tb, std::to_string(target_fps).c_str());

		auto cb = HackproAddCheckbox(ext, "Enable", on_checkbox_enable, on_checkbox_disable);
		HackproSetCheckbox(cb, enabled);

		HackproCommitExt(ext);
	}
}
