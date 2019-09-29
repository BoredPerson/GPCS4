#include "Module.h"
#include "Platform/PlatformUtils.h"

#include <algorithm>

MODULE_INFO &MemoryMappedModule::getModuleInfo()
{
	return moduleInfo;
}

bool MemoryMappedModule::getImportSymbolInfo(std::string const &encSymbol,
											 std::string *modName,
											 std::string * libName,
											 uint64_t * nid) const
{
	return getSymbolInfo(encSymbol, importModules, importLibraries, modName, libName, nid);
}

bool MemoryMappedModule::getExportSymbolInfo(std::string const &encSymbol,
											 std::string *modName,
											 std::string * libName,
											 uint64_t * nid) const
{
	return getSymbolInfo(encSymbol, exportModules, exportLibraries, modName, libName, nid);
}

bool MemoryMappedModule::getImportSymbol(std::string const & encName, SymbolInfo const **symbolInfo) const
{
	bool retVal = false;
	if (importSymbols.count(encName) == 0)
	{
		retVal = false;
	}
	else
	{
		*symbolInfo = &importSymbols.at(encName);
		retVal = true;
	}

	return retVal;

}

bool MemoryMappedModule::getSymbolInfo(std::string const & encSymbol,
									   std::vector<IMPORT_MODULE> const & mods,
									   std::vector<IMPORT_LIBRARY> const & libs,
									   std::string * modName, std::string * libName, uint64_t * nid) const
{
	bool retVal = false;

	do
	{
		uint modId = 0, libId = 0;

		if (modName == nullptr || libName == nullptr || nid == nullptr)
		{
			break;
		}

		retVal = decodeSymbol(encSymbol, &modId, &libId, nid);
		if (!retVal)
		{
			LOG_ERR("fail to decode encoded symbol");
			break;
		}

		retVal = getModNameFromId(modId, mods, modName);
		if (!retVal)
		{
			LOG_ERR("fail to get module name");
			break;
		}

		retVal = getLibNameFromId(libId, libs, libName);
		if (!retVal)
		{
			LOG_ERR("fail to get library name");
			break;
		}

		retVal = true;
	} while (false);

	return retVal;
}

bool MemoryMappedModule::getModNameFromId(uint64_t id, std::vector<IMPORT_MODULE> const &mods, std::string * modName) const 
{

	bool retVal = false;
	auto &modules = mods;
	do
	{
		if (modName == nullptr)
		{
			break;
		}

		auto iter = std::find_if(modules.begin(), modules.end(),
								 [=](IMPORT_MODULE const &mod) { return id == mod.id; });

		if (iter == modules.end())
		{
			break;
		}
		
		*modName = iter->strName;
		retVal = true;
	} while (false);

	return retVal;
}

bool MemoryMappedModule::getLibNameFromId(uint64_t id, std::vector<IMPORT_LIBRARY> const &libs, std::string *libName) const
{
	bool retVal = false;

	do
	{
		if (libName == nullptr)
		{
			break;
		}

		auto iter = std::find_if(libs.begin(), libs.end(),
								 [=](IMPORT_LIBRARY const &lib) {return id == lib.id; });

		if (iter == libs.end())
		{
			break;
		}
		
		*libName = iter->strName;
		retVal = true;

	} while (false);

	return retVal;
}

bool MemoryMappedModule::decodeValue(std::string const & encodedStr, uint64_t & value) const
{	
	bool bRet = false;

	//the max length for an encode id is 11
	//from orbis-ld.exe
	const uint nEncLenMax = 11;
	const char pCodes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

	do
	{

		if (encodedStr.size() > nEncLenMax)
		{
			LOG_ERR("encode id too long: %s", encodedStr.c_str());
			break;
		}

		bool bError = false;
		value = 0;

		for (int i = 0; i < encodedStr.size(); ++i)
		{
			auto pChPos = strchr(pCodes, encodedStr[i]);
			uint nIndex = 0;

			if (pChPos != nullptr)
			{
				nIndex = static_cast<uint>(pChPos - pCodes);
			}
			else
			{
				bError = true;
				break;
			}

			// NID is 64 bits long, thus we do 6 x 10 + 4 times
			if (i < nEncLenMax - 1)
			{
				value <<= 6;
				value |= nIndex;
			}
			else
			{
				value <<= 4;
				value |= (nIndex >> 2);
			}
		}

		if (bError)
		{
			break;
		}

		bRet = true;
	} while (false);
	return bRet;
}

bool MemoryMappedModule::decodeSymbol(std::string const & strEncName, uint * modId, uint * libId, uint64_t * funcNid) const
{
	bool bRet = false;

	do
	{
		if (modId == nullptr || libId == nullptr || funcNid == nullptr)
		{
			break;
		}

		auto &nModuleId = *modId;
		auto &nLibraryId = *libId;
		auto &nNid = *funcNid;

		std::vector<std::string> vtNameParts;
		if (!UtilString::Split(strEncName, '#', vtNameParts))
		{
			break;
		}

		if (!decodeValue(vtNameParts[0], nNid))
		{
			break;
		}

		uint64 nLibId = 0;
		if (!decodeValue(vtNameParts[1], nLibId))
		{
			break;
		}
		nLibraryId = static_cast<uint>(nLibId);

		uint64 nModId = 0;
		if (!decodeValue(vtNameParts[2], nModId))
		{
			break;
		}
		nModuleId = static_cast<uint>(nModId);

		bRet = true;
	} while (false);

	return bRet;
}
