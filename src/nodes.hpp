#pragma once
#include <gd.h>
#include <functional>
#include <optional>

#define GEN_CREATE(class_name) template <typename... Args> \
static auto create(Args... args) { \
	auto node = new class_name; \
	if (node && node->init(args...)) \
		node->autorelease(); \
	else \
		CC_SAFE_DELETE(node); \
	return node; \
}

class TextInputNode : public cocos2d::CCNode, public gd::TextInputDelegate {
public:
	gd::CCTextInputNode* input_node;
	cocos2d::extension::CCScale9Sprite* background;
	std::function<void(TextInputNode&)> callback = [](auto&){};

	GEN_CREATE(TextInputNode)

	bool init(cocos2d::CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt");

	virtual void textChanged(gd::CCTextInputNode*);

	void set_value(const std::string& value);
	std::string get_value();
};

class NumberInputNode : public TextInputNode {
public:
	std::function<void(NumberInputNode&)> callback = [](auto&){};

	GEN_CREATE(NumberInputNode)

	bool init(cocos2d::CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt");
	virtual void textChanged(gd::CCTextInputNode*);

	void set_value(int value);
	int get_value();
};

class FloatInputNode : public TextInputNode {
public:
	std::function<void(FloatInputNode&)> callback = [](auto&){};
	
	GEN_CREATE(FloatInputNode)

	bool init(cocos2d::CCSize size, float scale = 1.f, const std::string& font = "bigFont.fnt");
	virtual void textChanged(gd::CCTextInputNode*);

	void set_value(float value);
	std::optional<float> get_value();
};
