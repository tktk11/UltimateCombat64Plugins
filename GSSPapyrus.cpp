#include "GSSPapyrus.h"
#include <string>
#include <algorithm>
#include "GameForms.h"
#include "GameData.h"
#include "GameRTTI.h"
#include "PapyrusNativeFunctions.h"

#include "../ultimateplugins_common/UPCommon.h"

#if _GSS

bool GSS_Active;
bool GSS_bBlockAttackStart;
bool GSS_bBlockAttackRelease;
bool GSS_bBlockReloadStart;

UInt32 GSSKeywordFormId;

bool BlockAnimGSS(std::string str)
{
	PlayerCharacter* player = *g_thePlayer;

	std::transform(str.begin(), str.end(), str.begin(), tolower);

	if (str != "attackrelease" && str != "crossbowattackstart" && str != "attackstart" && str != "reloadstart")
		return false;

	TESForm* form = player->GetEquippedObject(false);
	if (!form || !UPCommon::FormHasKeyword(form, GSSKeywordFormId))
		return false;

	if (str == "attackrelease" && GSS_bBlockAttackRelease)
		return true;
	if ((str == "crossbowattackstart" || str == "attackstart") && GSS_bBlockAttackStart)
		return true;
	if (str == "reloadstart" && GSS_bBlockReloadStart)
		return true;

	return false;
}

static void BlockAttackRelease(StaticFunctionTag* base, bool flag)
{
	GSS_bBlockAttackRelease = flag;
}

static void BlockAttackStart(StaticFunctionTag* base, bool flag)
{
	GSS_bBlockAttackStart = flag;
}

static void BlockReloadStart(StaticFunctionTag* base, bool flag)
{
	GSS_bBlockReloadStart = flag;
}

static bool IsBlockAttackRelease(StaticFunctionTag* base)
{
	return GSS_bBlockAttackRelease;
}

static bool IsBlockAttackStart(StaticFunctionTag* base)
{
	return GSS_bBlockAttackStart;
}

static bool IsBlockReloadStart(StaticFunctionTag* base)
{
	return GSS_bBlockReloadStart;
}


bool GSSPapyrusRegister(VMClassRegistry* registry)
{
	registry->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, bool>("BlockAttackRelease", "GunsmithSystem", BlockAttackRelease,
		                                                   registry));

	registry->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, bool>("BlockAttackStart", "GunsmithSystem", BlockAttackStart, registry));

	registry->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, bool>("BlockReloadStart", "GunsmithSystem", BlockReloadStart, registry));

	registry->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, bool>
		("IsBlockAttackRelease", "GunsmithSystem", IsBlockAttackRelease, registry));

	registry->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, bool>("IsBlockAttackStart", "GunsmithSystem", IsBlockAttackStart, registry));

	registry->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, bool>("IsBlockReloadStart", "GunsmithSystem", IsBlockReloadStart, registry));

	GSSKeywordFormId = UPCommon::GetFormId(GSS_ESP_NAME, GSS_KEYWORD_FORMID);

	return true;
}

#endif
