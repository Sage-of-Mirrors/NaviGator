#include "application/AOptions.hpp"

#include <pugixml.hpp>

constexpr const char* OPTIONS_FILE_NAME = "navigator.xml";

AOptions OPTIONS;

AOptions::AOptions() : mLastOpenedDir(""), mLastOpenedRailroadDir(""), mLastSavedRailroadDir("") {

}

void AOptions::Load() {
	std::filesystem::path optionsPath = std::filesystem::current_path() / OPTIONS_FILE_NAME;
	if (!std::filesystem::exists(optionsPath)) {
		Save();
		return;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(optionsPath.c_str());

	if (!result) {
		return;
	}

	pugi::xml_node rootNode = doc.child("navigatorOptions");
	OPTIONS.mLastOpenedDir = rootNode.child("lastOpenedDir").text().as_string();
	OPTIONS.mLastOpenedRailroadDir = rootNode.child("lastOpenedRailroadDir").text().as_string();
	OPTIONS.mLastSavedRailroadDir  = rootNode.child("lastSavedRailroadDir").text().as_string();
}

void AOptions::Save() {
	std::filesystem::path optionsPath = std::filesystem::current_path() / OPTIONS_FILE_NAME;

	pugi::xml_document doc;
	doc.document_element().append_attribute("encoding").set_value("UTF-8");

	pugi::xml_node rootNode = doc.append_child("navigatorOptions");
	rootNode.append_child("lastOpenedDir").text().set(OPTIONS.mLastOpenedDir.u8string().data());
	rootNode.append_child("lastOpenedRailroadDir").text().set(OPTIONS.mLastOpenedRailroadDir.u8string().data());
	rootNode.append_child("lastSavedRailroadDir").text().set(OPTIONS.mLastSavedRailroadDir.u8string().data());

	doc.save_file(optionsPath.c_str(), PUGIXML_TEXT("\t"), pugi::format_indent | pugi::format_indent_attributes | pugi::format_save_file_text, pugi::encoding_utf8);
}
