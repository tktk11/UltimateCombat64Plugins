#pragma once

#define _GSS	1
#include "PapyrusVM.h"

#if _GSS
#define GSS_ESP_NAME         "dwrsp.esp"
#define GSS_KEYWORD_FORMID   0x0484D9

extern bool GSS_Active;
bool BlockAnimGSS(std::string str);

bool GSSPapyrusRegister(VMClassRegistry* registry);
#endif
