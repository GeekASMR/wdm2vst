#pragma once
#include <JuceHeader.h>
#include "AsmrtopIPC.h"
#include "PluginBranding.h"

namespace Asmrtop {

// ============================================================================
// Constants (C3: replace magic numbers)
// ============================================================================
constexpr int RING_BUFFER_SIZE = 131072;
constexpr int RING_BUFFER_MASK = RING_BUFFER_SIZE - 1;
constexpr int DEFAULT_UDP_PORT = 18810;
constexpr int DEFAULT_WS_PORT  = 18820;
constexpr int MAX_UDP_SAMPLES_PER_PACKET = 180; // A3: 12 + 180*2*4 = 1452 < MTU 1500
constexpr int WS_PORT_RETRY_COUNT = 10;         // A2: max port tries
constexpr int WS_PUSH_FIFO_SIZE = 32;           // B2: FIFO depth

// ============================================================================
// UDP Packet Header (matches Recv side)
// ============================================================================
#pragma pack(push, 1)
struct UdpPacketHeader {
    char     magic[4];      // "ASMR"
    uint8_t  channelId;     // 0-7
    uint8_t  channels;      // 1=mono, 2=stereo
    uint16_t sampleRate;    // actual / 1 (up to 65535)
    uint16_t seqNumber;     // wrap-around sequence
    uint16_t frameSize;     // number of samples per channel in this packet
};
#pragma pack(pop)

static_assert(sizeof(UdpPacketHeader) == 12, "UdpPacketHeader must be 12 bytes");

// ============================================================================
// P2P Sender: writes audio to shared memory + UDP + WebSocket
// ============================================================================
class P2PSender {
public:
    P2PSender() = default;
    ~P2PSender() { stop(); }

    void start(int channelId, int sampleRate, bool enableIPC, bool enableUDP,
               const juce::String& udpTargetIP, int udpPort, bool enableWS, int wsPort)
    {
        stop();
        this->channelId = channelId;
        this->currentSampleRate = sampleRate;

        // Shared memory IPC
        if (enableIPC) {
            ipcBridge = std::make_unique<SharedMemoryBridge>(PLUGIN_IPC_BASE_NAME, "P2P", channelId);
        }

        // UDP sender
        if (enableUDP && udpTargetIP.isNotEmpty()) {
            udpSocket = std::make_unique<juce::DatagramSocket>(true); // broadcast capable
            bool bound = udpSocket->bindToPort(0); // any port for sending
            this->udpTarget = udpTargetIP;
            this->udpPort = udpPort;
            udpActive = true;
            udpBindOk = bound;
            udpSendCount.store(0);
            udpLastResult.store(0);
        }

        // WebSocket server
        if (enableWS) {
            startWebSocketServer(wsPort);
        }

        active = true;
    }

    void stop()
    {
        active = false;
        udpActive = false;
        stopWebSocketServer();
        ipcBridge.reset();
        udpSocket.reset();
    }

    void sendAudio(const float* left, const float* right, int numSamples)
    {
        if (!active) return;

        // 1. Shared memory
        if (ipcBridge && ipcBridge->isConnected() && ipcBridge->getBuffer()) {
            auto* buf = ipcBridge->getBuffer();
            uint32_t w = buf->writePos.load(std::memory_order_relaxed);
            for (int i = 0; i < numSamples; ++i) {
                buf->ringL[(w + i) & IPC_RING_MASK] = left ? left[i] : 0.0f;
                buf->ringR[(w + i) & IPC_RING_MASK] = right ? right[i] : 0.0f;
            }
            buf->writePos.store(w + numSamples, std::memory_order_release);
        }

        // 2. UDP
        if (udpActive && udpSocket) {
            sendUdpPacket(left, right, numSamples);
        }

        // 3. WebSocket - push to connected clients
        if (wsRunning) {
            pushToWebSocketClients(left, right, numSamples);
        }
    }

    bool isActive() const { return active; }
    bool isIPCConnected() const { return ipcBridge && ipcBridge->isConnected(); }
    bool isUDPActive() const { return udpActive; }
    bool isWSRunning() const { return wsRunning; }
    int getWSClientCount() const { return wsClientCount.load(); }
    int getWSPort() const { return wsListenPort; }
    bool isUdpBindOk() const { return udpBindOk; }
    int getUdpSendCount() const { return udpSendCount.load(); }
    int getUdpLastResult() const { return udpLastResult.load(); }

private:
    int channelId = 0;
    int currentSampleRate = 48000;
    bool active = false;

    // IPC
    std::unique_ptr<SharedMemoryBridge> ipcBridge;

