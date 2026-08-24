#include "input_transport.hpp"

// Route only the macro engine's synthetic keyboard output through the
// DirectInput-compatible / official game-bridge transport. UI/hotkey capture stays native.
#define SendInput ppc_input::SendInputScanCodeCompatible
#include "app_part1.inc"

#define StartMacro PpcLegacyStartMacro
#define StopMacro PpcLegacyStopMacro
#define RefreshRuntimeHotkeys PpcLegacyRefreshRuntimeHotkeys
#include "app_part2.inc"
#undef RefreshRuntimeHotkeys
#undef StopMacro
#undef StartMacro
#undef SendInput

#define MakeChildControls PpcLegacyMakeChildControls
#define DrawUi PpcLegacyDrawUi
#define LoadRegistry PpcLegacyLoadRegistry
#include "app_part3.inc"
#undef LoadRegistry
#undef DrawUi
#undef MakeChildControls

#include "app_power.inc"
#include "app_part4.inc"
