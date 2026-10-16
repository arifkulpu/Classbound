#pragma once
#include <algorithm>

namespace Classbound
{
	class UIManager
	{
	public:
		static UIManager* GetSingleton()
		{
			static UIManager singleton;
			return &singleton;
		}

		void Render();
		void ToggleMenu();
		bool IsMenuOpen() const { return _isOpen; }

	private:
		UIManager() = default;
		~UIManager() = default;

		void RenderCharacterClass();
		void RenderClassStatistics();
		void RenderFollowers();
		void RenderBonusOverview();
		void RenderChangeClass();
		void RenderSettings();

		bool _isOpen{ false };
		int  _selectedPreviewClass{ 0 };
		bool _showConfirmDialog{ false };
	};
}