    // UDP
    std::unique_ptr<juce::DatagramSocket> udpSocket;
    juce::String udpTarget;
    int udpPort = DEFAULT_UDP_PORT;
    bool udpActive = false;
    bool udpBindOk = false;
    uint16_t udpSeqNumber = 0;
    std::atomic<int> udpSendCount { 0 };
    std::atomic<int> udpLastResult { 0 };
    // B4: Pre-allocated UDP send buffer (max packet = header + 180*2*4 = 1452)
    std::array<uint8_t, sizeof(UdpPacketHeader) + MAX_UDP_SAMPLES_PER_PACKET * 2 * sizeof(float)> udpPacketBuf {};

    void sendUdpPacket(const float* left, const float* right, int numSamples)
    {
        // A3: Split into chunks of max 180 samples per packet (1452B < MTU 1500)
        int offset = 0;
        while (offset < numSamples) {
            int chunk = juce::jmin(MAX_UDP_SAMPLES_PER_PACKET, numSamples - offset);
            int packetSize = (int)sizeof(UdpPacketHeader) + chunk * 2 * (int)sizeof(float);
            
            auto* hdr = reinterpret_cast<UdpPacketHeader*>(udpPacketBuf.data());
            hdr->magic[0] = 'A'; hdr->magic[1] = 'S'; hdr->magic[2] = 'M'; hdr->magic[3] = 'R';
            hdr->channelId = (uint8_t)channelId;
            hdr->channels = 2;
            hdr->sampleRate = (uint16_t)currentSampleRate;
            hdr->seqNumber = udpSeqNumber++;
            hdr->frameSize = (uint16_t)chunk;

            float* audioData = reinterpret_cast<float*>(udpPacketBuf.data() + sizeof(UdpPacketHeader));
            for (int i = 0; i < chunk; ++i) {
                audioData[i * 2]     = left  ? left[offset + i]  : 0.0f;
                audioData[i * 2 + 1] = right ? right[offset + i] : 0.0f;
            }

            int result = udpSocket->write(udpTarget, udpPort, udpPacketBuf.data(), packetSize);
            udpLastResult.store(result);
            if (result > 0) udpSendCount.fetch_add(1);
            offset += chunk;
        }
    }

    // ---- WebSocket Server (minimal binary streaming) ----
    int wsListenPort = DEFAULT_WS_PORT;
    bool wsRunning = false;
    std::atomic<int> wsClientCount { 0 };
    std::unique_ptr<juce::StreamingSocket> wsServerSocket;
    std::vector<std::unique_ptr<juce::StreamingSocket>> wsClients;
    std::unique_ptr<juce::Thread> wsAcceptThread;
    juce::CriticalSection wsClientLock;

    // B2: Lock-free FIFO for WebSocket push (off audio thread)
    struct WsFifoBlock {
        std::array<int16_t, 4096> pcm16 {};  // max 2048 stereo samples
        int numSamples = 0;
    };
    std::array<WsFifoBlock, WS_PUSH_FIFO_SIZE> wsFifo;
    std::atomic<int> wsFifoWriteIdx { 0 };
    std::atomic<int> wsFifoReadIdx { 0 };
    std::unique_ptr<juce::Thread> wsPushThread;

    class WSAcceptThread : public juce::Thread {
    public:
        WSAcceptThread(P2PSender& owner) : Thread("WSAccept"), owner(owner) {}
        void run() override {
            while (!threadShouldExit() && owner.wsServerSocket) {
                auto* client = owner.wsServerSocket->waitForNextConnection();
                if (client) {
                    // Read full HTTP request headers (may arrive in multiple TCP packets on mobile)
                    char buf[8192] = {0};
                    int totalRead = 0;
                    auto startMs = juce::Time::getMillisecondCounter();
                    while (totalRead < 8190) {
                        if (client->waitUntilReady(true, 2000) != 1) break;
                        int n = client->read(buf + totalRead, 8190 - totalRead, false);
                        if (n <= 0) break;
                        totalRead += n;
                        buf[totalRead] = 0;
                        // Check if we have complete headers
                        if (strstr(buf, "\r\n\r\n") != nullptr) break;
                        if (juce::Time::getMillisecondCounter() - startMs > 3000) break;
                    }
                    if (totalRead > 0) {
                        juce::String request(buf, totalRead);
                        juce::String key = extractWebSocketKey(request);
                        if (key.isNotEmpty()) {
                            // WebSocket upgrade
                            juce::String response = buildWebSocketAccept(key);
                            client->write(response.toRawUTF8(), (int)response.length());
                            juce::String config = "{\"sampleRate\":" + juce::String(owner.currentSampleRate) + ",\"channels\":2}";
                            sendWSTextFrame(client, config);
                            const juce::ScopedLock sl(owner.wsClientLock);
                            owner.wsClients.push_back(std::unique_ptr<juce::StreamingSocket>(client));
                            owner.wsClientCount.store((int)owner.wsClients.size());
                        } else if (request.startsWith("GET ")) {
                            // Serve embedded HTML page
                            serveHTTP(client);
                            delete client;
                        } else {
                            delete client;
                        }
                    } else {
                        delete client;
                    }
                }
            }
        }
    private:
        P2PSender& owner;
        
