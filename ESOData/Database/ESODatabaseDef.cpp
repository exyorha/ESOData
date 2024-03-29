#include <ESOData/Database/ESODatabaseDef.h>
#include <ESOData/Database/ESODatabaseParsingContext.h>
#include <ESOData/Database/DatabaseAddressing.h>
#include <ESOData/Database/DefFile.h>

#include <ESOData/Filesystem/Filesystem.h>
#include <ESOData/Serialization/InputSerializationStream.h>
#include <ESOData/Serialization/DeflatedSegment.h>

#include <sstream>

namespace esodata {

	ESODatabaseDef::ESODatabaseDef(const esodata::Filesystem* fs, const DatabaseDirectiveFile::Structure& def, const ESODatabaseParsingContext& parsingContext) :
		m_id(def.defIndex),
		m_name(def.name),
		m_fs(fs),
		m_def(&def),
		m_parsingContext(&parsingContext) {

	}

	ESODatabaseDef::~ESODatabaseDef() = default;

	ESODatabaseDef::ESODatabaseDef(ESODatabaseDef&& other) = default;

	ESODatabaseDef& ESODatabaseDef::operator =(ESODatabaseDef&& other) = default;

	void ESODatabaseDef::loadDef() {
		auto defData = m_fs->readFileByKey(getDefFileId(m_id));

		DefFileHeader header;
		size_t offset = 0;
		header.readFromData(defData, offset);

		if (header.flags != 0x13)
			throw std::logic_error("flag value of 0x13 is expected");

		if (m_def->version != 0 && m_def->version != header.version) {
			std::stringstream error;
			error << "Def file " << m_name << " (" << m_id << ") has unsupported version " << header.version << " (expected " << m_def->version << ").";
			throw std::runtime_error(error.str());
		}

		m_records.resize(header.itemCount);
		m_recordLookup.reserve(header.itemCount);

		const auto& baseDef = m_parsingContext->findStructureByName("BaseDef");

		for (auto& record : m_records) {
			record.addField("flags").emplace<unsigned long long>(header.flags);
			record.addField("version").emplace<unsigned long long>(header.version);

			DefFileRow row;
			row.readFromData(defData, offset);

			esodata::InputSerializationStream contentStream(row.recordData.data(), row.recordData.data() + row.recordData.size());
			contentStream.setSwapEndian(true);

			parseStructureIntoRecord(contentStream, baseDef, record);
			parseStructureIntoRecord(contentStream, *m_def, record);

			auto id = std::get<unsigned long long>(record.findField("id"));
			m_recordLookup.emplace(id, &record);
		}
	}

	void ESODatabaseDef::parseField(esodata::SerializationStream& stream, DatabaseDirectiveFile::FieldType type, ESODatabaseRecord::Value& value, const DatabaseDirectiveFile::StructureField& field) {
		switch (type) {
		case DatabaseDirectiveFile::FieldType::Int8:
		{
			int8_t val;
			stream >> val;
			value.emplace<long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::Int16:
		{
			int16_t val;
			stream >> val;
			value.emplace<long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::Int32:
		{
			int32_t val;
			stream >> val;
			value.emplace<long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::Int64:
		{
			int64_t val;
			stream >> val;
			value.emplace<long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::UInt8:
		{
			uint8_t val;
			stream >> val;
			value.emplace<unsigned long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::UInt16:
		{
			uint16_t val;
			stream >> val;
			value.emplace<unsigned long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::UInt32:
		{
			uint32_t val;
			stream >> val;
			value.emplace<unsigned long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::UInt64:
		{
			uint64_t val;
			stream >> val;
			value.emplace<unsigned long long>(val);
			break;
		}

		case DatabaseDirectiveFile::FieldType::Float:
		{
			float val;
			stream >> val;
			value.emplace<double>(val);
			break;
		}


		case DatabaseDirectiveFile::FieldType::Enum:
		{
			auto& evalue = value.emplace<ESODatabaseRecord::ValueEnum>();
			evalue.definition = &m_parsingContext->findEnumByName(field.typeName);
			stream >> evalue.value;
			break;
		}

		case DatabaseDirectiveFile::FieldType::String:
		{
			std::string svalue;
			stream >> svalue;
			value.emplace<std::string>(std::move(svalue));
			break;
		}

		case DatabaseDirectiveFile::FieldType::Array:
		{
			auto& avalue = value.emplace<ESODatabaseRecord::ValueArray>();
			uint32_t length;
			stream >> length;

			avalue.values.resize(length);

			for (auto& value : avalue.values) {
				parseField(stream, field.arrayType, value, field);
			}
			break;
		}

		case DatabaseDirectiveFile::FieldType::ForeignKey:
		{
			auto& fvalue = value.emplace<ESODatabaseRecord::ValueForeignKey>();
			stream >> fvalue.id;

			fvalue.def = m_parsingContext->findDefByName(field.typeName).name;
			break;
		}

		case DatabaseDirectiveFile::FieldType::Boolean:
			stream >> value.emplace<bool>();
			break;

		case DatabaseDirectiveFile::FieldType::AssetReference:
			stream >> value.emplace<ESODatabaseRecord::ValueAssetReference>().id;
			break;

		case DatabaseDirectiveFile::FieldType::Struct:
		{
			auto& svalue = value.emplace<ESODatabaseRecord::ValueStruct>();

			parseStructureIntoRecord(stream, m_parsingContext->findStructureByName(field.typeName), svalue);

			break;
		}

		case DatabaseDirectiveFile::FieldType::PolymorphicReference:
		{
			auto& pvalue = value.emplace<ESODatabaseRecord::ValuePolymorphicReference>();
			pvalue.selector.definition = &m_parsingContext->findEnumByName(field.typeName);
			stream >> pvalue.selector.value;

			auto it = pvalue.selector.definition->valueNames.find(pvalue.selector.value);
			if (it == pvalue.selector.definition->valueNames.end()) {
				stream >> pvalue.data.emplace<uint32_t>();
			}
			else {
				auto delimiter = it->second.find_first_of('$');
				std::string defName;
				if (delimiter == std::string::npos) {
					defName = it->second;
				}
				else {
					defName = it->second.substr(0, delimiter);
				}

				if (defName == "NULL") {
					uint32_t id;
					stream >> id;

					if (id == 0) {
						pvalue.data.emplace<std::monostate>();
					}
					else {
						pvalue.data.emplace<std::uint32_t>(id);
					}
				}
				else {

					auto& fkey = pvalue.data.emplace<ESODatabaseRecord::ValueForeignKey>();
					fkey.def = m_parsingContext->findDefByName(defName).name;
					stream >> fkey.id;
				}
			}

			break;
		}

		}
	}

	void ESODatabaseDef::parseStructureIntoRecord(esodata::SerializationStream& stream, const DatabaseDirectiveFile::Structure& structure, ESOFieldContainer& record) {
		for (const auto& field : structure.fields) {
			parseField(stream, field.type, record.addField(field.name), field);
		}
	}

	const ESODatabaseRecord* ESODatabaseDef::findRecordById(uint64_t id) const {
		auto it = m_recordLookup.find(id);
		if (it == m_recordLookup.end()) {
			return nullptr;
		}

		return it->second;
	}
}
