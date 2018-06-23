#include "f4se/PluginAPI.h"
#include "f4se/GameAPI.h"
#include "f4se/GameData.h"
#include "f4se/GameReferences.h"
#include "f4se/GameForms.h"
#include "f4se/GameMenus.h"
#include "f4se/GameRTTI.h"
#include "f4se_common\SafeWrite.h"
#include <shlobj.h>
#include <string>
#include "xbyak/xbyak.h"
#include "f4se_common\BranchTrampoline.h"
#include "f4se_common/f4se_version.h"
#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusEvents.h"
#include "f4se\GameExtraData.h"
#include "Globals.h"
#include "Utils.h"
#include "f4se/Translation.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/CustomMenu.h"
#include "f4se\GameTypes.h"
#include "Scaleform.h"
#include "PipboyExtraItemData.h"

std::string mName = "ExtraItemInfo";
IDebugLog	gLog;




PluginHandle			    g_pluginHandle = kPluginHandle_Invalid;
F4SEMessagingInterface		* g_messaging = nullptr;
F4SEPapyrusInterface   *g_papyrus = NULL;

F4SEScaleformInterface		*g_scaleform = NULL;





typedef UInt32(*_GetRandomPercent)(UInt32 maxvalue);
RelocAddr <_GetRandomPercent> GetRandomPercent(0x01B12B20);

typedef UInt32(*_GetRandomPercent2)(UInt32 minvalue, UInt32 maxvalue);
RelocAddr <_GetRandomPercent2> GetRandomPercent2(0x01B12BC0);



void logMessage(std::string aString)
{
	_MESSAGE(("[" + mName + "] " + aString).c_str());
}

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case F4SEMessagingInterface::kMessage_GameDataReady:
	{
		bool isReady = reinterpret_cast<bool>(msg->data);
		if (isReady)
		{
			LoadSettings();
			_MESSAGE("Registered TESContainerChangedEvent");
		}
		break;
	}

	}
}

extern "C"
{

	bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info)
	{
		// Check game version
		if (f4se->runtimeVersion != RUNTIME_VERSION_1_10_89) {
			_WARNING("WARNING: Unsupported runtime version %08X. This DLL is built for v1.10.89 only.", f4se->runtimeVersion);
			MessageBox(NULL, (LPCSTR)("Unsupported runtime version (expected v1.10.89). \n" + mName + " will be disabled.").c_str(), (LPCSTR)mName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		gLog.OpenRelative(CSIDL_MYDOCUMENTS, (const char*)("\\My Games\\Fallout4\\F4SE\\" + mName + ".log").c_str());
		logMessage("v1.0");
		logMessage("query");
		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = mName.c_str();
		info->version = 1;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = f4se->GetPluginHandle();

		g_scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
		if (!g_scaleform) {
			_MESSAGE("couldn't get scaleform interface");
			return false;
		}

		g_messaging = (F4SEMessagingInterface *)f4se->QueryInterface(kInterface_Messaging);
		if (!g_messaging)
		{
			_FATALERROR("couldn't get messaging interface");
			return false;
		}

		return true;
	}

	bool F4SEPlugin_Load(const F4SEInterface *f4se)
	{
		if (g_scaleform)
		{
			g_scaleform->Register(mName.c_str(), RegisterScaleform);
			logMessage("Scaleform Register Succeeded");
		}
		if (g_messaging)
			g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);
		PipboyMenu::InitHooks();
		logMessage("load");
		return true;

	}

};
