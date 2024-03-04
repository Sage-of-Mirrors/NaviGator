#include "util/rdr1util.hpp"

#include <bstream.h>
#include <pugixml.hpp>

#include <sstream>
#include <fstream>

void RDR1Util::ExtractTrainPoints(std::filesystem::path wsiPath) {
	bStream::CFileStream stream(wsiPath.generic_string(), bStream::Little, bStream::In);

	pugi::xml_document doc;
	doc.document_element().append_attribute("encoding").set_value("UTF-8");

	pugi::xml_node rootNode = doc.append_child("train_tracks");
	rootNode.append_attribute("version").set_value("1");

	stream.seek(0x7690);

	for (uint32_t i = 0; i < 0x15; i++) {
		stream.skip(4);
		uint32_t pointArrayOffset = stream.readUInt32() & 0x0FFFFFFF;

		stream.skip(4);
		uint32_t indexListingOffset = stream.readUInt32() & 0x0FFFFFFF;

		stream.skip(0x0C);
		uint16_t pointCount = stream.readUInt16() + 1;

		stream.skip(0x22);
		uint32_t nameOffset = stream.readUInt32() & 0x0FFFFFFF;

		stream.skip(0x0C);
		size_t nextEntryPos = stream.tell();

		if (pointCount > 1) {
			stream.seek(nameOffset);
			std::string name = stream.readString(0);

			std::stringstream outData;
			outData << pointCount << " " << "0 " << "open" << std::endl;

			stream.seek(pointArrayOffset);
			for (uint32_t pointIdx = 0; pointIdx < pointCount; pointIdx++) {
				float x, y, z;
				x = stream.readFloat() - 1536.0f;
				y = stream.readFloat() - 31.0f;
				z = stream.readFloat();

				outData << x << " " << -z << " " << y << " 0 0" << std::endl;
				stream.skip(4);
			}

			std::filesystem::path outPath = wsiPath.parent_path() / name;
			outPath.replace_extension(".dat");
			std::ofstream writer(outPath.c_str());
			writer << outData.str();
			writer.close();

			pugi::xml_node trackNode = rootNode.append_child("train_track");
			trackNode.append_attribute("filename").set_value(outPath.generic_string().data());
			trackNode.append_attribute("trainConfigName").set_value(name.data());
			trackNode.append_attribute("stopsAtStations").set_value(true);
			trackNode.append_attribute("brakingDist").set_value(10);
		}

		stream.seek(nextEntryPos);
	}

	std::filesystem::path xmlPath = wsiPath.parent_path() / "traintracks.xml";
	doc.save_file(xmlPath.c_str(), PUGIXML_TEXT("\t"), pugi::format_indent | pugi::format_indent_attributes | pugi::format_save_file_text, pugi::encoding_utf8);
}
