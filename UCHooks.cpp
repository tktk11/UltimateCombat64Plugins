#include "../skse64_common/SafeWrite.h"
#include "../skse64_common/BranchTrampoline.h"

#include "../skse64/PapyrusEvents.h"
#include "../skse64/GameRTTI.h"
#include "../skse64/GameReferences.h"

#include "../ultimateplugins_common/UPCommon.h"
#include "../ultimateplugins_common/EventFunctors.h"

#include <cinttypes>

#include <xbyak/xbyak.h>

#include "UltimateCombat.h"
#include "UCHooks.h"
#include "GSSPapyrus.h"
#include "Offsets.h"

extern SKSEMessagingInterface* g_messaging;

bool bNPCBlockKillmove = false;

UInt32 UCQuestFormID;
UInt32 UCStaggerEffectFormID;

typedef bool (*_IAnimationGraphManagerHolder_Unk1)(uintptr_t* iagmh, BSFixedString* animName);
RelocAddr<_IAnimationGraphManagerHolder_Unk1> IAnimationGraphManagerHolder_OriginalUnk1(
	IANIMATIONGRAPHMANAGERHOLDER_ORIGINALUNK1);
// these call the same function. just different vtbl for player character & other npcs
RelocAddr<uintptr_t> Character_IAnimationGraphManagerHolder_Unk1_vtbl(CHARACTER_IANIMATIONGRAPHMANAGERHOLDER_UNK1_VTBL);
RelocAddr<uintptr_t> PlayerCharacter_IAnimationGraphManagerHolder_Unk1_vtbl(
	PLAYERCHARACTER_IANIMATIONGRAPHMANAGERHOLDER_UNK1_VTBL);

bool Hooked_IAnimationGraphManagerHolder_Unk1(uintptr_t* iagmh, BSFixedString* animName)
{
	T(_DMESSAGE("[TRACE] In IAnimationGraphManagerHolder_Unk1 with IAGMH pointer 0x%016" PRIXPTR " and animName %s", iagmh,
		animName->c_str()));

	const auto ref = reinterpret_cast<TESObjectREFR*>(iagmh - 7); // IAnimationGraphManager is at 0x38 in TESObjectREFR	

	if (ref->formType == kFormType_Character)
	{
		const auto actor = DYNAMIC_CAST(ref, TESObjectREFR, Actor);

		D(const char* name = UPCommon::GetActorName(actor));

		static BSFixedString staggerStart("staggerStart");

		if (*animName == staggerStart && UPCommon::ActorHasEffect(actor, UCStaggerEffectFormID))
		{
			D(_DMESSAGE("[DEBUG] <Block Stagger> on %s(%08X)", name, ref->formID));
			return false;
		}
#ifdef _GSS
		if (GSS_Active && ref == *g_thePlayer && BlockAnimGSS(animName->c_str()))
		{
			D(_DMESSAGE("[DEBUG] <Block Animation - GSS> on %s(%08X)", name, ref->formID));
			return false;
		}
#endif
	}

	return IAnimationGraphManagerHolder_OriginalUnk1(iagmh, animName);
}

typedef int64_t (*_OnProjectileHitFunction)(Projectile* akProjectile, TESObjectREFR* akTarget, NiPoint3* point,
                                            UInt32 unk1, UInt32 unk2, UInt8 unk3);
RelocAddr<_OnProjectileHitFunction> OnProjectileHitFunction(ONPROJECTILEHIT_INNERFUNCTION);
RelocAddr<uintptr_t> OnProjectileHitHookLocation(ONPROJECTILEHIT_HOOKLOCATION);

typedef UInt32*(*_GetActorCause)(TESObjectREFR* refr);
RelocAddr<_GetActorCause> Projectile_GetActorCauseFn(PROJECTILE_GETACTORCAUSEFN);

