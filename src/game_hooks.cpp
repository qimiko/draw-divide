#include "game_hooks.hpp"

#include <hackpro_ext.h>

int FRAME_COUNTER = 0;
int ENABLE_HACK = false;
int SETUP_HACKPRO = false;

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

void(__thiscall *CCDirector_drawScene_O)(cocos2d::CCDirector*);
void __fastcall CCDirector_drawScene_H(cocos2d::CCDirector* self) {
	if (!ENABLE_HACK) {
		return CCDirector_drawScene_O(self);
	}

	// scary floats
	// getAnimationInterval is 1/fps bypass
	// 1/((1/fps bypass) * target) = fps bypass/target
	const double thing = 1.f / (self->getAnimationInterval() * static_cast<double>(target_fps));

	FRAME_COUNTER++;

	// run full scene draw (glClear, visit) each time the counter is full
	if (static_cast<double>(FRAME_COUNTER) + frame_remainder >= thing) {
		frame_remainder += static_cast<double>(FRAME_COUNTER) - thing;
		FRAME_COUNTER = 0;
		return CCDirector_drawScene_O(self);
	}

	// otherwise, we only run updates

	// upcast to remove protection
	auto visible_director = static_cast<CCDirectorVisible*>(self);

//	this line seems to create a speedhack
//	visible_director->calculateDeltaTime();

	if (!self->isPaused()) {
		self->getScheduler()->update(
			self->getDeltaTime()
		);
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

	// HackproSetTextBoxText(tb, std::to_string(DRAW_DIVIDE).c_str());
}

void __stdcall on_frame_input(void* tb) {
	HackproSetTextBoxText(tb, std::to_string(FRAME_COUNTER).c_str());
}

void __stdcall on_checkbox_enable(void* cb) {
	ENABLE_HACK = true;
}

void __stdcall on_checkbox_disable(void* cb) {
	ENABLE_HACK = false;
}

void setup_hackpro() {
	if (!SETUP_HACKPRO) {
		if (InitialiseHackpro()) {
			if (HackproIsReady()) {
				auto ext = HackproInitialiseExt("Draw Divide");

				auto tb = HackproAddTextBox(ext, on_text_input);
				HackproSetTextBoxPlaceholder(tb, "Factor");
				HackproSetTextBoxText(tb, std::to_string(target_fps).c_str());

				auto cb = HackproAddCheckbox(ext, "Enable", on_checkbox_enable, on_checkbox_disable);
				HackproSetCheckbox(cb, ENABLE_HACK);

				HackproCommitExt(ext);
			}
		}

		SETUP_HACKPRO = true;
	}
}

void do_hooks() {
	setup_hackpro();

	if (MH_Initialize() != MH_OK) {
		return;
	}

	auto cocos_handle = LoadLibraryA("libcocos2d.dll");

	auto drawscene_addr = reinterpret_cast<void*>(
		GetProcAddress(cocos_handle, "?drawScene@CCDirector@cocos2d@@QAEXXZ")
	);

	// CCDirector::drawScene
	MH_CreateHook(
		drawscene_addr,
		reinterpret_cast<void*>(&CCDirector_drawScene_H),
		reinterpret_cast<void**>(&CCDirector_drawScene_O)
	);

	MH_EnableHook(MH_ALL_HOOKS);
}
