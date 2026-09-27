#include <pgmspace.h>

static const char WEB_INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><title>BMS-DASH</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#030a10;color:#e6ebf0;font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif}button{border:1px solid #26414e;background:#0c1b26;color:#e6ebf0;border-radius:10px;padding:10px 14px;font-size:14px}button:active{transform:scale(.98)}.wrap{max-width:760px;margin:auto;padding:14px}.top{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}.title{font-size:20px;font-weight:700}.dot{display:inline-block;width:9px;height:9px;border-radius:50%;background:#ff2f3d;margin-right:6px}.ok{background:#19eb5c}.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}.card{background:#08141d;border:1px solid #26414e;border-radius:14px;padding:14px}.big{font-size:32px;font-weight:700}.label{font-size:12px;color:#87919a}.unit{font-size:14px;color:#87919a}.full{grid-column:1/-1}.cells{display:grid;grid-template-columns:repeat(2,1fr);gap:7px;margin-top:8px}.cell{background:#0c1b26;padding:8px;border-radius:9px}.actions{display:flex;flex-wrap:wrap;gap:8px}.status{font-size:12px;color:#87919a;line-height:1.6}.nav{display:flex;gap:8px;margin:12px 0}.hidden{display:none}.bar{height:8px;background:#1a2932;border-radius:8px;overflow:hidden}.bar i{display:block;height:100%;background:#19eb5c;width:0}.log{font-family:monospace;font-size:11px;white-space:pre-wrap;max-height:140px;overflow:auto;color:#87919a}@media(min-width:600px){.cells{grid-template-columns:repeat(4,1fr)}}
</style></head><body><div class="wrap">
<div class="top"><div class="title">BMS-DASH <span id="conn"><span class="dot"></span>离线</span></div><button onclick="refresh()">刷新</button></div>
<div class="nav"><button onclick="tab('dash')">仪表盘</button><button onclick="tab('bms')">电池</button><button onclick="tab('set')">设置</button></div>
<section id="dash"><div class="grid">
<div class="card full"><div class="label">SOC</div><div class="big" id="soc">--%</div><div class="bar"><i id="socbar"></i></div></div>
<div class="card"><div class="label">总电压</div><div class="big" id="voltage">--</div><span class="unit">V</span></div>
<div class="card"><div class="label">电流</div><div class="big" id="current">--</div><span class="unit">A</span></div>
<div class="card"><div class="label">功率</div><div class="big" id="power">--</div><span class="unit">W</span></div>
<div class="card"><div class="label">MOS温度</div><div class="big" id="temp">--</div><span class="unit">°C</span></div>
<div class="card"><div class="label">剩余容量</div><div class="big" id="remain">--</div><span class="unit">Ah</span></div>
<div class="card"><div class="label">压差</div><div class="big" id="delta">--</div><span class="unit">mV</span></div>
<div class="card full"><div class="label">当前电池</div><div id="bms">未连接</div><div class="status" id="state">状态：--</div></div>
</div></section>
<section id="bms" class="hidden"><div class="card"><div class="actions"><button onclick="cmd('SCAN_BMS')">扫描电池</button><button onclick="cmd('RECONNECT_BMS')">重新连接</button><button onclick="cmd('FORGET_BMS')">清除保存</button></div><div id="list" class="status" style="margin-top:12px">等待扫描…</div></div></section>
<section id="set" class="hidden"><div class="card"><div class="actions"><button onclick="cmd('GET_STATUS')">读取状态</button><button onclick="cmd('REBOOT')">重启 ESP32</button></div><p class="status">Wi-Fi：BMS-DASH<br>密码：12345678<br>网页：192.168.4.1</p></div></section>
<div class="card full" style="margin-top:10px"><div class="label">通信日志</div><div id="log" class="log"></div></div>
</div>
<script>
let ws,scan=[];
const $=id=>document.getElementById(id);
function log(s){$('log').textContent=(new Date().toLocaleTimeString())+' '+s+'\n'+$('log').textContent.slice(0,3500)}
function tab(x){['dash','bms','set'].forEach(id=>$(id).classList.toggle('hidden',id!==x))}
function connect(){ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onopen=()=>{$('conn').innerHTML='<span class="dot ok"></span>已连接';log('WebSocket connected');cmd('GET_STATUS')};
ws.onclose=()=>{$('conn').innerHTML='<span class="dot"></span>离线';setTimeout(connect,1500)};
ws.onerror=()=>log('WebSocket error');
ws.onmessage=e=>{log('RX '+e.data);try{msg(JSON.parse(e.data))}catch(_){}}
}
function cmd(c){if(ws&&ws.readyState===1)ws.send(c);else log('未连接')}
function refresh(){cmd('GET_STATUS')}
function val(v,d=1){return typeof v==='number'?v.toFixed(d):'--'}
function msg(m){
 if(m.type==='status'){ $('soc').textContent=m.soc==null?'--%':m.soc+'%';$('socbar').style.width=(m.soc||0)+'%';$('voltage').textContent=val(m.voltage,1);$('current').textContent=val(m.current,1);$('power').textContent=val(m.power,0);$('temp').textContent=val(m.mos_temperature,1);$('remain').textContent=val(m.remaining_capacity,1);$('delta').textContent=val(m.delta_cell_mv,0);$('bms').textContent=(m.name||'未命名')+' '+(m.selected_bms||'');$('state').textContent='状态：'+(m.state||'--');}
 if(m.type==='scan_start'){scan=[];$('list').innerHTML='扫描中…'}
 if(m.type==='bms_found'){scan.push(m);$('list').innerHTML=scan.map(x=>'<div class="cell"><b>'+esc(x.name||'未命名')+'</b><br>'+esc(x.mac)+'<br>RSSI '+x.rssi+' <button onclick="cmd(\'SELECT_BMS:'+x.mac+'\')">选择</button></div>').join('')}
 if(m.type==='scan_done')log('扫描完成');
 if(m.type==='error')log('ERROR '+m.message);
}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
connect();
</script></body></html>)rawliteral";
