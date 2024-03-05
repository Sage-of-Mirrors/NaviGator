#include "util/rdr1util.hpp"

#include <bstream.h>
#include <pugixml.hpp>

#include <sstream>
#include <fstream>

constexpr float ONE_THIRD = 0.33333333333f;

void RDR1Util::ExtractTrainPoints(std::filesystem::path wsiPath) {
	bStream::CFileStream stream(wsiPath.generic_string(), bStream::Little, bStream::In);
	std::vector<RDR1Track> tracks;

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

		RDR1Track track;

		if (pointCount > 1) {
			stream.seek(nameOffset);
			track.name = stream.readString(0);

			stream.seek(pointArrayOffset);
			for (uint32_t pointIdx = 0; pointIdx < pointCount; pointIdx++) {
				track.points.push_back({ stream.readFloat() - 1536.0f, stream.readFloat() - 31.0f, stream.readFloat() });
				stream.skip(4);
			}
		}

		tracks.push_back(track);
		stream.seek(nextEntryPos);
	}

	for (const RDR1Track& track : tracks) {
		std::stringstream outData;
		outData << track.points.size() << " " << track.points.size() << " " << "open" << std::endl;

		for (uint32_t pointIdx = 0; pointIdx < track.points.size(); pointIdx++) {
			glm::vec3 pos, handleA, handleB;
			pos = track.points[pointIdx];

			if (pointIdx == 0) {
				handleA = pos + (pos - track.points[pointIdx + 1]) * ONE_THIRD;
			}
			else {
				handleA = pos - (pos - track.points[pointIdx - 1]) * ONE_THIRD;
			}

			if (pointIdx == track.points.size() - 1) {
				handleB = pos + (pos - track.points[pointIdx - 1]) * ONE_THIRD;
			}
			else {
				handleB = pos - (pos - track.points[pointIdx + 1]) * ONE_THIRD;
			}

			outData << "c ";
			outData << pos.x     << " " << -pos.z     << " " << pos.y     << " ";
			outData << handleA.x << " " << -handleA.z << " " << handleA.y << " ";
			outData << handleB.x << " " << -handleB.z << " " << handleB.y << " ";
			outData << "0 0" << std::endl;
		}

		std::filesystem::path outPath = wsiPath.parent_path() / track.name;
		outPath.replace_extension(".dat");

		std::ofstream writer(outPath.c_str());
		writer << outData.str();
		writer.close();

		pugi::xml_node trackNode = rootNode.append_child("train_track");
		trackNode.append_attribute("filename").set_value(outPath.generic_string().data());
		trackNode.append_attribute("trainConfigName").set_value(track.name.data());
		trackNode.append_attribute("stopsAtStations").set_value(true);
		trackNode.append_attribute("brakingDist").set_value(10);
	}

	std::filesystem::path xmlPath = wsiPath.parent_path() / "traintracks.xml";
	doc.save_file(xmlPath.c_str(), PUGIXML_TEXT("\t"), pugi::format_indent | pugi::format_indent_attributes | pugi::format_save_file_text, pugi::encoding_utf8);
}
