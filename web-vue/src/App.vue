<template>
  <main class="wrap">
    <header><h1>BMS-DASH</h1><span :class="['dot',connected?'ok':'']">{{connected?'WebSocket 已连接':'离线'}}</span></header>
    <nav><button @click="tab='dash'">仪表盘</button><button @click="tab='bms'">电池</button><button @click="tab='set'">设置</button></nav>

    <section v-if="tab==='dash'" class="grid">
      <article class="card full"><small>SOC</small><strong>{{num(data.soc,0)}}%</strong><div class="bar"><i :style="{width:(data.soc||0)+'%'}"></i></div></article>
      <article class="card"><small>总电压</small><strong>{{num(data.voltage,1)}} <em>V</em></strong></article>
      <article class="card"><small>电流</small><strong>{{num(data.current,1)}} <em>A</em></strong></article>
      <article class="card"><small>功率</small><strong>{{num(data.power,0)}} <em>W</em></strong></article>
      <article class="card"><small>MOS温度</small><strong>{{num(data.mos_temperature,1)}} <em>°C</em></strong></article>
      <article class="card"><small>剩余容量</small><strong>{{num(data.remaining_capacity,1)}} <em>Ah</em></strong></article>
      <article class="card"><small>压差</small><strong>{{num(data.delta_cell_mv,0)}} <em>mV</em></strong></article>
      <article class="card full"><small>当前电池</small><div>{{data.name||'未连接'}} {{data.selected_bms||''}}</div><small>状态：{{data.state||'--'}}</small></article>
    </section>

    <section v-else-if="tab==='bms'">
      <article class="card">
        <div class="actions"><button @click="send('SCAN_BMS')">扫描电池</button><button @click="send('RECONNECT_BMS')">重新连接</button><button @click="send('FORGET_BMS')">清除保存</button></div>
        <div v-for="x in scan" :key="x.mac" class="cell"><b>{{x.name||'未命名'}}</b><br>{{x.mac}}　RSSI {{x.rssi}} <button @click="send('SELECT_BMS:'+x.mac)">选择</button></div>
      </article>
    </section>

    <section v-else>
      <article class="card actions"><button @click="send('GET_STATUS')">读取状态</button><button @click="send('REBOOT')">重启 ESP32</button></article>
      <article class="card"><small>Wi-Fi：BMS-DASH　密码：12345678　网页：http://192.168.4.1</small></article>
    </section>
  </main>
</template>

<script setup>
import { onMounted, onBeforeUnmount, reactive, ref } from 'vue'
const tab=ref('dash'), connected=ref(false), scan=ref([])
const data=reactive({soc:null,voltage:null,current:null,power:null,mos_temperature:null,remaining_capacity:null,delta_cell_mv:null,name:'',selected_bms:'',state:''})
let ws
const num=(v,d)=>typeof v==='number'?v.toFixed(d):'--'
function send(c){if(ws&&ws.readyState===1)ws.send(c)}
function onMessage(e){
  let m; try{m=JSON.parse(e.data)}catch{return}
  if(m.type==='status')Object.assign(data,m)
  if(m.type==='scan_start')scan.value=[]
  if(m.type==='bms_found')scan.value.push(m)
}
function connect(){
  ws=new WebSocket('ws://'+location.hostname+':81/')
  ws.onopen=()=>{connected.value=true;send('GET_STATUS')}
  ws.onclose=()=>{connected.value=false;setTimeout(connect,1500)}
  ws.onmessage=onMessage
}
onMounted(connect)
onBeforeUnmount(()=>ws&&ws.close())
</script>