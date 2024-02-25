#include "util/uiutil.hpp"

int TextInputCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		std::string* str;

		str = static_cast<std::string*>(data->UserData);
		str->resize(static_cast<size_t>(data->BufTextLen));
		data->Buf = str->data();
	}

	return 0;
}

bool UIUtil::RenderTextInput(std::string name, std::string* value, const int width)
{
	ImGui::PushItemWidth(width);

	bool result = ImGui::InputText(name.c_str(), value->data(), value->size() + 1, ImGuiInputTextFlags_CallbackResize, TextInputCallback, value);

	ImGui::PopItemWidth();

	return result;
}
