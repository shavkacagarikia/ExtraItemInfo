#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/PapyrusScaleformAdapter.h"

#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusUtilities.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameMenus.h"
#include "f4se/GameInput.h"
#include "f4se/InputMap.h"
#include "f4se/GameReferences.h"
#include "Scaleform.h"

bool IsCapacityVisible = true;
bool IsAPCostVisible = true;
bool IsWeapVWVisible = true;
bool IsArmoVWVisible = true;
bool IsAlchVWVisible = true;
bool IsMiscVWVisible = true;
bool IsAmmoVWVisible = true;


bool to_bool(std::string const& s) {
	return s != "0";
}

std::string EIIGetConfigOption(const char * section, const char * key)
{
	std::string	result;

	const std::string & configPath = "./Data/MCM/Settings/ExtraItemInfo.ini";
	if (!configPath.empty())
	{
		char	resultBuf[256];
		resultBuf[0] = 0;

		UInt32	resultLen = GetPrivateProfileString(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());

		result = resultBuf;
	}

	return result;
}

class Scaleform_getBP : public GFxFunctionHandler
{
public:
	virtual void    Invoke(Args * args) {

		UInt32 posX = (*g_player)->pos.x;
		UInt32 posY = (*g_player)->pos.y;
		std::string xStr = std::to_string(posX);
		std::string yStr = std::to_string(posY);

		std::string finStr = "X: " + xStr + " Y: " + yStr;
		args->movie->movieRoot->CreateString(args->result, finStr.c_str());

		bool val = to_bool(EIIGetConfigOption("Main", "bShowCoordinates"));
		GFxValue arrArgs[1];
		arrArgs[0].SetBool(val);
		bool a = arrArgs[0].GetUInt();
		args->movie->movieRoot->Invoke("root.dt_pipboy_bottom_bar_widget_loader.content.SetXYVisibility", nullptr, arrArgs, 1);
		/*auto inventory = (*g_player)->inventoryList;
		if (inventory)
		{
			inventory->inventoryLock.Lock();
			for (UInt32 i = 0; i < inventory->items.count; i++)
			{
				SInt32 s = 0;
				inventory->items[i].stack->Visit([&](BGSInventoryItem::Stack * stack)
				{
					if (inventory->items[i].form->formID == 0x00075FE4) {
						SInt32 count = inventory->items[i].stack->count;
						args->result->SetInt(count);
						return true;
					}
					s++;
				});
			}
			inventory->inventoryLock.Release();
		}*/

	}
};




void SetVisibility(bool value) {
	IMenu * pHUD = nullptr;
	static BSFixedString menuName("PipboyMenu");
	if ((*g_ui) != nullptr && (pHUD = (*g_ui)->GetMenu(menuName), pHUD))
	{
		GFxMovieRoot * movieRoot = pHUD->movie->movieRoot;
		if (movieRoot != nullptr) {
			GFxValue arrArgs[1];
			arrArgs[0].SetBool(value);
			bool a = arrArgs[0].GetUInt();
			movieRoot->Invoke("root.dt_pipboy_bottom_bar_widget_loader.content.SetXYVisibility", nullptr, arrArgs, 2);

		}
	}
}

void LoadSettings() {
	IsCapacityVisible = to_bool(EIIGetConfigOption("Main", "bShowCapacity"));
	IsAPCostVisible = to_bool(EIIGetConfigOption("Main", "bShowAPCost"));
	IsWeapVWVisible = to_bool(EIIGetConfigOption("Main", "bShowWeapVW"));
	IsArmoVWVisible = to_bool(EIIGetConfigOption("Main", "bShowArmoVW"));
	IsAlchVWVisible = to_bool(EIIGetConfigOption("Main", "bShowAlchVW"));
	IsMiscVWVisible = to_bool(EIIGetConfigOption("Main", "bShowMiscVW"));
	IsAmmoVWVisible = to_bool(EIIGetConfigOption("Main", "bShowAmmoVW"));
}

class EII_OnModSettingChanged : public GFxFunctionHandler
{
public:


	virtual void	Invoke(Args * args) {
		std::string settingId = args->args[0].GetString();
		bool result = true;
		if (settingId.find("bShowCapacity") != std::string::npos)
		{
			IsCapacityVisible = to_bool(EIIGetConfigOption("Main", "bShowCapacity"));
		}
		else if (settingId.find("bShowAPCost") != std::string::npos)
		{
			IsAPCostVisible = to_bool(EIIGetConfigOption("Main", "bShowAPCost"));
		}
		else if (settingId.find("bShowWeapVW") != std::string::npos)
		{
			IsWeapVWVisible = to_bool(EIIGetConfigOption("Main", "bShowWeapVW"));
		}
		else if (settingId.find("bShowArmoVW") != std::string::npos)
		{
			IsArmoVWVisible = to_bool(EIIGetConfigOption("Main", "bShowArmoVW"));
		}
		else if (settingId.find( "bShowAlchVW") != std::string::npos)
		{
			IsAlchVWVisible = to_bool(EIIGetConfigOption("Main", "bShowAlchVW"));
		}
		else if (settingId.find("bShowMiscVW") != std::string::npos)
		{
			IsMiscVWVisible = to_bool(EIIGetConfigOption("Main", "bShowMiscVW"));
		}
		else if (settingId.find("bShowAmmoVW") != std::string::npos)
		{
			IsAmmoVWVisible = to_bool(EIIGetConfigOption("Main", "bShowAmmoVW"));
		}
		args->result->SetBool(result);
	}
};


bool RegisterScaleform(GFxMovieView * view, GFxValue * f4se_root)
{
	RegisterFunction<EII_OnModSettingChanged>(f4se_root, view->movieRoot, "onModSettingChanged");

	GFxMovieRoot* movieRoot = view->movieRoot;

	GFxValue currentSWFPathPip;
	std::string currentSWFPathStringPip = "";
	if (movieRoot->GetVariable(&currentSWFPathPip, "root.loaderInfo.url")) {
		currentSWFPathStringPip = currentSWFPathPip.GetString();
		//_MESSAGE("hooking %s", currentSWFPathString.c_str());
		if (currentSWFPathStringPip.find("PipboyMenu.swf") != std::string::npos)
		{
			if (!movieRoot)
				return false;
			GFxValue loaderPip, urlRequestPip, rootPip;
			movieRoot->GetVariable(&rootPip, "root");
			movieRoot->CreateObject(&loaderPip, "flash.display.Loader");
			movieRoot->CreateObject(&urlRequestPip, "flash.net.URLRequest", &GFxValue("ExtraItemInfo.swf"), 1);
			rootPip.SetMember("dt_pipboy_bottom_bar_widget_loader", &loaderPip);
			GFxValue codeObj;
			movieRoot->GetVariable(&codeObj, "root.Menu_mc.BGSCodeObj");
			if (!codeObj.IsUndefined()) {
				RegisterFunction<Scaleform_getBP>(&codeObj, movieRoot, "getBPs");
			}
			bool pip = movieRoot->Invoke("root.dt_pipboy_bottom_bar_widget_loader.load", nullptr, &urlRequestPip, 1);
			bool pip2 = movieRoot->Invoke("root.Menu_mc.BottomBar_mc.Info_mc.addChild", nullptr, &loaderPip, 1);
			if (!pip || !pip2) {
				_MESSAGE("WARNING: injection failed.");
			}
		}

	}


	return true;
}