#include <ESOData/Database/ESODatabaseParsingContext.h>

namespace esodata {
	void ESODatabaseParsingContext::buildLookupCaches() {
		m_structureLookup.clear();
		m_defLookup.clear();
		m_enumLookup.clear();

		for (const auto& structure : structures) {
			m_structureLookup.emplace(structure.name, &structure);
		}

		for (const auto& structure : defs) {
			m_defLookup.emplace(structure.name, &structure);
		}

		for (const auto& enumDef : enums) {
			m_enumLookup.emplace(enumDef.name, &enumDef);
		}
	}

	const DatabaseDirectiveFile::Structure& ESODatabaseParsingContext::findStructureByName(const std::string& name) const {
		auto it = m_structureLookup.find(name);
		if (it == m_structureLookup.end()) {
			throw std::logic_error("Structure is not defined: " + name);
		}

		return *it->second;
	}

	const DatabaseDirectiveFile::Structure& ESODatabaseParsingContext::findDefByName(const std::string& name) const {
		auto it = m_defLookup.find(name);
		if (it == m_defLookup.end()) {
			throw std::logic_error("Def is not defined: " + name);
		}

		return *it->second;
	}

	const DatabaseDirectiveFile::Enum& ESODatabaseParsingContext::findEnumByName(const std::string& name) const {
		auto it = m_enumLookup.find(name);
		if (it == m_enumLookup.end()) {
			throw std::logic_error("Enum is not defined: " + name);
		}

		return *it->second;
	}
}
