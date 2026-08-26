#include "input_transport.hpp"

// Route only the macro engine's synthetic keyboard output through the
// DirectInput-compatible / official game-bridge transport. UI/hotkey capture stays native.
#define SendInput ppc_input::SendInputScanCodeCompatible

// Keep the original zero-duration key helper compiled for regression comparison,
// but production game input uses the paced non-overlapping implementation below.
#define SendKey PpcLegacySendKey
#include "app_part1.inc"
#undef SendKey

#include "game_input_runtime.inc"

#define StartMacro PpcLegacyStartMacro
#define StopMacro PpcLegacyStopMacro
#define RefreshRuntimeHotkeys PpcLegacyRefreshRuntimeHotkeys
#define SendCombo PpcLegacySendCombo
#define ComboWorker PpcLegacyComboWorker
#define RWorker PpcLegacyRWorker
#define UpdateDiagnosticMarkers PpcLegacyUpdateDiagnosticMarkers
#include "app_part2.inc"
#undef UpdateDiagnosticMarkers
#undef RWorker
#undef ComboWorker
#undef SendCombo
#undef RefreshRuntimeHotkeys
#undef StopMacro
#undef StartMacro

#include "game_input_workers.inc"
#undef SendInput

#define MakeChildControls PpcLegacyMakeChildControls
#define DrawUi PpcLegacyDrawUi
#define LoadRegistry PpcLegacyLoadRegistry
#include "app_part3.inc"
#undef LoadRegistry
#undef DrawUi
#undef MakeChildControls

// Keep the locked UI/power implementation intact. The wrapper below only repairs
// the bad TAB/CAPS migration that was accidentally shipped in an intermediate build.
#define LoadRegistry PpcPowerLoadRegistry
#include "app_power.inc"
#undef LoadRegistry
#include "app_restore_migration.inc"
#include "app_part4.inc"
