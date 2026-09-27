#include "WebConfig.h"
#include <WiFi.h>
#include <Preferences.h>

WebConfig::WebConfig():server_(80),ble_(nullptr),active_(false){}

String WebConfig::jsonEscape(const String& s){
  String o;
  for(size_t i=0;i<s.length();i++){
    char c=s[i];
    if(c=='"') o+="\\"";
    else if(c=='\\') o+="\\\\";
    else if(c=='\n') o+="\\n";
    else o+=c;
  }
  return o;
}

String WebConfig::makeStatusJson(){
  String j="{";
  j+=""online":"+String(g_bmsData.online?"true":"false");
  j+=","state":"+String((int)g_bmsData.bootState);
  j+=","message":""+jsonEscape(g_bmsData.statusMessage)+""";
  j+=","mac":""+jsonEscape(g_bmsData.mac)+""";
  j+=","name":""+jsonEscape(g_bmsData.deviceName)+""";
  j+=","ip":""+jsonEscape(WiFi.softAPIP().toString())+""";
  j+=","scanCount":"+String(ble_?ble_->getScanCount():0);
  j+=","scanAttempt":"+String(g_bmsData.scanAttempt);
  j+=","voltage":"+String(g_bmsData.totalVoltage,3);
  j+=","current":"+String(g_bmsData.current,3);
  j+=","soc":"+String(g_bmsData.soc,1);
  j+="}";
  return j;
}

String WebConfig::makePage(){
  return R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>JK BMS 设置</title>
<style>
body{font-family:Arial,sans-serif;background:#101418;color:#eee;margin:0;padding:16px}
.card{max-width:720px;margin:auto;background:#1b2229;border-radius:14px;padding:18px}
h2{margin-top:0}button{padding:10px 14px;margin:5px;border:0;border-radius:8px;background:#1976d2;color:white}
input,select{padding:10px;margin:5px 0;width:100%;box-sizing:border-box;border-radius:7px;border:1px solid #555;background:#0f1317;color:#fff}
.item{padding:10px;border:1px solid #3a444e;border-radius:8px;margin:8px 0}
.small{color:#aeb8c2;font-size:13px}.ok{color:#4caf50}.warn{color:#ffc107}
</style>
</head>
<body>
<div class="card">
<h2>JK BMS 蓝牙设置</h2>
<div id="status" class="small">读取状态...</div>

<label>保护板 MAC（可留空自动扫描）</label>
<input id="mac" placeholder="例如 AA:BB:CC:DD:EE:FF">

<label>协议型号</label>
<select id="proto">
<option value="32">JK02_32S</option>
<option value="24">JK02_24S</option>
</select>

<button onclick="scan()">扫描蓝牙电池</button>
<button onclick="save()">保存参数</button>
<div id="list"></div>

<hr>
<div id="live"></div>
</div>

<script>
async function api(url,opt){return await (await fetch(url,opt)).json();}
async function status(){
  try{
    let s=await api('/api/status');
    document.getElementById('status').innerHTML=
      '<b class="'+(s.online?'ok':'warn')+'">'+
      (s.online?'已连接':'未连接')+'</b>　'+s.message+
      '<br>MAC: '+(s.mac||'未设置')+'　IP: '+s.ip;
    document.getElementById('live').innerHTML=
      '电压 '+s.voltage.toFixed(2)+' V　电流 '+s.current.toFixed(2)+
      ' A　SOC '+s.soc.toFixed(0)+'%';
  }catch(e){}
}
async function scan(){
  document.getElementById('list').innerHTML='正在扫描，请等待...';
  let r=await api('/api/scan');
  let h='<h3>扫描结果</h3>';
  if(!r.items.length) h+='没有找到JK/BMS设备';
  r.items.forEach((x,i)=>{
    h+='<div class="item"><b>'+x.name+'</b><br>'+x.address+
       '　RSSI '+x.rssi+' dBm<br>'+
       '<button onclick="connectTo('+i+')">连接此电池</button></div>';
  });
  document.getElementById('list').innerHTML=h;
}
async function connectTo(i){
  let r=await api('/api/connect?index='+i);
  alert(r.message);
  status();
}
async function save(){
  let fd=new FormData();
  fd.append('mac',document.getElementById('mac').value);
  fd.append('proto',document.getElementById('proto').value);
  let r=await api('/api/save',{method:'POST',body:fd});
  alert(r.message);
  status();
}
status();
setInterval(status,1000);
</script>
</body></html>)HTML";
}

void WebConfig::begin(JkBle* ble){
  ble_=ble;
  active_=true;

  server_.on("/",HTTP_GET,[this](){handleRoot();});
  server_.on("/api/status",HTTP_GET,[this](){handleStatus();});
  server_.on("/api/scan",HTTP_GET,[this](){handleScan();});
  server_.on("/api/connect",HTTP_GET,[this](){handleConnect();});
  server_.on("/api/save",HTTP_POST,[this](){handleSave();});
  server_.onNotFound([this](){handleNotFound();});
  server_.begin();
}

void WebConfig::loop(){
  if(active_) server_.handleClient();
}

void WebConfig::handleRoot(){
  server_.send(200,"text/html; charset=utf-8",makePage());
}

void WebConfig::handleStatus(){
  server_.send(200,"application/json; charset=utf-8",makeStatusJson());
}

void WebConfig::handleScan(){
  if(!ble_){
    server_.send(500,"application/json","{"message":"BLE未初始化","items":[]}");
    return;
  }

  uint8_t count=ble_->scanDevices(5);
  String j="{"message":"扫描完成","items":[";
  for(uint8_t i=0;i<count;i++){
    if(i) j+=",";
    const JkScanItem& x=ble_->getScanItem(i);
    j+="{"name":""+jsonEscape(x.name)+"","address":""+
      jsonEscape(x.address)+"","rssi":"+String(x.rssi)+"}";
  }
  j+="]}";
  server_.send(200,"application/json; charset=utf-8",j);
}

void WebConfig::handleConnect(){
  if(!ble_){
    server_.send(500,"application/json","{"message":"BLE未初始化"}");
    return;
  }
  if(!server_.hasArg("index")){
    server_.send(400,"application/json","{"message":"缺少index"}");
    return;
  }
  int index=server_.arg("index").toInt();
  bool ok=ble_->connectDeviceByIndex((uint8_t)index);
  String j="{"ok":" + String(ok?"true":"false") +
           ","message":""+String(ok?"连接成功":"连接失败")+""}";
  server_.send(ok?200:500,"application/json; charset=utf-8",j);
}

void WebConfig::handleSave(){
  if(!ble_){
    server_.send(500,"application/json","{"message":"BLE未初始化"}");
    return;
  }

  String mac=server_.hasArg("mac")?server_.arg("mac"):"";
  mac.trim();
  bool is32=server_.hasArg("proto") ? server_.arg("proto")=="32" : true;

  ble_->setConfiguredAddress(mac);
  ble_->setProtocol32S(is32);

  Preferences p;
  p.begin("jkcfg",false);
  p.putBool("32s",is32);
  p.end();

  String j="{"message":"参数已保存，下次开机自动使用"}";
  server_.send(200,"application/json; charset=utf-8",j);
}

void WebConfig::handleNotFound(){
  server_.send(404,"text/plain; charset=utf-8","404");
}
