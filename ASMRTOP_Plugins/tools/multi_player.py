"""
A/B Player v5 — Premium UI with pywebview + HTML/CSS
Compact, bold, Spotify-meets-DJ aesthetic.
"""
import webview
import sounddevice as sd
import soundfile as sf
import numpy as np
import os, json, threading, time

# ─── Audio Engine ───
class Engine:
    def __init__(self):
        self.data = None
        self.sr = 48000
        self.playing = False
        self.loop = False
        self.tracks = []  # [{id, did, name, vol, mute, solo, stream, peak, pos}]
        self.tid = 0
        self.api = "WDM-KS"
        self._lock = threading.Lock()
    
    def get_devices(self):
        devs = []
        for i, d in enumerate(sd.query_devices()):
            if d['max_output_channels'] >= 2:
                api = sd.query_hostapis(d['hostapi'])['name']
                if self.api != "ALL" and self.api not in api:
                    continue
                devs.append({'id': i, 'name': d['name'], 'api': api,
                    'full': f"{d['name']} [{api}]", 'ch': d['max_output_channels']})
        return devs
    
    def load_file(self, path):
        self.stop()
        self.data, self.sr = sf.read(path, dtype='float32')
        if self.data.ndim == 1:
            self.data = np.column_stack([self.data, self.data])
        name = os.path.basename(path)
        dur = len(self.data) / self.sr
        return {'name': name, 'duration': dur, 'sr': self.sr, 'ch': self.data.shape[1]}
    
    def add_track(self, dev_id, dev_name):
        self.tid += 1
        t = {'id': self.tid, 'did': dev_id, 'name': dev_name,
             'vol': 1.0, 'mute': False, 'solo': False,
             'stream': None, 'peak': 0.0, 'pos': 0,
             'data': None, 'sr': None, 'fname': None,
             'playing': False, 'tloop': False}
        self.tracks.append(t)
        return self.tid
    
    def load_track_file(self, tid, path):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if not t:
            return None
        data, sr = sf.read(path, dtype='float32')
        if data.ndim == 1:
            data = np.column_stack([data, data])
        t['data'] = data
        t['sr'] = sr
        t['pos'] = 0
        t['fname'] = os.path.basename(path)
        dur = len(data) / sr
        return {'name': t['fname'], 'duration': dur, 'sr': sr, 'ch': data.shape[1]}
    
    def remove_track(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if t:
            if t['stream']:
                t['stream'].stop()
                t['stream'].close()
            self.tracks.remove(t)
    
    def set_volume(self, tid, vol):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if t: t['vol'] = vol
    
    def toggle_mute(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if t: t['mute'] = not t['mute']
        return t['mute'] if t else False
    
    def toggle_solo(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if t: t['solo'] = not t['solo']
        return t['solo'] if t else False
    
    def _get_track_data(self, t):
        """Get audio data for a track: per-track file or global file."""
        if t['data'] is not None:
            return t['data']
        return self.data
    
    def _get_track_sr(self, t):
        if t['sr'] is not None:
            return t['sr']
        return self.sr
    
    def _is_track_active(self, t):
        """Check if track should be playing: per-track state if has own file, else global."""
        if t['data'] is not None:
            return t['playing']
        return self.playing
    
    def _get_loop(self, t):
        if t['data'] is not None:
            return t['tloop']
        return self.loop
    
    def _make_cb(self, t):
        def cb(out, frames, ti, st):
            audio = self._get_track_data(t)
            if audio is None or not self._is_track_active(t):
                out[:] = 0; return
            total = len(audio)
            pos = t['pos']
            if pos >= total:
                if self._get_loop(t):
                    t['pos'] = pos = 0
                else:
                    out[:] = 0
                    if t['data'] is not None:
                        t['playing'] = False
                    return
            end = min(pos + frames, total)
            chunk = audio[pos:end].copy()
            t['pos'] = end
            has_solo = any(x['solo'] for x in self.tracks)
            vol = 0.0 if (t['mute'] or (has_solo and not t['solo'])) else t['vol']
            chunk *= vol
            if len(chunk) < frames:
                chunk = np.vstack([chunk, np.zeros((frames-len(chunk), chunk.shape[1]), dtype='float32')])
            oc, ic = out.shape[1], chunk.shape[1]
            if ic >= oc:
                out[:] = chunk[:, :oc]
            else:
                out[:, :ic] = chunk
                for c in range(ic, oc):
                    out[:, c] = chunk[:, ic-1]
            t['peak'] = float(np.max(np.abs(out))) if vol > 0 else 0.0
        return cb
    
    def _start_stream(self, t):
        if t['stream']:
            return
        try:
            devs = sd.query_devices()
            ch = min(2, devs[t['did']]['max_output_channels'])
            sr = self._get_track_sr(t)
            t['stream'] = sd.OutputStream(
                samplerate=sr, channels=ch, device=t['did'],
                callback=self._make_cb(t), blocksize=1024,
                dtype='float32', latency='low')
            t['stream'].start()
        except Exception as e:
            print(f"Error track {t['id']}: {e}")
    
    def _stop_stream(self, t):
        if t['stream']:
            t['stream'].stop()
            t['stream'].close()
            t['stream'] = None
    
    def play(self):
        has_audio = self.data is not None or any(t['data'] is not None for t in self.tracks)
        if not has_audio or not self.tracks:
            return
        self.playing = True
        for t in self.tracks:
            if t['data'] is None:  # global file tracks
                self._start_stream(t)
    
    def pause(self):
        self.playing = False
        for t in self.tracks:
            if t['data'] is None:
                self._stop_stream(t)
    
    def stop(self):
        self.playing = False
        for t in self.tracks:
            if t['data'] is None:
                self._stop_stream(t)
                t['peak'] = 0
                t['pos'] = 0
    
    # Per-track controls (for tracks with own file)
    def track_play(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if not t or t['data'] is None:
            return
        t['playing'] = True
        self._start_stream(t)
    
    def track_pause(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if not t:
            return
        t['playing'] = False
        self._stop_stream(t)
    
    def track_stop(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if not t:
            return
        t['playing'] = False
        self._stop_stream(t)
        t['peak'] = 0
        t['pos'] = 0
    
    def track_toggle_loop(self, tid):
        t = next((x for x in self.tracks if x['id'] == tid), None)
        if not t:
            return False
        t['tloop'] = not t['tloop']
        return t['tloop']
    
    def seek(self, pct):
        for t in self.tracks:
            audio = self._get_track_data(t)
            if audio is not None:
                t['pos'] = int(pct * len(audio))
    
    def get_state(self):
        has_audio = self.data is not None or any(t['data'] is not None for t in self.tracks)
        if not has_audio:
            return {'pos': 0, 'total': 0, 'playing': False, 'tracks': []}
        # Use global file for progress, or longest track file
        if self.data is not None:
            total = len(self.data)
            sr = self.sr
        else:
            longest = max(self.tracks, key=lambda t: len(t['data']) if t['data'] is not None else 0)
            total = len(longest['data']) if longest['data'] is not None else 0
            sr = longest['sr'] or 48000
        pos = max((t['pos'] for t in self.tracks), default=0)
        # Auto-stop/loop
        all_done = True
        for t in self.tracks:
            audio = self._get_track_data(t)
            if audio is not None and t['pos'] < len(audio):
                all_done = False
        if self.playing and all_done:
            if self.loop:
                for t in self.tracks:
                    t['pos'] = 0
            else:
                self.stop()
        tdata = []
        for t in self.tracks:
            audio = self._get_track_data(t)
            t_total = len(audio) if audio is not None else 0
            tdata.append({'id': t['id'], 'peak': t['peak'], 'mute': t['mute'],
                          'solo': t['solo'], 'fname': t.get('fname'),
                          'pos': t['pos'], 'total': t_total,
                          'tplaying': t.get('playing', False),
                          'tloop': t.get('tloop', False),
                          'has_own': t['data'] is not None})
            t['peak'] *= 0.82
        return {'pos': pos, 'total': total, 'sr': sr,
                'playing': self.playing, 'tracks': tdata}


# ─── JS API Bridge ───
class Api:
    def __init__(self, engine, window_ref):
        self.e = engine
        self.win_ref = window_ref
    
    def get_devices(self):
        return self.e.get_devices()
    
    def set_api(self, api):
        self.e.api = api
    
    def open_file(self):
        types = ('音频文件 (*.wav;*.mp3;*.flac;*.ogg;*.aiff)', '所有文件 (*.*)')
        result = self.win_ref[0].create_file_dialog(webview.OPEN_DIALOG, file_types=types)
        if result and len(result) > 0:
            info = self.e.load_file(result[0])
            return info
        return None
    
    def add_track(self, dev_id, dev_name):
        return self.e.add_track(dev_id, dev_name)
    
    def remove_track(self, tid):
        self.e.remove_track(tid)
    
    def open_track_file(self, tid):
        types = ('音频文件 (*.wav;*.mp3;*.flac;*.ogg;*.aiff)', '所有文件 (*.*)')
        result = self.win_ref[0].create_file_dialog(webview.OPEN_DIALOG, file_types=types)
        if result and len(result) > 0:
            return self.e.load_track_file(tid, result[0])
        return None
    
    def set_volume(self, tid, vol):
        self.e.set_volume(tid, vol)
    
    def toggle_mute(self, tid):
        return self.e.toggle_mute(tid)
    
    def toggle_solo(self, tid):
        return self.e.toggle_solo(tid)
    
    def play(self):
        self.e.play()
    
    def pause(self):
        self.e.pause()
    
    def stop(self):
        self.e.stop()
    
    def seek(self, pct):
        self.e.seek(pct)
    
    def toggle_loop(self):
        self.e.loop = not self.e.loop
        return self.e.loop
    
    def track_play(self, tid):
        self.e.track_play(tid)
    
    def track_pause(self, tid):
        self.e.track_pause(tid)
    
    def track_stop(self, tid):
        self.e.track_stop(tid)
    
    def track_toggle_loop(self, tid):
        return self.e.track_toggle_loop(tid)
    
    def resize_window(self, h):
        h = max(380, min(int(h) + 40, 900))  # clamp + title bar
        if self.win_ref[0]:
            try:
                self.win_ref[0].resize(660, h)
            except: pass
    
    def get_state(self):
        return self.e.get_state()


# ─── HTML UI ───
HTML = """<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body {
    font-family: 'Segoe UI', system-ui, sans-serif;
    background: #08090d;
    color: #e0e0e0;
    overflow-y: auto;
    user-select: none;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
}
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: #0c0e14; }
::-webkit-scrollbar-thumb { background: #1e2230; border-radius: 3px; }

/* ── Header ── */
.header {
    padding: 14px 20px 10px;
    background: #0c0e14;
}
.header h1 {
    font-size: 28px;
    font-weight: 900;
    letter-spacing: 2px;
    color: #fff;
}
.header h1 span { color: #E91E63; }
.accent-bar {
    height: 4px;
    background: linear-gradient(90deg, #E91E63, #E91E63 60%, #ff5281 100%);
}

/* ── Transport ── */
.transport {
    display: flex;
    align-items: center;
    padding: 16px 20px;
    gap: 10px;
    background: #0c0e14;
}
.btn-play {
    width: 56px; height: 56px;
    border-radius: 50%;
    border: none;
    background: #E91E63;
    color: white;
    font-size: 22px;
    cursor: pointer;
    transition: all .15s;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0 0 20px rgba(233,30,99,.3);
}
.btn-play:hover { transform: scale(1.08); box-shadow: 0 0 30px rgba(233,30,99,.5); }
.btn-play.paused { background: #FF9800; box-shadow: 0 0 20px rgba(255,152,0,.3); }
.btn-sm {
    width: 40px; height: 40px;
    border-radius: 50%;
    border: none;
    background: #1a1f2e;
    color: #555;
    font-size: 16px;
    cursor: pointer;
    transition: all .15s;
}
.btn-sm:hover { background: #252a3a; color: #888; }
.btn-sm.active { background: #E91E63; color: white; }
.btn-open {
    height: 40px;
    padding: 0 18px;
    border-radius: 20px;
    border: none;
    background: #1a1f2e;
    color: #777;
    font-size: 13px;
    font-weight: 700;
    cursor: pointer;
    letter-spacing: 1px;
    transition: all .15s;
}
.btn-open:hover { background: #252a3a; color: #aaa; }
.time {
    margin-left: auto;
    font-family: 'JetBrains Mono', 'Consolas', monospace;
    font-size: 26px;
    font-weight: 700;
    color: #444;
    letter-spacing: 1px;
}
.time.active { color: #e0e0e0; }

/* ── Progress ── */
.progress-wrap {
    padding: 0 20px 6px;
    background: #0c0e14;
    cursor: pointer;
}
.progress-bar {
    height: 8px;
    background: #12141c;
    border-radius: 4px;
    position: relative;
    overflow: hidden;
}
.progress-fill {
    height: 100%;
    background: linear-gradient(90deg, #E91E63, #ff5281);
    border-radius: 4px;
    width: 0%;
    transition: width 0.05s linear;
}

/* ── File info ── */
.file-info {
    padding: 4px 20px 10px;
    font-size: 13px;
    color: #333;
    background: #0c0e14;
    border-bottom: 1px solid #111420;
}
.file-info.loaded { color: #E91E63; }

/* ── Tracks ── */
.tracks-header {
    display: flex;
    align-items: center;
    padding: 12px 20px 6px;
}
.tracks-header h3 {
    font-size: 13px;
    font-weight: 900;
    letter-spacing: 2px;
    color: #333;
}
.btn-add {
    margin-left: auto;
    height: 32px;
    padding: 0 16px;
    border-radius: 16px;
    border: none;
    background: #00838f;
    color: white;
    font-size: 13px;
    font-weight: 700;
    cursor: pointer;
    transition: all .15s;
}
.btn-add:hover { background: #00acc1; transform: scale(1.04); }

.tracks-list {
    flex: 1;
    overflow-y: auto;
    padding: 4px 16px 12px;
    min-height: 0;
}
.track-card {
    display: flex;
    align-items: stretch;
    background: #0e1018;
    border-radius: 12px;
    margin-bottom: 6px;
    padding: 10px 14px;
    gap: 12px;
    transition: background .15s;
    flex-wrap: wrap;
}
.track-card:hover { background: #131720; }
.track-top { display: flex; align-items: center; gap: 12px; width: 100%; }
.track-transport {
    display: flex; align-items: center; gap: 4px;
    width: 100%; padding: 6px 0 0 17px;
}
.track-transport .tt-btn {
    width: 28px; height: 28px; border-radius: 50%;
    border: none; font-size: 12px; cursor: pointer;
    transition: all .12s; display: flex; align-items: center; justify-content: center;
}
.tt-play { background: #E91E63; color: white; }
.tt-play.on { background: #FF9800; }
.tt-stop { background: #1a1f2e; color: #555; }
.tt-stop:hover { color: #aaa; }
.tt-loop { background: #1a1f2e; color: #444; font-size: 14px; }
.tt-loop.on { background: #E91E63; color: white; }
.tt-progress {
    flex: 1; height: 4px; background: #12141c; border-radius: 2px;
    overflow: hidden; margin: 0 6px;
}
.tt-progress-fill { height: 100%; background: #E91E63; border-radius: 2px; width: 0%; }
.tt-time { font-size: 11px; color: #444; font-family: Consolas, monospace; min-width: 70px; text-align: right; }

.track-bar {
    width: 5px;
    height: 44px;
    border-radius: 3px;
    flex-shrink: 0;
}
.track-info {
    flex: 1;
    min-width: 0;
}
.track-name {
    font-size: 14px;
    font-weight: 600;
    color: #ccc;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}
.track-meter {
    height: 4px;
    background: #12141c;
    border-radius: 2px;
    margin-top: 6px;
    overflow: hidden;
}
.track-meter-fill {
    height: 100%;
    border-radius: 2px;
    width: 0%;
    transition: width 0.05s, background-color 0.2s;
}

.track-controls {
    display: flex;
    align-items: center;
    gap: 6px;
    flex-shrink: 0;
}
.track-vol {
    -webkit-appearance: none;
    width: 70px;
    height: 4px;
    border-radius: 2px;
    background: #1a1f2e;
    outline: none;
    cursor: pointer;
}
.track-vol::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 14px; height: 14px;
    border-radius: 50%;
    background: white;
    cursor: pointer;
    box-shadow: 0 0 4px rgba(0,0,0,.5);
}
.tbtn {
    width: 30px; height: 30px;
    border-radius: 8px;
    border: none;
    font-size: 12px;
    font-weight: 800;
    cursor: pointer;
    transition: all .12s;
}
.tbtn-m { background: #1a1f2e; color: #555; }
.tbtn-m.on { background: #FF9800; color: #000; }
.tbtn-s { background: #1a1f2e; color: #555; }
.tbtn-s.on { background: #4CAF50; color: #fff; }
.tbtn-x { background: #1a1f2e; color: #333; }
.tbtn-x:hover { background: #3a1515; color: #f44; }

.track-file-row {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-top: 3px;
}
.tbtn-file {
    width: 22px; height: 22px;
    border: none;
    background: transparent;
    cursor: pointer;
    font-size: 12px;
    padding: 0;
    opacity: 0.5;
    transition: opacity .15s;
}
.tbtn-file:hover { opacity: 1; }
.track-file-name {
    font-size: 11px;
    color: #444;
    font-style: italic;
}
.track-file-name.has-file {
    color: #E91E63;
    font-style: normal;
    font-weight: 600;
}

/* ── API selector ── */
.api-bar {
    display: flex;
    gap: 4px;
    padding: 0 20px 8px;
    background: #0c0e14;
}
.api-pill {
    padding: 4px 12px;
    border-radius: 12px;
    border: 1px solid #1a1f2e;
    background: transparent;
    color: #444;
    font-size: 11px;
    font-weight: 700;
    cursor: pointer;
    transition: all .15s;
}
.api-pill.active { border-color: #E91E63; color: #E91E63; background: rgba(233,30,99,.08); }

/* ── Device picker modal ── */
.modal-bg {
    display: none;
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,.7);
    z-index: 100;
    align-items: center;
    justify-content: center;
}
.modal-bg.show { display: flex; }
.modal {
    background: #0e1018;
    border-radius: 16px;
    padding: 20px;
    width: 520px;
    max-height: 420px;
    display: flex;
    flex-direction: column;
    box-shadow: 0 20px 60px rgba(0,0,0,.5);
    border: 1px solid #1a1f2e;
}
.modal h3 {
    font-size: 16px;
    font-weight: 800;
    color: #888;
    letter-spacing: 1px;
    margin-bottom: 12px;
}
.modal-list {
    flex: 1;
    overflow-y: auto;
}
.dev-item {
    padding: 10px 12px;
    border-radius: 8px;
    cursor: pointer;
    font-size: 13px;
    color: #aaa;
    transition: all .12s;
}
.dev-item:hover { background: #1a1f2e; color: #fff; }
.dev-item.used { color: #444; }
.modal-close {
    margin-top: 12px;
    align-self: flex-end;
    padding: 6px 20px;
    border-radius: 12px;
    border: none;
    background: #1a1f2e;
    color: #666;
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
}
</style>
</head>
<body>

<div class="header">
    <h1><span>A/B</span> 播放器</h1>
</div>
<div class="accent-bar"></div>

<div class="transport">
    <button class="btn-play" id="playBtn" onclick="togglePlay()">▶</button>
    <button class="btn-sm" onclick="doStop()">■</button>
    <button class="btn-sm" id="loopBtn" onclick="toggleLoop()">↻</button>
    <button class="btn-open" onclick="openFile()">打开</button>
    <div class="time" id="timeDisplay">0:00 / 0:00</div>
</div>

<div class="progress-wrap" onmousedown="seekStart(event)" onmousemove="seekMove(event)" onmouseup="seekEnd()">
    <div class="progress-bar">
        <div class="progress-fill" id="progFill"></div>
    </div>
</div>

<div class="file-info" id="fileInfo">未加载文件 — 点击「打开」或按 O</div>

<div class="api-bar">
    <button class="api-pill" onclick="setApi('WASAPI',this)">WASAPI</button>
    <button class="api-pill active" onclick="setApi('WDM-KS',this)">WDM-KS</button>
    <button class="api-pill" onclick="setApi('ALL',this)">ALL</button>
</div>

<div class="tracks-header">
    <h3>输出通道</h3>
    <button class="btn-add" onclick="showAddModal()">+ 添加</button>
</div>

<div class="tracks-list" id="tracksList"></div>

<!-- Device picker modal -->
<div class="modal-bg" id="modalBg" onclick="closeModal(event)">
    <div class="modal" onclick="event.stopPropagation()">
        <h3>选择输出设备</h3>
        <div class="modal-list" id="modalList"></div>
        <button class="modal-close" onclick="document.getElementById('modalBg').classList.remove('show')">取消</button>
    </div>
</div>

<script>
const COLORS = ['#E91E63','#00E5FF','#FFD600','#4CAF50','#9C27B0','#FF5722','#2196F3','#FF4081'];
let tracks = [];
let fileLoaded = false;
let seeking = false;

async function openFile() {
    const info = await pywebview.api.open_file();
    if (info) {
        fileLoaded = true;
        const m = Math.floor(info.duration / 60);
        const s = Math.floor(info.duration % 60);
        document.getElementById('fileInfo').textContent = 
            info.name + '  •  ' + m + ':' + String(s).padStart(2,'0') + '  •  ' + info.sr + 'Hz  •  ' + info.ch + 'ch';
        document.getElementById('fileInfo').classList.add('loaded');
    }
}

async function togglePlay() {
    const btn = document.getElementById('playBtn');
    const st = await pywebview.api.get_state();
    if (st.playing) {
        await pywebview.api.pause();
        btn.textContent = '▶';
        btn.classList.remove('paused');
    } else {
        await pywebview.api.play();
        btn.textContent = '⏸';
        btn.classList.add('paused');
    }
}

async function doStop() {
    await pywebview.api.stop();
    document.getElementById('playBtn').textContent = '▶';
    document.getElementById('playBtn').classList.remove('paused');
}

async function toggleLoop() {
    const on = await pywebview.api.toggle_loop();
    document.getElementById('loopBtn').classList.toggle('active', on);
}

function setApi(api, el) {
    document.querySelectorAll('.api-pill').forEach(p => p.classList.remove('active'));
    el.classList.add('active');
    pywebview.api.set_api(api);
}

function seekStart(e) { seeking = true; doSeek(e); }
function seekMove(e) { if (seeking) doSeek(e); }
function seekEnd() { seeking = false; }
function doSeek(e) {
    const rect = e.currentTarget.getBoundingClientRect();
    const pct = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
    pywebview.api.seek(pct);
}

async function showAddModal() {
    const devs = await pywebview.api.get_devices();
    const usedIds = tracks.map(t => t.did);
    const list = document.getElementById('modalList');
    list.innerHTML = '';
    devs.forEach(d => {
        const div = document.createElement('div');
        div.className = 'dev-item' + (usedIds.includes(d.id) ? ' used' : '');
        div.textContent = (usedIds.includes(d.id) ? '●  ' : '') + d.full;
        div.onclick = async () => {
            const tid = await pywebview.api.add_track(d.id, d.full);
            tracks.push({id: tid, did: d.id, name: d.full});
            renderTracks();
            autoResize();
            document.getElementById('modalBg').classList.remove('show');
        };
        list.appendChild(div);
    });
    document.getElementById('modalBg').classList.add('show');
}

function closeModal(e) {
    if (e.target.id === 'modalBg') e.target.classList.remove('show');
}

function renderTracks() {
    const el = document.getElementById('tracksList');
    el.innerHTML = '';
    tracks.forEach((t, i) => {
        const c = COLORS[i % COLORS.length];
        const hasOwn = !!t.fname;
        const transport = hasOwn ? `
            <div class="track-transport">
                <button class="tt-btn tt-play" id="tplay-${t.id}" onclick="tToggle(${t.id})">▶</button>
                <button class="tt-btn tt-stop" onclick="tStop(${t.id})">■</button>
                <button class="tt-btn tt-loop" id="tloop-${t.id}" onclick="tLoop(${t.id})">↻</button>
                <div class="tt-progress"><div class="tt-progress-fill" id="tprog-${t.id}"></div></div>
                <span class="tt-time" id="ttime-${t.id}">0:00</span>
            </div>` : '';
        el.innerHTML += `
        <div class="track-card" id="card-${t.id}">
          <div class="track-top">
            <div class="track-bar" style="background:${c}"></div>
            <div class="track-info">
                <div class="track-name">${t.name}</div>
                <div class="track-file-row">
                    <button class="tbtn-file" onclick="loadTrackFile(${t.id})" title="为此轨道选择音频文件">📂</button>
                    <span class="track-file-name ${hasOwn?'has-file':''}" id="fname-${t.id}">${t.fname || '使用全局文件'}</span>
                </div>
                <div class="track-meter"><div class="track-meter-fill" id="meter-${t.id}"></div></div>
            </div>
            <div class="track-controls">
                <input type="range" class="track-vol" min="0" max="100" value="100"
                    oninput="pywebview.api.set_volume(${t.id}, this.value/100)"
                    style="accent-color:${c}">
                <button class="tbtn tbtn-m" id="mute-${t.id}" onclick="doMute(${t.id})">M</button>
                <button class="tbtn tbtn-s" id="solo-${t.id}" onclick="doSolo(${t.id})">S</button>
                <button class="tbtn tbtn-x" onclick="doRemove(${t.id})">✕</button>
            </div>
          </div>
          ${transport}
        </div>`;
    });
}

async function loadTrackFile(tid) {
    try {
        console.log('loadTrackFile called, tid=', tid);
        const info = await pywebview.api.open_track_file(tid);
        console.log('open_track_file returned:', info);
        if (info) {
            const t = tracks.find(x => x.id === tid);
            if (t) t.fname = info.name;
            renderTracks();
            autoResize();
        }
    } catch(e) {
        console.error('loadTrackFile error:', e);
        alert('Error: ' + e);
    }
}

async function tToggle(tid) {
    const st = await pywebview.api.get_state();
    const ts = st.tracks.find(x => x.id === tid);
    if (ts && ts.tplaying) {
        await pywebview.api.track_pause(tid);
    } else {
        await pywebview.api.track_play(tid);
    }
}
async function tStop(tid) { await pywebview.api.track_stop(tid); }
async function tLoop(tid) {
    const on = await pywebview.api.track_toggle_loop(tid);
    const el = document.getElementById('tloop-'+tid);
    if (el) el.classList.toggle('on', on);
}

async function doMute(tid) {
    const on = await pywebview.api.toggle_mute(tid);
    document.getElementById('mute-'+tid).classList.toggle('on', on);
}
async function doSolo(tid) {
    const on = await pywebview.api.toggle_solo(tid);
    document.getElementById('solo-'+tid).classList.toggle('on', on);
}
async function doRemove(tid) {
    await pywebview.api.remove_track(tid);
    tracks = tracks.filter(t => t.id !== tid);
    renderTracks();
    autoResize();
}

// ── Polling loop for meters & progress ──
async function tick() {
    try {
        const s = await pywebview.api.get_state();
        if (s.total > 0) {
            const pct = (s.pos / s.total * 100).toFixed(2);
            document.getElementById('progFill').style.width = pct + '%';
            const cur = s.pos / s.sr;
            const tot = s.total / s.sr;
            const m1 = Math.floor(cur/60), s1 = Math.floor(cur%60);
            const m2 = Math.floor(tot/60), s2 = Math.floor(tot%60);
            const td = document.getElementById('timeDisplay');
            td.textContent = m1+':'+String(s1).padStart(2,'0')+' / '+m2+':'+String(s2).padStart(2,'0');
            td.classList.toggle('active', s.playing);
            
            const pb = document.getElementById('playBtn');
            if (!s.playing && pb.classList.contains('paused')) {
                pb.textContent = '▶';
                pb.classList.remove('paused');
            }
        }
        if (s.tracks) {
            s.tracks.forEach(t => {
                const mel = document.getElementById('meter-'+t.id);
                if (mel) {
                    const pw = Math.min(100, t.peak * 100);
                    mel.style.width = pw + '%';
                    mel.style.backgroundColor = t.peak < 0.7 ? '#4CAF50' : (t.peak < 0.9 ? '#FFD600' : '#f44336');
                }
                // Per-track transport
                if (t.has_own) {
                    const pb = document.getElementById('tplay-'+t.id);
                    if (pb) {
                        pb.textContent = t.tplaying ? '⏸' : '▶';
                        pb.classList.toggle('on', t.tplaying);
                    }
                    if (t.total > 0) {
                        const pf = document.getElementById('tprog-'+t.id);
                        if (pf) pf.style.width = (t.pos/t.total*100)+'%';
                        const tt = document.getElementById('ttime-'+t.id);
                        if (tt) {
                            const cs = t.pos / (s.sr || 48000);
                            const ts2 = t.total / (s.sr || 48000);
                            const m1=Math.floor(cs/60), s1=Math.floor(cs%60);
                            const m2=Math.floor(ts2/60), s2=Math.floor(ts2%60);
                            tt.textContent = m1+':'+String(s1).padStart(2,'0')+'/'+m2+':'+String(s2).padStart(2,'0');
                        }
                    }
                }
            });
        }
    } catch(e) {}
    setTimeout(tick, 40);
}

function autoResize() {
    setTimeout(() => {
        const h = document.body.scrollHeight;
        pywebview.api.resize_window(h);
    }, 50);
}

// ── Init: auto-add default tracks ──
async function init() {
    const devs = await pywebview.api.get_devices();
    const v = devs.find(d => d.name.includes('Virtual 1/2'));
    const o = devs.find(d => d !== v);
    if (v) { const tid = await pywebview.api.add_track(v.id, v.full); tracks.push({id:tid,did:v.id,name:v.full}); }
    if (o) { const tid = await pywebview.api.add_track(o.id, o.full); tracks.push({id:tid,did:o.id,name:o.full}); }
    renderTracks();
    autoResize();
    tick();
}

window.addEventListener('pywebviewready', init);
document.addEventListener('keydown', e => {
    if (e.code === 'Space') { e.preventDefault(); togglePlay(); }
    if (e.code === 'Escape') doStop();
    if (e.key === 'o') openFile();
    if (e.key === 'l') toggleLoop();
});
</script>
</body>
</html>
"""

def main():
    engine = Engine()
    win_ref = [None]
    api = Api(engine, win_ref)
    
    window = webview.create_window(
        'A/B 播放器', html=HTML,
        width=660, height=600, min_size=(500, 380),
        resizable=True,
        js_api=api, background_color='#08090d'
    )
    win_ref[0] = window
    webview.start(debug=False)

if __name__ == '__main__':
    main()
