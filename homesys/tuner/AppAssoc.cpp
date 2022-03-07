#include "StdAfx.h"
#include "AppAssoc.h"

extern std::wstring Translate(String^ s);
extern String^ Translate(LPCWSTR s);

namespace Homesys
{
	AppAssocReg::AppAssocReg()
	{
	}

	AppAssocReg::~AppAssocReg()
	{
		this->!AppAssocReg();
	}

	AppAssocReg::!AppAssocReg()
	{
	}

    String^ AppAssocReg::QueryCurrentDefault(String^ pszQuery, AssocType atQueryType, AssocLevel alQueryLevel)
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistration> reg;

		hr = reg.CoCreateInstance(__uuidof(ApplicationAssociationRegistration));

		if(FAILED(hr)) throw hr;

		std::wstring query = Translate(pszQuery);

		LPWSTR buff[256] = {0};

		hr = reg->QueryCurrentDefault(query.c_str(), (ASSOCIATIONTYPE)atQueryType, (ASSOCIATIONLEVEL)alQueryLevel, buff);

		if(FAILED(hr)) throw hr;

		return Translate((LPCWSTR)buff);
	}

    bool AppAssocReg::QueryAppIsDefault(String^ pszQuery, AssocType atQueryType, AssocLevel alQueryLevel, String^ pszAppRegistryName)
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistration> reg;

		hr = reg.CoCreateInstance(__uuidof(ApplicationAssociationRegistration));

		if(FAILED(hr)) throw hr;

		std::wstring query = Translate(pszQuery);
		std::wstring name = Translate(pszAppRegistryName);

		BOOL res = FALSE;

		hr = reg->QueryAppIsDefault(query.c_str(), (ASSOCIATIONTYPE)atQueryType, (ASSOCIATIONLEVEL)alQueryLevel, name.c_str(), &res);

		if(FAILED(hr)) throw hr;

		return !!res;
	}

    bool AppAssocReg::QueryAppIsDefaultAll(AssocLevel alQueryLevel, String^ pszAppRegistryName)
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistration> reg;

		hr = reg.CoCreateInstance(__uuidof(ApplicationAssociationRegistration));

		if(FAILED(hr)) throw hr;

		std::wstring name = Translate(pszAppRegistryName);

		BOOL res = FALSE;

		hr = reg->QueryAppIsDefaultAll((ASSOCIATIONLEVEL)alQueryLevel, name.c_str(), &res);

		if(FAILED(hr)) throw hr;

		return !!res;
	}

    void AppAssocReg::SetAppAsDefault(String^ pszAppRegistryName, String^ pszSet, AssocType atSetType)
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistration> reg;

		hr = reg.CoCreateInstance(__uuidof(ApplicationAssociationRegistration));

		if(FAILED(hr)) throw hr;

		std::wstring name = Translate(pszAppRegistryName);
		std::wstring set = Translate(pszSet);

		hr = reg->SetAppAsDefault(name.c_str(), set.c_str(), (ASSOCIATIONTYPE)atSetType);

		if(FAILED(hr)) throw hr;
	}

    void AppAssocReg::SetAppAsDefaultAll(String^ pszAppRegistryName)
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistration> reg;

		hr = reg.CoCreateInstance(__uuidof(ApplicationAssociationRegistration));

		if(FAILED(hr)) throw hr;

		std::wstring name = Translate(pszAppRegistryName);

		hr = reg->SetAppAsDefaultAll(name.c_str());

		if(FAILED(hr)) throw hr;
	}

    void AppAssocReg::ClearUserAssociations()
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistration> reg;

		hr = reg.CoCreateInstance(__uuidof(ApplicationAssociationRegistration));

		if(FAILED(hr)) throw hr;

		hr = reg->ClearUserAssociations();

		if(FAILED(hr)) throw hr;
	}

	AppAssocRegUI::AppAssocRegUI()
	{
	}

	AppAssocRegUI::~AppAssocRegUI()
	{
		this->!AppAssocRegUI();
	}

	AppAssocRegUI::!AppAssocRegUI()
	{
	}

    void AppAssocRegUI::LaunchAdvancedAssociationUI(String^ pszAppRegistryName)
	{
		HRESULT hr;

		CComPtr<IApplicationAssociationRegistrationUI> regui;

		hr = regui.CoCreateInstance(__uuidof(ApplicationAssociationRegistrationUI));

		if(FAILED(hr)) throw hr;

		std::wstring name = Translate(pszAppRegistryName);

		hr = regui->LaunchAdvancedAssociationUI(name.c_str());

		if(FAILED(hr)) throw hr;
	}
}