int64_t OnProjectileHitFunctionHooked(Projectile* akProjectile, TESObjectREFR* akTarget, NiPoint3* point, UInt32 unk1,
                                      UInt32 unk2, UInt8 unk3)
{
	if (akProjectile != nullptr && akProjectile->formType == kFormType_Arrow)
	{
		UInt32* handle = Projectile_GetActorCauseFn(akProjectile);

		NiPointer<TESObjectREFR> refCaster = nullptr;

		if (handle && *handle != *g_invalidRefHandle)
		{
			LookupREFRByHandle(*handle, refCaster);
		}

		D(_DMESSAGE("[DEBUG] OnHitProjectile: %08X %08X | %f %f %f", akTarget->formID, akProjectile->baseForm->formID, point->
			x,
			point->y, point->z));

		if (refCaster && refCaster == *g_thePlayer)
		{
			D(_DMESSAGE("[DEBUG] Caster is player, dispatch Event UC_OnHitProjectile"));

			const auto form = LookupFormByID(UCQuestFormID);
			// realistically this should never fail but we'll be cautious
			if (form)
			{
				const auto quest = DYNAMIC_CAST(form, TESForm, TESQuest);
				if (quest)
				{
#ifdef _DEBUG
					_MESSAGE("0x%016" PRIXPTR, quest);
#endif
					const auto vmhandle = UPCommon::GetVMHandleForQuest(quest);
					if (vmhandle)
					{
						static BSFixedString eventName("UC_OnHitProjectile");
						UPCommon::EventFunctor4<TESObjectREFR *, float, float, float
						>(eventName, akTarget, point->x, point->y, point->z)(vmhandle);
					}
				}
			}
		}
	}

	return OnProjectileHitFunction(akProjectile, akTarget, point, unk1, unk2, unk3);
}

class CombatEventHandler : public BSTEventSink<TESCombatEvent>
{
public:
	EventResult ReceiveEvent(TESCombatEvent* evn, EventDispatcher<TESCombatEvent>* dispatcher) override
	{
		const auto target = DYNAMIC_CAST(evn->target, TESObjectREFR, Actor);
		const auto source = DYNAMIC_CAST(evn->source, TESObjectREFR, Actor);

		D(
			const char* casterName = UPCommon::GetActorName(source);
			const char* targetName = UPCommon::GetActorName(target);

			switch (evn->state)
			{
				case 0:
				_DMESSAGE("[DEBUG] Not in combat:\n  caster: %s", casterName);
				break;
				case 1:
				_DMESSAGE("[DEBUG] In combat:\n  caster: %s\n  target: %s", casterName, targetName);
				break;
				case 2:
				_DMESSAGE("[DEBUG] Searching:\n  caster: %s\n  target: %s", casterName, targetName);
				break;
				default:
				break;
			}
		);

		if (!target || target != *g_thePlayer)
			return kEvent_Continue;

		D(_DMESSAGE("[DEBUG] Combat event with player as target, dispatch Event UC_OnCombat");)

		const auto form = LookupFormByID(UCQuestFormID);
		// realistically this should never fail but we'll be cautious
		if (form)
		{
			const auto quest = DYNAMIC_CAST(form, TESForm, TESQuest);
			if (quest)
			{
				T(_DMESSAGE("[DEBUG] Quest pointer - 0x%016" PRIXPTR, quest));

				const UInt64 vmhandle = UPCommon::GetVMHandleForQuest(quest);
				if (vmhandle)
				{
					static BSFixedString eventName("UC_OnCombat");
					UPCommon::EventFunctor2<Actor *, UInt32>(eventName, source, evn->state)(vmhandle);
				}
			}
		}
		return kEvent_Continue;
	}
};

CombatEventHandler g_combatEventHandler;


typedef bool (*FnEval)(TESObjectREFR* attacker, TESObjectREFR* target, int64_t unk01, double* result);
RelocAddr<FnEval> ShouldAttackKillEval_Original(SHOULDATTACKKILL_EVAL_ORIGINAL);
RelocAddr<uintptr_t> ShouldAttackKill_commandtable_eval(SHOULDATTACKKILL_COMMANDTABLE_EVAL);

bool ShouldAttackKillEval(TESObjectREFR* attacker, TESObjectREFR* target, int64_t unk01, double* result)
{
	T(_DMESSAGE("[TRACE] In OnShouldAttackKill"));

	if (!target || target != *g_thePlayer || !bNPCBlockKillmove)
	{
		T(_DMESSAGE("[TRACE] killmove not blocked"));
		return ShouldAttackKillEval_Original(attacker, target, unk01, result);
	}

	D(_DMESSAGE("[DEBUG] killmove blocked"));

	*result = 0.0;

	return true;
}

