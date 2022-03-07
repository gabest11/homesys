#pragma once

#include <ShObjIdl.h>

using namespace System;

namespace Homesys
{
    public enum class AssocLevel
    {
        AL_MACHINE,
        AL_EFFECTIVE,
        AL_USER,
    };

    public enum class AssocType
    {
        AT_FILEEXTENSION,
        AT_URLPROTOCOL,
        AT_STARTMENUCLIENT,
        AT_MIMETYPE,
    };

	public ref class AppAssocReg
	{
	public:
		AppAssocReg();
		~AppAssocReg();
		!AppAssocReg();

        String^ QueryCurrentDefault(String^ pszQuery, AssocType atQueryType, AssocLevel alQueryLevel);
        bool QueryAppIsDefault(String^ pszQuery, AssocType atQueryType, AssocLevel alQueryLevel, String^ pszAppRegistryName);
        bool QueryAppIsDefaultAll(AssocLevel alQueryLevel, String^ pszAppRegistryName);
        void SetAppAsDefault(String^ pszAppRegistryName, String^ pszSet, AssocType atSetType);
        void SetAppAsDefaultAll(String^ pszAppRegistryName);
        void ClearUserAssociations();
	};

	public ref class AppAssocRegUI
	{
	public:
		AppAssocRegUI();
		~AppAssocRegUI();
		!AppAssocRegUI();

        void LaunchAdvancedAssociationUI(String^ pszAppRegistryName);
	};
}