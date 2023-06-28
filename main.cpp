#include "../skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"

#include <ShlObj.h>

#include <fstream>

#include "UltimateCombat.h"
#include "UCHooks.h"
#include "UCPapyrus.h"
#include "GSSPapyrus.h"
#include "GameData.h"
#include <direct.h>

IDebugLog gLog;

PluginHandle g_pluginHandle = kPluginHandle_Invalid;

// Interfaces
SKSEPapyrusInterface* g_papyrus = nullptr;
SKSEMessagingInterface* g_messaging = nullptr;


void SKSEMessageHandler(SKSEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case SKSEMessagingInterface::kMessage_DataLoaded:
		if (DataHandler::GetSingleton()->LookupModByName(UC_ESP_NAME) && DataHandler::GetSingleton()->LookupModByName(UC_ESP_NAME)->IsActive())
		{
			_MESSAGE("[STARTUP] UC ESP %s found, loading UC hooks", UC_ESP_NAME);
			if (!InitEventHooks())
				return;
			UCPapyrusRegister((*g_skyrimVM)->GetClassRegistry());
		}
		if (DataHandler::GetSingleton()->LookupModByName(GSS_ESP_NAME) && DataHandler::GetSingleton()->LookupModByName(GSS_ESP_NAME)->IsActive())
		{
			_MESSAGE("[STARTUP] GSS ESP %s found, loading GSS hooks", GSS_ESP_NAME);
			GSSPapyrusRegister((*g_skyrimVM)->GetClassRegistry());
		}

		_MESSAGE("[STARTUP] Loaded succesfully, have fun :)");

		break;
	}
}

extern "C" {
bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, R"(\My Games\Skyrim Special Edition\SKSE\UltimateCombat.log)");
	gLog.SetPrintLevel(IDebugLog::kLevel_Error);
	gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	_MESSAGE("[STARTUP] Ultimate Combat 3 - 64 bit");

	// populate info structure
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "UltimateCombat64";
	info->version = 1;

	g_pluginHandle = skse->GetPluginHandle();

	if (skse->isEditor)
	{
		_FATALERROR("[ERROR] loaded in editor, marking as incompatible");
		return false;
	}
	if (skse->runtimeVersion != RUNTIME_VERSION_1_5_73)
	{
		_FATALERROR("[ERROR] unsupported runtime version %08X", skse->runtimeVersion);
		return false;
	}

	g_papyrus = static_cast<SKSEPapyrusInterface *>(skse->QueryInterface(kInterface_Papyrus));
	if (!g_papyrus)
	{
		_FATALERROR("[ERROR] couldn't get papyrus interface");
		return false;
	}

	g_messaging = static_cast<SKSEMessagingInterface *>(skse->QueryInterface((kInterface_Messaging)));
	if (!g_messaging)
	{
		_FATALERROR("[ERROR] couldn't get messaging interface");
		return false;
	}

	return true;
}

bool SKSEPlugin_Load(const SKSEInterface* skse)
{
	g_messaging->RegisterListener(g_pluginHandle, "SKSE", SKSEMessageHandler);
	return true;
}
}
