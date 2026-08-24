#include "app_part1.inc"

#define StartMacro PpcLegacyStartMacro
#define StopMacro PpcLegacyStopMacro
#define RefreshRuntimeHotkeys PpcLegacyRefreshRuntimeHotkeys
#include "app_part2.inc"
#undef RefreshRuntimeHotkeys
#undef StopMacro
#undef StartMacro

#define MakeChildControls PpcLegacyMakeChildControls
#define DrawUi PpcLegacyDrawUi
#include "app_part3.inc"
#undef DrawUi
#undef MakeChildControls

#include "app_power.inc"
#include "app_part4.inc"
