#pragma once

// ============================================================================
// Plugin Branding Configuration
// Comment out ASMRTOP_BRAND to build the public version.
// Or use: set_brand.ps1 asmrtop / set_brand.ps1 public
// ============================================================================
// >>> BRAND:ASMRTOP <<<
#define ASMRTOP_PLUGIN_BRAND

#ifdef ASMRTOP_PLUGIN_BRAND

#define PLUGIN_COMPANY_NAME   "ASMRTOP"
#define PLUGIN_W2V_NAME       "ASMRTOP WDM2VST"
#define PLUGIN_V2W_NAME       "ASMRTOP VST2WDM"
#define PLUGIN_IPC_BASE_NAME  "AsmrtopWDM"
#define PLUGIN_W2V_TITLE      "ASMRTOP WDM2VST"
#define PLUGIN_V2W_TITLE      "ASMRTOP VST2WDM"

#else  // Public version

#define PLUGIN_COMPANY_NAME   "VirtualAudioRouter"
#define PLUGIN_W2V_NAME       "WDM2VST"
#define PLUGIN_V2W_NAME       "VST2WDM"
#define PLUGIN_IPC_BASE_NAME  "VirtualAudioWDM"
#define PLUGIN_W2V_TITLE      "WDM2VST"
#define PLUGIN_V2W_TITLE      "VST2WDM"

#endif // ASMRTOP_PLUGIN_BRAND
