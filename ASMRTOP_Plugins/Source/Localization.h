#pragma once
#include <JuceHeader.h>
#include "PluginBranding.h"

// Helper macro for UTF-8 Chinese strings
#define ZH(s) juce::String::fromUTF8(s)

namespace Asmrtop {

class Lang {
public:
    enum ID {
        CHANNEL, MODE, STATUS, START, STOP, IDLE, ACTIVE,
        SAMPLE_RATE, BUFFER, LATENCY, VOLUME,
        IPC, LAN, MOBILE, 
        UDP_PORT, WS_PORT, LAN_TARGET,
        FIX_FW, FIXING, FW_DONE, FW_FAILED,
        QR_BTN, QR_TITLE, QR_HINT1, QR_HINT2,
        CONNECTED, DISCONNECTED, CLIENTS,
        SEND_TITLE, LAN_HINT, MOBILE_HINT,
        RECV_TITLE,
        V2W_TITLE, V2W_DEVICE, V2W_IPC_MODE, V2W_WASAPI_MODE,
        V2W_GAIN, V2W_LIMITER, V2W_MONITOR,
        W2V_TITLE, W2V_DEVICE, W2V_IPC_MODE, W2V_WASAPI_MODE,
        OPEN_PORT, PORT_OK, LANG_SWITCH,
        SEC_IPC, SEC_LOOPBACK, SEC_RECORDING, SEC_PLAYBACK, SEC_DRIVER_BRIDGE,
        _COUNT
    };
    
    static bool isChinese() { return chinese; }
    static void toggle() { chinese = !chinese; }
    static void setChinese(bool cn) { chinese = cn; }
    
