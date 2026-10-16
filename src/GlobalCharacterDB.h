#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Classbound
{
	struct CharacterSaveEntry
	{
		std::string charName;
		std::string saveFileName;
		uint32_t    classType{ 0 };
		uint32_t    classLevel{ 1 };
		std::string lastUpdate; // ISO date or simple string
	};

	class GlobalCharacterDB
	{
	public:
		static GlobalCharacterDB* GetSingleton()
		{
			static GlobalCharacterDB singleton;
			return &singleton;
		}

		void LoadDB();
		void SaveDB();

		// Update or add an entry for the current character/save
		void UpsertCurrentCharacter(const std::string& saveName, uint32_t classType, uint32_t classLevel);

		// Remove duplicate or invalid stale entries
		void PruneStaleEntries();

		const std::vector<CharacterSaveEntry>& GetEntries() const { return _entries; }

	private:
		GlobalCharacterDB() = default;
		~GlobalCharacterDB() = default;

		std::vector<CharacterSaveEntry> _entries;
		std::string GetDBPath() const;
	};
}
