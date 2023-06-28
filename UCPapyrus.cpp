#include "PapyrusNativeFunctions.h"
#include "UCHooks.h"
#include "GameMenus.h"
#include "../ultimateplugins_common/DummyMenu.h"
#include "../ultimateplugins_common/UPCommon.h"

bool isPaused = false;

void PauseGame(StaticFunctionTag* base, bool bPause)
{
	D(_DMESSAGE("[DEBUG] Setting pause %d", bPause));

	isPaused = bPause;
	static BSFixedString s("DummyMenu_UC");
	CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)(
		&s, (bPause) ? UIMessage::kMessage_Open : UIMessage::kMessage_Close, nullptr);
}

bool IsGamePaused(StaticFunctionTag* base)
{
	T(_DMESSAGE("[TRACE] IsGamePaused"));
	return isPaused;
}

void BlockNPCKillmove(StaticFunctionTag* base, bool bBlock)
{
	T(_MESSAGE("[TRACE] setting NPC killmove to %d", bBlock));
	bNPCBlockKillmove = bBlock;
}

bool IsBlockNPCKillmove(StaticFunctionTag* base)
{
	return bNPCBlockKillmove;
}

bool UCPapyrusRegister(VMClassRegistry* registry)
{
	_MESSAGE("[STARTUP] Registering UCPapyrus Functions");
	registry->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, bool>("PauseGame", "aaaUCPlayerAliasScript", PauseGame, registry));

	registry->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, bool>("IsGamePaused", "aaaUCPlayerAliasScript", IsGamePaused, registry));

	registry->RegisterFunction(
		new NativeFunction1<StaticFunctionTag, void, bool>("BlockNPCKillmove", "aaaUCPlayerAliasScript", BlockNPCKillmove,
		                                                   registry));

	registry->RegisterFunction(
		new NativeFunction0<StaticFunctionTag, bool>("IsBlockNPCKillMove", "aaaUCPlayerAliasScript", IsBlockNPCKillmove,
		                                             registry));
	_MESSAGE("[STARTUP] Done");

	MenuManager* mm = MenuManager::GetSingleton();
	if (mm)
		mm->Register("DummyMenu_UC", UPCommon::DummyMenuCreator::Create);

	return true;
}