    static juce::String get(ID id) {
        if (chinese) {
            switch(id) {
                case CHANNEL:       return ZH("\xe9\x80\x9a\xe9\x81\x93");
                case MODE:          return ZH("\xe6\xa8\xa1\xe5\xbc\x8f");
                case STATUS:        return ZH("\xe7\x8a\xb6\xe6\x80\x81");
                case START:         return ZH("\xe5\x90\xaf\xe5\x8a\xa8");
                case STOP:          return ZH("\xe5\x81\x9c\xe6\xad\xa2");
                case IDLE:          return ZH("\xe7\xa9\xba\xe9\x97\xb2");
                case ACTIVE:        return ZH("\xe8\xbf\x90\xe8\xa1\x8c\xe4\xb8\xad");
                case SAMPLE_RATE:   return ZH("\xe9\x87\x87\xe6\xa0\xb7\xe7\x8e\x87");
                case BUFFER:        return ZH("\xe7\xbc\x93\xe5\x86\xb2");
                case LATENCY:       return ZH("\xe5\xbb\xb6\xe8\xbf\x9f");
                case VOLUME:        return ZH("\xe9\x9f\xb3\xe9\x87\x8f");
                case IPC:           return "IPC";
                case LAN:           return ZH("\xe5\xb1\x80\xe5\x9f\x9f\xe7\xbd\x91");
                case MOBILE:        return ZH("\xe6\x89\x8b\xe6\x9c\xba\xe7\x9b\x91\xe5\x90\xac");
                case UDP_PORT:      return ZH("UDP \xe7\xab\xaf\xe5\x8f\xa3");
                case WS_PORT:       return ZH("WS \xe7\xab\xaf\xe5\x8f\xa3");
                case LAN_TARGET:    return ZH("\xe7\x9b\xae\xe6\xa0\x87\xe5\x9c\xb0\xe5\x9d\x80");
                case FIX_FW:        return ZH("\xe4\xbf\xae\xe5\xa4\x8d\xe9\x98\xb2\xe7\x81\xab\xe5\xa2\x99");
                case FIXING:        return ZH("\xe4\xbf\xae\xe5\xa4\x8d\xe4\xb8\xad...");
                case FW_DONE:       return ZH("\xe5\xb7\xb2\xe4\xbf\xae\xe5\xa4\x8d");
                case FW_FAILED:     return ZH("\xe5\xa4\xb1\xe8\xb4\xa5");
                case QR_BTN:        return ZH("\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81");
                case QR_TITLE:      return ZH("\xe6\x89\xab\xe7\xa0\x81\xe7\x9b\x91\xe5\x90\xac");
                case QR_HINT1:      return ZH("\xe6\x89\x8b\xe6\x9c\xba\xe6\xb5\x8f\xe8\xa7\x88\xe5\x99\xa8\xe6\x89\x93\xe5\xbc\x80\xe4\xbb\xa5\xe4\xb8\x8a\xe7\xbd\x91\xe5\x9d\x80");
                case QR_HINT2:      return ZH("\xe7\xa1\xae\xe4\xbf\x9d\xe6\x89\x8b\xe6\x9c\xba\xe5\x92\x8c\xe7\x94\xb5\xe8\x84\x91\xe5\x9c\xa8\xe5\x90\x8c\xe4\xb8\x80WiFi");
                case CONNECTED:     return ZH("\xe5\xb7\xb2\xe8\xbf\x9e\xe6\x8e\xa5");
                case DISCONNECTED:  return ZH("\xe6\x9c\xaa\xe8\xbf\x9e\xe6\x8e\xa5");
                case CLIENTS:       return ZH("\xe5\xae\xa2\xe6\x88\xb7\xe7\xab\xaf");
                case SEND_TITLE:    return juce::String(PLUGIN_COMPANY_NAME) + ZH(" \xe5\x8f\x91\xe9\x80\x81");
                case LAN_HINT:      return ZH("\xe7\x9b\xae\xe6\xa0\x87: 127.0.0.1=\xe6\x9c\xac\xe6\x9c\xba | 192.168.x.x=\xe5\x85\xb6\xe4\xbb\x96\xe7\x94\xb5\xe8\x84\x91 | .255=\xe5\xb9\xbf\xe6\x92\xad");
                case MOBILE_HINT:   return ZH("\xe7\x82\xb9\xe5\x87\xbb\xe4\xba\x8c\xe7\xbb\xb4\xe7\xa0\x81\xe6\x89\xab\xe7\xa0\x81\xe8\xbf\x9e\xe6\x8e\xa5\xe6\x89\x8b\xe6\x9c\xba");
                case RECV_TITLE:    return juce::String(PLUGIN_COMPANY_NAME) + ZH(" \xe6\x8e\xa5\xe6\x94\xb6");
                case V2W_TITLE:     return PLUGIN_V2W_TITLE;
                case V2W_DEVICE:    return ZH("\xe8\xbe\x93\xe5\x87\xba\xe8\xae\xbe\xe5\xa4\x87");
                case V2W_IPC_MODE:  return ZH("IPC \xe6\xa8\xa1\xe5\xbc\x8f");
                case V2W_WASAPI_MODE: return ZH("WASAPI \xe6\xa8\xa1\xe5\xbc\x8f");
                case V2W_GAIN:      return ZH("\xe9\x9f\xb3\xe9\x87\x8f");
                case V2W_LIMITER:   return ZH("\xe9\x99\x90\xe5\x88\xb6\xe5\x99\xa8");
                case V2W_MONITOR:   return ZH("\xe7\x9b\x91\xe5\x90\xac");
                case W2V_TITLE:     return PLUGIN_W2V_TITLE;
                case W2V_DEVICE:    return ZH("\xe8\xbe\x93\xe5\x85\xa5\xe8\xae\xbe\xe5\xa4\x87");
                case W2V_IPC_MODE:  return ZH("IPC \xe6\xa8\xa1\xe5\xbc\x8f");
                case W2V_WASAPI_MODE: return ZH("WASAPI \xe6\xa8\xa1\xe5\xbc\x8f");
                case OPEN_PORT:     return ZH("\xe5\xbc\x80\xe6\x94\xbe\xe7\xab\xaf\xe5\x8f\xa3");
                case PORT_OK:       return ZH("\xe7\xab\xaf\xe5\x8f\xa3\xe6\xad\xa3\xe5\xb8\xb8");
                case LANG_SWITCH:   return "EN";
                case SEC_IPC:       return ZH("[ IPC \xe9\x80\x9a\xe9\x81\x93 ] \xe9\x9b\xb6\xe5\xbb\xb6\xe8\xbf\x9f");
                case SEC_LOOPBACK:  return ZH("[ \xe7\xb3\xbb\xe7\xbb\x9f\xe5\x9b\x9e\xe7\x8e\xaf ] \xe9\xab\x98\xe5\xbb\xb6\xe8\xbf\x9f");
                case SEC_RECORDING: return ZH("[ \xe5\xbd\x95\xe9\x9f\xb3\xe8\xae\xbe\xe5\xa4\x87 ]");
                case SEC_PLAYBACK:  return ZH("[ \xe6\x92\xad\xe6\x94\xbe\xe8\xae\xbe\xe5\xa4\x87 ]");
                case SEC_DRIVER_BRIDGE: return ZH("[ \xe9\xa9\xb1\xe5\x8a\xa8\xe6\xa1\xa5\xe6\x8e\xa5 ]");
                default: return "?";
            }
        } else {
            switch(id) {
                case CHANNEL:       return "CHANNEL";
                case MODE:          return "MODE";
                case STATUS:        return "STATUS";
                case START:         return "START";
                case STOP:          return "STOP";
                case IDLE:          return "Idle";
                case ACTIVE:        return "Active";
                case SAMPLE_RATE:   return "Sample Rate";
                case BUFFER:        return "Buffer";
                case LATENCY:       return "Latency";
                case VOLUME:        return "Volume";
                case IPC:           return "IPC";
                case LAN:           return "LAN";
                case MOBILE:        return "Mobile";
                case UDP_PORT:      return "UDP PORT";
                case WS_PORT:       return "WS PORT";
                case LAN_TARGET:    return "LAN TARGET";
                case FIX_FW:        return "Fix FW";
                case FIXING:        return "Fixing...";
                case FW_DONE:       return "Done";
                case FW_FAILED:     return "Failed";
                case QR_BTN:        return "QR";
                case QR_TITLE:      return "Scan to Monitor";
                case QR_HINT1:      return "Open this URL on your phone browser";
                case QR_HINT2:      return "Make sure phone and PC are on the same WiFi";
                case CONNECTED:     return "Connected";
                case DISCONNECTED:  return "Disconnected";
                case CLIENTS:       return "clients";
                case SEND_TITLE:    return juce::String(PLUGIN_COMPANY_NAME) + " Send";
                case LAN_HINT:      return "TARGET: 127.0.0.1=Same PC | 192.168.x.x=Other PC | .255=Broadcast";
                case MOBILE_HINT:   return "Click QR button to scan with phone";
                case RECV_TITLE:    return juce::String(PLUGIN_COMPANY_NAME) + " Receive";
                case V2W_TITLE:     return PLUGIN_V2W_TITLE;
                case V2W_DEVICE:    return "Output Device";
                case V2W_IPC_MODE:  return "IPC Mode";
                case V2W_WASAPI_MODE: return "WASAPI Mode";
                case V2W_GAIN:      return "Volume";
                case V2W_LIMITER:   return "Limiter";
                case V2W_MONITOR:   return "Monitor";
                case W2V_TITLE:     return PLUGIN_W2V_TITLE;
                case W2V_DEVICE:    return "Input Device";
                case W2V_IPC_MODE:  return "IPC Mode";
                case W2V_WASAPI_MODE: return "WASAPI Mode";
                case OPEN_PORT:     return "Open Port";
                case PORT_OK:       return "Port OK";
                case LANG_SWITCH:   return "CN";
                case SEC_IPC:       return "[ IPC Channel ] Zero Latency";
                case SEC_LOOPBACK:  return "[ System Loopback ] High Latency";
                case SEC_RECORDING: return "[ Recording Devices ]";
                case SEC_PLAYBACK:  return "[ Playback Devices ]";
                case SEC_DRIVER_BRIDGE: return "[ Driver Bridge ]";
                default: return "?";
            }
        }
    }
    
    static juce::String t(ID id) { return get(id); }

private:
    static inline bool chinese = true;
};

} // namespace Asmrtop