        static void serveHTTP(juce::StreamingSocket* client) {
            juce::String html = R"HTML(<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes"><title>ASMRTOP Monitor</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:-apple-system,sans-serif;background:#0F1115;color:#fff;min-height:100vh;display:flex;flex-direction:column;align-items:center}
.hd{width:100%;padding:20px 24px 16px;background:linear-gradient(180deg,#191C22,#0F1115);border-bottom:2px solid #00BCD4;text-align:center}
.hd h1{font-size:22px;background:linear-gradient(135deg,#00BCD4,#4DD0E1);-webkit-background-clip:text;background-clip:text;-webkit-text-fill-color:transparent}
.hd .sub{font-size:11px;color:#555;margin-top:4px;letter-spacing:2px;text-transform:uppercase}
.cd{width:calc(100% - 32px);max-width:420px;margin:16px auto;background:#16191F;border-radius:16px;padding:20px;border:1px solid rgba(255,255,255,.06)}
.ct{font-size:11px;font-weight:700;color:#00BCD4;letter-spacing:2px;text-transform:uppercase;margin-bottom:12px}
.ig{display:flex;gap:8px;margin-bottom:12px}.ig input{flex:1;background:#0F1115;border:1px solid rgba(255,255,255,.1);border-radius:10px;padding:12px 16px;color:#fff;font-size:16px;font-family:monospace;outline:none}
.ig input:focus{border-color:#00BCD4}.pi{max-width:80px}
.btn{width:100%;padding:14px;border:none;border-radius:12px;font-size:16px;font-weight:700;cursor:pointer}.btn.i{background:linear-gradient(135deg,#00BCD4,#0097A7);color:#fff}
.btn.c{background:linear-gradient(135deg,#E91E63,#C2185B);color:#fff}.btn:active{transform:scale(.98)}
.st{text-align:center;padding:6px 0;font-size:12px;color:#555}.st.ok{color:#4CAF50}.st.er{color:#E91E63}
.mr{display:flex;align-items:center;margin:6px 0}.ml{width:16px;font-size:10px;color:#555;font-weight:700}
.mt{flex:1;height:10px;background:#0A0C0F;border-radius:5px;overflow:hidden}
.mf{height:100%;border-radius:5px;background:linear-gradient(90deg,#00BCD4,#4CAF50,#FFEB3B,#FF5722);width:0%;transition:width 50ms linear}
canvas{width:100%;height:120px;border-radius:8px;background:#0A0C0F}
.vr{display:flex;align-items:center;gap:12px;margin-top:12px}
.vs{flex:1;-webkit-appearance:none;height:6px;background:#0A0C0F;border-radius:3px;outline:none}
.vs::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;border-radius:50%;background:#00BCD4;cursor:pointer}
.vv{width:40px;text-align:right;font-size:13px;color:#888;font-family:monospace}
.ss{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-top:12px}
.si{text-align:center;padding:8px;background:#0F1115;border-radius:8px}
.sv{font-size:18px;font-weight:700;color:#00BCD4;font-family:monospace}.sl{font-size:9px;color:#555;letter-spacing:1px;text-transform:uppercase;margin-top:2px}
</style></head><body>
<div class="hd"><h1>ASMRTOP Monitor</h1><div class="sub">Real-time Audio Monitor</div></div>
<div class="cd"><div class="ct">Connection</div><div class="ig"><input type="text" id="h" placeholder="192.168.1.100"><input type="text" id="p" class="pi" placeholder="18820" value="18820"></div>
<button class="btn i" id="b" onclick="T()">CONNECT</button><div class="st" id="s">Ready - Tap CONNECT</div></div>
<div class="cd"><div class="ct">Audio</div><div><div class="mr"><span class="ml">L</span><div class="mt"><div class="mf" id="mL"></div></div></div>
<div class="mr"><span class="ml">R</span><div class="mt"><div class="mf" id="mR"></div></div></div></div>
<canvas id="cv"></canvas>
<div class="vr"><span style="font-size:14px;cursor:pointer;color:#00BCD4;font-weight:700" id="mu" onclick="M()">VOL</span><input type="range" class="vs" id="vl" min="0" max="100" value="80" oninput="V(this.value)"><span class="vv" id="vv">80%</span></div>
<div class="ss"><div class="si"><div class="sv" id="sr">--</div><div class="sl">Sample Rate</div></div>
<div class="si"><div class="sv" id="bf">--</div><div class="sl">Buffer</div></div>
<div class="si"><div class="sv" id="pk">--</div><div class="sl">Packets/s</div></div></div></div>
<script>
var ws,ac,gn,sr=48000,vol=.8,mt=false,pc=0,lc=0,mL=0,mR=0;
var ringL=new Float32Array(65536),ringR=new Float32Array(65536),rw=0,rr=0,proc=null;
var wfHist=[];var WF_LEN=200;
function initAudio(){
if(ac)return;
ac=new(window.AudioContext||window.webkitAudioContext)({sampleRate:sr});
gn=ac.createGain();gn.gain.value=mt?0:vol;gn.connect(ac.destination);
proc=ac.createScriptProcessor(2048,0,2);
proc.onaudioprocess=function(e){
var oL=e.outputBuffer.getChannelData(0),oR=e.outputBuffer.getChannelData(1);
var avail=((rw-rr)&65535);
if(avail<2048){for(var i=0;i<2048;i++){oL[i]=0;oR[i]=0}return}
for(var i=0;i<2048;i++){var idx=(rr+i)&65535;oL[i]=ringL[idx];oR[i]=ringR[idx]}
rr=(rr+2048)&65535};
proc.connect(gn);
ac.resume()}
function T(){
if(ws&&ws.readyState<=1){ws.close();return}
var h=document.getElementById('h').value.trim(),p=document.getElementById('p').value||'18820';
if(!h){SS('Enter IP','er');return}
SS('Connecting...','');
initAudio();
if(ac&&ac.state==='suspended')ac.resume();
try{ws=new WebSocket('ws://'+h+':'+p);ws.binaryType='arraybuffer'}catch(e){SS('Error','er');return}
ws.onopen=function(){SS('Connected','ok');document.getElementById('b').className='btn c';document.getElementById('b').textContent='DISCONNECT';rw=0;rr=0};
ws.onmessage=function(e){if(typeof e.data==='string'){try{var c=JSON.parse(e.data);if(c.sampleRate){sr=c.sampleRate;document.getElementById('sr').textContent=sr}}catch(x){}}else{P(e.data);pc++}};
ws.onerror=function(){SS('Error','er');document.getElementById('b').className='btn i';document.getElementById('b').textContent='CONNECT'};
ws.onclose=function(){SS('Disconnected','');document.getElementById('b').className='btn i';document.getElementById('b').textContent='CONNECT'}}
function P(ab){
if(ac&&ac.state==='suspended')ac.resume();
var d=new Int16Array(ab),n=d.length/2;if(!n)return;
var pL=0,pR=0,sum=0;
for(var i=0;i<n;i++){var l=d[i*2]/32767.0,r=d[i*2+1]/32767.0;ringL[(rw+i)&65535]=l;ringR[(rw+i)&65535]=r;
pL=Math.max(pL,Math.abs(l));pR=Math.max(pR,Math.abs(r));sum+=Math.abs(l)+Math.abs(r)}
rw=(rw+n)&65535;mL=Math.max(mL,pL);mR=Math.max(mR,pR);
var pk=sum/(n*2);wfHist.push(pk);if(wfHist.length>WF_LEN)wfHist.shift()}
function V(v){vol=v/100;document.getElementById('vv').textContent=v+'%';if(gn&&!mt)gn.gain.setValueAtTime(vol,ac.currentTime)}
function M(){mt=!mt;document.getElementById('mu').textContent=mt?'MUTE':'VOL';if(gn)gn.gain.setValueAtTime(mt?0:vol,ac.currentTime)}
function SS(t,c){var e=document.getElementById('s');e.textContent=t;e.className='st '+c}
function U(){
document.getElementById('mL').style.width=Math.min(100,(mL*100))+'%';
document.getElementById('mR').style.width=Math.min(100,(mR*100))+'%';
mL*=.88;mR*=.88;
var now=performance.now();
if(now-lc>1e3){document.getElementById('pk').textContent=pc;pc=0;lc=now;
var avail=((rw-rr)&65535);document.getElementById('bf').textContent=Math.round(avail/48)+'ms'}
var cv=document.getElementById('cv'),cx=cv.getContext('2d');
if(cv.offsetWidth<10){requestAnimationFrame(U);return}
var W=cv.width=cv.offsetWidth*2,H=cv.height=240;
cx.fillStyle='#0A0C0F';cx.fillRect(0,0,W,H);
cx.strokeStyle='rgba(255,255,255,.04)';cx.lineWidth=1;
cx.beginPath();cx.moveTo(0,H/2);cx.lineTo(W,H/2);cx.stroke();
var len=wfHist.length;if(len>0){
var gap=1;var bw=Math.max(2,Math.floor((W-gap*len)/len));
var gU=cx.createLinearGradient(0,0,0,H/2);
gU.addColorStop(0,'#00BCD4');gU.addColorStop(1,'#006064');
var gD=cx.createLinearGradient(0,H/2,0,H);
gD.addColorStop(0,'#006064');gD.addColorStop(1,'#00BCD4');
var step=(bw+gap);
for(var i=0;i<len;i++){
var x=i*step;if(x+bw>W)break;
var amp=Math.min(1,wfHist[i]*6);var barH=amp*(H/2-4);
if(barH<1)barH=1;
cx.fillStyle=gU;cx.fillRect(x,H/2-barH,bw,barH);
cx.fillStyle=gD;cx.fillRect(x,H/2,bw,barH)}}
requestAnimationFrame(U)}requestAnimationFrame(U);
if(location.hostname&&location.hostname!=='localhost'&&location.hostname!=='127.0.0.1')document.getElementById('h').value=location.hostname;
document.addEventListener('touchstart',function(){if(ac&&ac.state==='suspended')ac.resume()},{once:true});
</script></body></html>)HTML";
            juce::String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                "Access-Control-Allow-Origin: *\r\nContent-Length: " + juce::String(html.getNumBytesAsUTF8()) + "\r\n"
                "Connection: close\r\n\r\n" + html;
            client->write(response.toRawUTF8(), (int)response.getNumBytesAsUTF8());
        }
        static juce::String extractWebSocketKey(const juce::String& request) {
            // Case-insensitive search for Sec-WebSocket-Key
            juce::String lower = request.toLowerCase();
            int keyStart = lower.indexOf("sec-websocket-key: ");
            if (keyStart < 0) return {};
            keyStart += 19;
            int keyEnd = request.indexOf(keyStart, "\r\n");
            if (keyEnd < 0) keyEnd = request.length();
            return request.substring(keyStart, keyEnd).trim();
        }
        
        static juce::String buildWebSocketAccept(const juce::String& key) {
            auto combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            uint8_t sha1[20];
            {
                auto data = combined.toUTF8();
                computeSHA1((const uint8_t*)data.getAddress(), (int)strlen(data.getAddress()), sha1);
            }
            
            auto base64 = juce::Base64::toBase64(sha1, 20);
            
            return "HTTP/1.1 101 Switching Protocols\r\n"
                   "Upgrade: websocket\r\n"
                   "Connection: Upgrade\r\n"
                   "Sec-WebSocket-Accept: " + base64 + "\r\n\r\n";
        }
        
        static void sendWSTextFrame(juce::StreamingSocket* sock, const juce::String& text) {
            auto utf8 = text.toUTF8();
            int len = (int)strlen(utf8);
            std::vector<uint8_t> frame;
            frame.push_back(0x81); // text frame, FIN
            if (len < 126) {
                frame.push_back((uint8_t)len);
            } else {
                frame.push_back(126);
                frame.push_back((uint8_t)(len >> 8));
                frame.push_back((uint8_t)(len & 0xFF));
            }
            frame.insert(frame.end(), (uint8_t*)utf8.getAddress(), (uint8_t*)utf8.getAddress() + len);
            sock->write(frame.data(), (int)frame.size());
        }
        
        // Minimal SHA-1 for WebSocket handshake
        static void computeSHA1(const uint8_t* data, int len, uint8_t out[20])
        {
            uint32_t h0=0x67452301, h1=0xEFCDAB89, h2=0x98BADCFE, h3=0x10325476, h4=0xC3D2E1F0;
            uint64_t totalBits = (uint64_t)len * 8;
            std::vector<uint8_t> msg(data, data + len);
            msg.push_back(0x80);
            while ((msg.size() % 64) != 56) msg.push_back(0x00);
            for (int i = 7; i >= 0; --i) msg.push_back((uint8_t)(totalBits >> (i * 8)));
            
            for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
                uint32_t w[80];
                for (int i = 0; i < 16; ++i)
                    w[i] = ((uint32_t)msg[chunk+i*4]<<24)|((uint32_t)msg[chunk+i*4+1]<<16)|
                            ((uint32_t)msg[chunk+i*4+2]<<8)|msg[chunk+i*4+3];
                for (int i = 16; i < 80; ++i) {
                    uint32_t t = w[i-3]^w[i-8]^w[i-14]^w[i-16];
                    w[i] = (t<<1)|(t>>31);
                }
                uint32_t a=h0,b=h1,c=h2,d=h3,e=h4;
                for (int i = 0; i < 80; ++i) {
                    uint32_t f,k;
                    if (i<20)      { f=(b&c)|((~b)&d); k=0x5A827999; }
                    else if (i<40) { f=b^c^d;           k=0x6ED9EBA1; }
                    else if (i<60) { f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC; }
                    else           { f=b^c^d;           k=0xCA62C1D6; }
                    uint32_t temp = ((a<<5)|(a>>27)) + f + e + k + w[i];
                    e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
                }
                h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
            }
            uint32_t h[] = {h0,h1,h2,h3,h4};
            for (int i = 0; i < 5; ++i) {
                out[i*4]=(uint8_t)(h[i]>>24); out[i*4+1]=(uint8_t)(h[i]>>16);
                out[i*4+2]=(uint8_t)(h[i]>>8); out[i*4+3]=(uint8_t)h[i];
            }
        }
    };

    // B2: Push thread that drains FIFO and sends to WS clients
    class WSPushThread : public juce::Thread {
    public:
        WSPushThread(P2PSender& owner) : Thread("WSPush"), owner(owner) {}
        void run() override {
            while (!threadShouldExit()) {
                int ri = owner.wsFifoReadIdx.load(std::memory_order_acquire);
                int wi = owner.wsFifoWriteIdx.load(std::memory_order_acquire);
                if (ri == wi) {
                    Thread::sleep(1); // ~1ms poll
                    continue;
                }
                auto& block = owner.wsFifo[ri % WS_PUSH_FIFO_SIZE];
                int dataLen = block.numSamples * 2 * (int)sizeof(int16_t);
                if (dataLen > 0) {
                    // Build WS binary frame
                    std::vector<uint8_t> frame;
                    frame.push_back(0x82);
                    if (dataLen < 126) {
                        frame.push_back((uint8_t)dataLen);
                    } else if (dataLen < 65536) {
                        frame.push_back(126);
                        frame.push_back((uint8_t)(dataLen >> 8));
                        frame.push_back((uint8_t)(dataLen & 0xFF));
                    } else {
                        frame.push_back(127);
                        for (int i = 7; i >= 0; --i)
                            frame.push_back((uint8_t)((uint64_t)dataLen >> (i * 8)));
                    }
                    frame.insert(frame.end(), (uint8_t*)block.pcm16.data(), (uint8_t*)block.pcm16.data() + dataLen);

                    const juce::ScopedLock sl(owner.wsClientLock);
                    for (int i = (int)owner.wsClients.size() - 1; i >= 0; --i) {
                        int written = owner.wsClients[i]->write(frame.data(), (int)frame.size());
                        if (written < 0) owner.wsClients.erase(owner.wsClients.begin() + i);
                    }
                    owner.wsClientCount.store((int)owner.wsClients.size());
                }
                owner.wsFifoReadIdx.store(ri + 1, std::memory_order_release);
            }
        }
    private:
        P2PSender& owner;
    };

    void startWebSocketServer(int port)
    {
        wsListenPort = port;
        wsServerSocket = std::make_unique<juce::StreamingSocket>();
        // A2: Auto-increment port on bind failure
        bool bound = false;
        for (int i = 0; i < WS_PORT_RETRY_COUNT; ++i) {
            if (wsServerSocket->createListener(wsListenPort, "0.0.0.0")) {
                bound = true;
                break;
            }
            wsListenPort++;
        }
        if (bound) {
            wsRunning = true;
            wsFifoWriteIdx.store(0); wsFifoReadIdx.store(0);
            wsAcceptThread = std::make_unique<WSAcceptThread>(*this);
            wsAcceptThread->startThread();
            wsPushThread = std::make_unique<WSPushThread>(*this);
            wsPushThread->startThread();
        }
    }

    void stopWebSocketServer()
    {
        wsRunning = false;
        if (wsPushThread) {
            wsPushThread->stopThread(1000);
            wsPushThread.reset();
        }
        if (wsAcceptThread) {
            wsAcceptThread->stopThread(1000);
            wsAcceptThread.reset();
        }
        {
            const juce::ScopedLock sl(wsClientLock);
            wsClients.clear();
            wsClientCount.store(0);
        }
        wsServerSocket.reset();
    }

public:
    void pushToWebSocketClients(const float* left, const float* right, int numSamples)
    {
        // B2: Push to lock-free FIFO instead of sending directly on audio thread
        int wi = wsFifoWriteIdx.load(std::memory_order_relaxed);
        int ri = wsFifoReadIdx.load(std::memory_order_acquire);
        if (wi - ri >= WS_PUSH_FIFO_SIZE) return; // FIFO full, drop

        auto& block = wsFifo[wi % WS_PUSH_FIFO_SIZE];
        int clampedSamples = juce::jmin(numSamples, (int)(block.pcm16.size() / 2));
        for (int i = 0; i < clampedSamples; ++i) {
            float l = left  ? juce::jlimit(-1.0f, 1.0f, left[i])  : 0.0f;
            float r = right ? juce::jlimit(-1.0f, 1.0f, right[i]) : 0.0f;
            block.pcm16[i * 2]     = (int16_t)(l * 32767.0f);
            block.pcm16[i * 2 + 1] = (int16_t)(r * 32767.0f);
        }
        block.numSamples = clampedSamples;
        wsFifoWriteIdx.store(wi + 1, std::memory_order_release);
    }
};

// ============================================================================
// P2P Receiver: reads audio from shared memory or UDP
// ============================================================================
class P2PReceiver {
public:
    P2PReceiver() = default;
    ~P2PReceiver() { stop(); }

    void start(int channelId, bool enableIPC, bool enableUDP, int udpPort)
    {
        stop();
        this->channelId = channelId;

        // Shared memory
        if (enableIPC) {
            ipcBridge = std::make_unique<SharedMemoryBridge>(PLUGIN_IPC_BASE_NAME, "P2P", channelId);
        }

        // UDP receiver
        if (enableUDP) {
            udpSocket = std::make_unique<juce::DatagramSocket>(true); // broadcast capable
            udpBindOk = udpSocket->bindToPort(udpPort, "0.0.0.0");
            udpRecvCount.store(0); udpWaitCount.store(0); udpMatchCount.store(0);
            if (udpBindOk) {
                udpActive = true;
                udpListenThread = std::make_unique<UdpListenThread>(*this);
                udpListenThread->startThread();
            }
        }

        active = true;
    }

    void stop()
    {
        active = false;
        udpActive = false;
        if (udpListenThread) {
            udpListenThread->stopThread(1000);
            udpListenThread.reset();
        }
        ipcBridge.reset();
        udpSocket.reset();
    }

    // Read numSamples into output buffers. Returns available samples.
    int readAudio(float* left, float* right, int numSamples)
    {
        if (!active) return 0;

        // Priority 1: Shared memory (lowest latency)
        if (ipcBridge && ipcBridge->isConnected() && ipcBridge->getBuffer()) {
            return readFromIPC(left, right, numSamples);
        }

        // Priority 2: UDP ring buffer
        if (udpActive) {
            return readFromUdpRing(left, right, numSamples);
        }

        return 0;
    }

    bool isActive() const { return active; }
    bool isIPCConnected() const { return ipcBridge && ipcBridge->isConnected(); }
    bool isUDPActive() const { return udpActive; }
    bool isUdpBindOk() const { return udpBindOk; }
    int getUdpRecvCount() const { return udpRecvCount.load(); }
    int getUdpWaitCount() const { return udpWaitCount.load(); }
    int getUdpMatchCount() const { return udpMatchCount.load(); }
    int getAvailableSamples() const {
        if (ipcBridge && ipcBridge->isConnected() && ipcBridge->getBuffer()) {
            uint32_t w = ipcBridge->getBuffer()->writePos.load(std::memory_order_relaxed);
            uint32_t r = ipcReadPos.load(std::memory_order_relaxed);
            int32_t avail = (int32_t)(w - r);
            return (avail < 0 || avail > RING_BUFFER_SIZE) ? 0 : avail;
        }
        uint32_t w = udpWritePos.load(std::memory_order_relaxed);
        uint32_t r = udpReadPos.load(std::memory_order_relaxed);
        int32_t avail = (int32_t)(w - r);
        return (avail < 0 || avail > RING_BUFFER_SIZE) ? 0 : avail;
    }
    int getReceivedSampleRate() const { return receivedSampleRate.load(); }

private:
    int channelId = 0;
    bool active = false;

    // IPC
    std::unique_ptr<SharedMemoryBridge> ipcBridge;
    std::atomic<uint32_t> ipcReadPos { 0 };

    int readFromIPC(float* left, float* right, int numSamples)
    {
        auto* buf = ipcBridge->getBuffer();
        uint32_t w = buf->writePos.load(std::memory_order_acquire);
        uint32_t r = ipcReadPos.load(std::memory_order_relaxed);
        int32_t avail = (int32_t)(w - r);
        if (avail < 0 || avail > RING_BUFFER_SIZE) { ipcReadPos.store(w); return 0; }
        int toRead = juce::jmin(numSamples, (int)avail);
        for (int i = 0; i < toRead; ++i) {
            if (left)  left[i]  = buf->ringL[(r + i) & RING_BUFFER_MASK];
            if (right) right[i] = buf->ringR[(r + i) & RING_BUFFER_MASK];
        }
        for (int i = toRead; i < numSamples; ++i) {
            if (left)  left[i]  = 0.0f;
            if (right) right[i] = 0.0f;
        }
        ipcReadPos.store(r + toRead, std::memory_order_release);
        return toRead;
    }

    // UDP
    std::unique_ptr<juce::DatagramSocket> udpSocket;
    bool udpActive = false;
    bool udpBindOk = false;
    std::unique_ptr<juce::Thread> udpListenThread;
    std::atomic<int> udpRecvCount { 0 };
    std::atomic<int> udpWaitCount { 0 };
    std::atomic<int> udpMatchCount { 0 };
    
    // UDP ring buffer (same layout as IPC for consistency)
    std::array<float, RING_BUFFER_SIZE> udpRingL {};
    std::array<float, RING_BUFFER_SIZE> udpRingR {};
    std::atomic<uint32_t> udpWritePos { 0 };
    std::atomic<uint32_t> udpReadPos { 0 };
    std::atomic<int> receivedSampleRate { 48000 };

    class UdpListenThread : public juce::Thread {
    public:
        UdpListenThread(P2PReceiver& owner) : Thread("UDPRecv"), owner(owner) {}
        void run() override {
            std::vector<uint8_t> buf(65536);
            while (!threadShouldExit() && owner.udpSocket) {
                int ready = owner.udpSocket->waitUntilReady(true, 500);
                owner.udpWaitCount.fetch_add(1);
                if (ready != 1) continue;
                juce::String senderIP;
                int senderPort;
                int bytesRead = owner.udpSocket->read(buf.data(), (int)buf.size(), false, senderIP, senderPort);
                owner.udpRecvCount.fetch_add(1);
                if (bytesRead >= (int)sizeof(UdpPacketHeader)) {
                    auto* hdr = reinterpret_cast<const UdpPacketHeader*>(buf.data());
                    if (hdr->magic[0]=='A' && hdr->magic[1]=='S' && hdr->magic[2]=='M' && hdr->magic[3]=='R'
                        && hdr->channelId == owner.channelId)
                    {
                        owner.udpMatchCount.fetch_add(1);
                        owner.receivedSampleRate.store(hdr->sampleRate);
                        int samples = hdr->frameSize;
                        const float* audioData = reinterpret_cast<const float*>(buf.data() + sizeof(UdpPacketHeader));
                        int expectedBytes = (int)sizeof(UdpPacketHeader) + samples * hdr->channels * (int)sizeof(float);
                        if (bytesRead >= expectedBytes) {
                            uint32_t w = owner.udpWritePos.load(std::memory_order_relaxed);
                            for (int i = 0; i < samples; ++i) {
                                owner.udpRingL[(w + i) & RING_BUFFER_MASK] = audioData[i * 2];
                                owner.udpRingR[(w + i) & RING_BUFFER_MASK] = (hdr->channels > 1) ? audioData[i * 2 + 1] : audioData[i * 2];
                            }
                            owner.udpWritePos.store(w + samples, std::memory_order_release);
                        }
                    }
                }
            }
        }
    private:
        P2PReceiver& owner;
    };

    int readFromUdpRing(float* left, float* right, int numSamples)
    {
        uint32_t w = udpWritePos.load(std::memory_order_acquire);
        uint32_t r = udpReadPos.load(std::memory_order_relaxed);
        int32_t avail = (int32_t)(w - r);
        if (avail < 0 || avail > RING_BUFFER_SIZE) { udpReadPos.store(w); return 0; }
        int toRead = juce::jmin(numSamples, (int)avail);
        for (int i = 0; i < toRead; ++i) {
            if (left)  left[i]  = udpRingL[(r + i) & RING_BUFFER_MASK];
            if (right) right[i] = udpRingR[(r + i) & RING_BUFFER_MASK];
        }
        for (int i = toRead; i < numSamples; ++i) {
            if (left)  left[i]  = 0.0f;
            if (right) right[i] = 0.0f;
        }
        udpReadPos.store(r + toRead, std::memory_order_release);
        return toRead;
    }
};

} // namespace Asmrtop