bool InitEventHooks()
{
	UCQuestFormID = UPCommon::GetFormId(UC_ESP_NAME, UC_QUEST_FORM_ID);
	if (!UCQuestFormID)
	{
		_FATALERROR("[ERROR] Couldn't find UC Init Quest form ID %d in ESP %s, aborting load", UC_QUEST_FORM_ID, UC_ESP_NAME);
		return false;
	}
	UCStaggerEffectFormID = UPCommon::GetFormId(UC_ESP_NAME, UC_STAGGER_EFFECT_FORM_ID);
	if (!UCStaggerEffectFormID)
	{
		_FATALERROR("[ERROR] Couldn't find UC Stagger Effect form ID %d in ESP %s, aborting load", UC_STAGGER_EFFECT_FORM_ID,
		            UC_ESP_NAME);
		return false;
	}

	if (!g_branchTrampoline.Create(1024 * 64))
	{
		_FATALERROR("[ERROR] couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
		return false;
	}

	if (!g_localTrampoline.Create(1024 * 64, nullptr))
	{
		_FATALERROR("[ERROR] couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
		return false;
	}

	_MESSAGE("[STARTUP] Hooking Character_IAnimationGraphManagerHolder_Unk1 in vtbl");
	SafeWrite64(Character_IAnimationGraphManagerHolder_Unk1_vtbl.GetUIntPtr(),
	            uintptr_t(Hooked_IAnimationGraphManagerHolder_Unk1));
	_MESSAGE("[STARTUP] Patched");

	_MESSAGE("[STARTUP] Hooking PlayerCharacter_IAnimationGraphManagerHolder_Unk1 in vtbl");
	SafeWrite64(PlayerCharacter_IAnimationGraphManagerHolder_Unk1_vtbl.GetUIntPtr(),
	            uintptr_t(Hooked_IAnimationGraphManagerHolder_Unk1));
	_MESSAGE("[STARTUP] Patched");

	_MESSAGE("[STARTUP] Hooking OnProjectileHit function");
	{
		struct DoAddHook_Code : Xbyak::CodeGenerator
		{
			DoAddHook_Code(void* buf, UInt64 hook_OnProjectileHit) : CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;
				Xbyak::Label funcLabel;

				//  .text:000000014074CFBA                 lea     r9, [r13 + 0Ch]; a4
				lea(r9, ptr[r13 + 0x0C]);
				// 	.text:000000014074CFBE                 mov[rsp + 180h + a6], 0; a6
				mov(byte[rsp + 0x180 + 0x158], 0);
				// 	.text:000000014074CFC3                 mov[rsp + 180h + a5], ecx; a5
				mov(dword[rsp + 0x180 + 0x160], ecx);
				// 	.text:000000014074CFC7                 mov     r8, r13; a3
				mov(r8, r13);
				// 	.text:000000014074CFCA                 mov     rdx, rbx; a2
				mov(rdx, rbx);
				// 	.text:000000014074CFCD                 mov     rcx, r14; a1
				mov(rcx, r14);
				// 	.text:000000014074CFD0                 call    sub_140753670
				// int64_t OnProjectileHitFunctionHooked(Projectile * akProjectile, TESObjectREFR * akTarget, NiPoint3 * point, UInt32 unk1, UInt32 unk2, UInt8 unk3)
				call(ptr[rip + funcLabel]);
				// 0x1B
				//  .text:000000014074CFD5                 lea     r8, [r14+0F0h]			
				// exit 74CFD5
				jmp(ptr[rip + retnLabel]);

				L(funcLabel);
				dq(hook_OnProjectileHit);

				L(retnLabel);
				dq(OnProjectileHitHookLocation.GetUIntPtr() + 0x1B);
			}
		};

		void* codeBuf = g_localTrampoline.StartAlloc();
		DoAddHook_Code code(codeBuf, uintptr_t(OnProjectileHitFunctionHooked));
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write6Branch(OnProjectileHitHookLocation.GetUIntPtr(), uintptr_t(code.getCode()));
	}
	_MESSAGE("[STARTUP] Patched");

	_MESSAGE("[STARTUP] Registering combat event listener");
	auto eventDispatchers = GetEventDispatcherList();
	eventDispatchers->combatDispatcher.AddEventSink(&g_combatEventHandler);
	_MESSAGE("[STARTUP] Registered");

	_MESSAGE("[STARTUP] Hooking ShouldAttackKill");
	SafeWrite64(ShouldAttackKill_commandtable_eval.GetUIntPtr(), uintptr_t(ShouldAttackKillEval));
	_MESSAGE("[STARTUP] Patched");

	return true;
}
