#pragma once

// SSE 1.5.73

#define IANIMATIONGRAPHMANAGERHOLDER_ORIGINALUNK1				0x0060F240
#define CHARACTER_IANIMATIONGRAPHMANAGERHOLDER_UNK1_VTBL		0x0165E3E0 // ??_7Character@@6B@_2			vtbl[1]
#define PLAYERCHARACTER_IANIMATIONGRAPHMANAGERHOLDER_UNK1_VTBL	0x01663FA8 // ??_7PlayerCharacter@@6B@_2	vtbl[1]

// .text:000000014074CFBA                 lea     r9, [r13 + 0Ch]; a4
// .text:000000014074CFBE                 mov[rsp + 180h + a6], 0; a6
// .text:000000014074CFC3                 mov[rsp + 180h + a5], ecx; a5
// .text:000000014074CFC7                 mov     r8, r13; a3
// .text:000000014074CFCA                 mov     rdx, rbx; a2
// .text:000000014074CFCD                 mov     rcx, r14; a1
// .text:000000014074CFD0                 call    sub_140753670
// 33 C9 4D 8D 4D 0C 
#define ONPROJECTILEHIT_HOOKLOCATION							0x0074CB0A
#define ONPROJECTILEHIT_INNERFUNCTION							0x007531C0

#define PROJECTILE_GETACTORCAUSEFN								0x0074DCF0 // ??_7Projectile@@6B@			vtbl[51]

#define SHOULDATTACKKILL_EVAL_ORIGINAL							0x002E2340
#define SHOULDATTACKKILL_COMMANDTABLE_EVAL						0x01DC5D30 // "ShouldAttackKill" command table function 3
