#pragma once
#include "ClassData.h"
#include <unordered_map>
#include <optional>

namespace Classbound
{
	class ClassRegistry
	{
	public:
		static ClassRegistry* GetSingleton()
		{
			static ClassRegistry singleton;
			return &singleton;
		}

		void Initialize();
		void RegisterClass(const ClassDefinition& def);
		std::optional<ClassDefinition> GetClass(ClassType type) const;
		const std::unordered_map<ClassType, ClassDefinition>& GetAllClasses() const;

	private:
		ClassRegistry() = default;
		~ClassRegistry() = default;

		std::unordered_map<ClassType, ClassDefinition> _classes;
	};
}
