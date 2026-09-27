<template>
  <main class="wrap">
    <header class="topbar">
      <div>
        <h1>BMS-DASH</h1>
        <p>ANT BMS · ESP32 · Vue 3</p>
      </div>
      <span :class="['dot', connected ? 'ok' : '']">
        {{ connected ? 'WebSocket 已连接' : '等待连接' }}
      </span>
    </header>

    <nav class="tabs">
      <button :class="{active: tab === 'dash'}" @click="tab = 'dash'">仪表盘</button>
      <button :class="{active: tab === 'bms'}" @click="tab = 'bms'">电池</button>
      <button :class="{active: tab === 'set'}" @click="tab = 'set'">设置</button>
    </nav>

    <section v-if="tab === 'dash'" class="grid">
      <article class="card full soc-card">
        <div class="row"><small>SOC</small><strong>{{ num(data.soc, 0) }}%</strong></div>
        <div class="bar"><i :style="{width: socWidth + '%'}"></i></div>
      </article>

      <article class="card metric">
        <small>总电压</small>
        <strong>{{ num(data.voltage, 1) }} <em>V</em></strong>
      </article>
      <article class="card metric">
        <small>电流</small>
        <strong>{{ num(data.current, 1) }} <em>A</em></strong>
      </article>
      <article class="card metric">
        <small>功率</small>
        <strong>{{ num(data.power, 0) }} <em>W</em></strong>
      </article>
      <article class="card metric">
        <small>MOS 温度</small>
        <strong>{{ num(data.mos_temperature, 1) }} <em>°C</em></strong>
      </article>
      <article class="card metric">
        <small>剩余容量</small>
        <strong>{{ num(data.remaining_capacity, 1) }} <em>Ah</em></strong>
      </article>
      <article class="card metric">
        <small>压差</small>
        <strong>{{ num(data.delta_cell_mv, 0) }} <em>mV</em></strong>
      </article>

      <article class="card full device-card">
        <div><small>当前电池</small><b>{{ data.name || '未连接' }}</b></div>
        <div><small>MAC</small><span>{{ data.selected_bms || '--' }}</span></div>
        <div><small>ESP32 状态</small><span>{{ data.state || '--' }}</span></div>
        <div><small>RSSI</small><span>{{ data.rssi ?? '--' }} dBm</span></div>
      </article>
    </section>

    <section v-else-if="tab === 'bms'">
      <article class="card">
        <div class="actions">
          <button @click="send('SCAN_BMS')">扫描电池</button>
          <button @click="send('RECONNECT_BMS')">重新连接</button>
          <button @click="send('FORGET_BMS')">清除保存</button>
        </div>

        <p class="hint">{{ scan.length ? '扫描结果' : '点击“扫描电池”搜索附近 ANT BMS' }}</p>
        <div v-for="x in scan" :key="x.mac" class="cell">
          <div>
            <b>{{ x.name || '未命名 BMS' }}</b>
            <span>{{ x.mac }}</span>
            <span>RSSI {{ x.rssi }}</span>
          </div>
          <button @click="send('SELECT_BMS:' + x.mac)">选择</button>
        </div>
      </article>
    </section>

    <section v-else>
      <article class="card actions">
        <button @click="send('PING')">PING</button>
        <button @click="send('GET_STATUS')">读取状态</button>
        <button @click="send('REBOOT')">重启 ESP32</button>
      </article>

      <article class="card">
        <h2>网络</h2>
        <p>Wi-Fi：<b>BMS-DASH</b></p>
        <p>密码：<b>12345678</b></p>
        <p>网页：<b>http://192.168.4.1/</b></p>
        <p>WebSocket：<b>ws://192.168.4.1:81/</b></p>
      </article>

      <article class="card">
        <h2>固件 / 文件系统</h2>
        <p>Firmware：{{ data.firmware || '--' }}</p>
        <p>LittleFS：{{ fs.ready ? '正常' : '未检测' }}</p>
        <p v-if="fs.total">网页空间：{{ bytes(fs.used) }} / {{ bytes(fs.total) }}</p>
        <button @click="loadFsInfo">读取 LittleFS 状态</button>
      </article>
    </section>

    <footer>{{ lastMessage }}</footer>
  </main>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref } from 'vue'

const tab = ref('dash')
const connected = ref(false)
const scan = ref([])
const lastMessage = ref('正在连接 ESP32 WebSocket...')
const fs = reactive({ ready: false, total: 0, used: 0 })

const data = reactive({
  soc: null,
  voltage: null,
  current: null,
  power: null,
  mos_temperature: null,
  total_capacity: null,
  remaining_capacity: null,
  delta_cell_mv: null,
  max_cell_voltage: null,
  min_cell_voltage: null,
  rssi: null,
  name: '',
  selected_bms: '',
  state: '',
  firmware: ''
})

let ws = null
let reconnectTimer = null

const socWidth = computed(() => {
  const value = Number(data.soc)
  return Number.isFinite(value) ? Math.max(0, Math.min(100, value)) : 0
})

function num(value, decimals) {
  return typeof value === 'number' && Number.isFinite(value) ? value.toFixed(decimals) : '--'
}

function bytes(value) {
  const n = Number(value)
  if (!Number.isFinite(n)) return '--'
  if (n < 1024) return n + ' B'
  if (n < 1024 * 1024) return (n / 1024).toFixed(1) + ' KB'
  return (n / 1024 / 1024).toFixed(1) + ' MB'
}

function send(command) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    lastMessage.value = 'WebSocket 未连接，无法发送：' + command
    return
  }
  ws.send(command)
  lastMessage.value = '已发送：' + command
}

function onMessage(event) {
  let message
  try {
    message = JSON.parse(event.data)
  } catch (error) {
    return
  }

  if (message.type === 'status') {
    Object.assign(data, message)
    return
  }

  if (message.type === 'scan_start') {
    scan.value = []
    lastMessage.value = '正在扫描附近 BMS...'
    return
  }

  if (message.type === 'bms_found') {
    if (!scan.value.some(item => item.mac === message.mac)) {
      scan.value.push(message)
    }
    return
  }

  if (message.type === 'scan_done') {
    lastMessage.value = '扫描完成，共发现 ' + scan.value.length + ' 个 BMS'
    return
  }

  if (message.type === 'select_ok') {
    lastMessage.value = 'BMS 选择成功：' + (message.mac || '')
    return
  }

  if (message.type === 'error') {
    lastMessage.value = 'ESP32：' + (message.message || '操作失败')
    return
  }

  if (message.type === 'pong') {
    lastMessage.value = 'ESP32 PONG'
    return
  }

  if (message.type === 'rebooting') {
    lastMessage.value = 'ESP32 正在重启...'
  }
}

function connect() {
  if (ws) {
    try { ws.close() } catch (error) {}
  }

  const host = window.location.hostname || '192.168.4.1'
  ws = new WebSocket('ws://' + host + ':81/')

  ws.onopen = function () {
    connected.value = true
    lastMessage.value = 'WebSocket 已连接'
    send('GET_STATUS')
  }

  ws.onmessage = onMessage

  ws.onerror = function () {
    lastMessage.value = 'WebSocket 连接错误'
  }

  ws.onclose = function () {
    connected.value = false
    lastMessage.value = 'WebSocket 已断开，1.5 秒后重连'
    reconnectTimer = window.setTimeout(connect, 1500)
  }
}

async function loadFsInfo() {
  try {
    const response = await fetch('/fs-info', { cache: 'no-store' })
    if (!response.ok) throw new Error('HTTP ' + response.status)
    const value = await response.json()
    Object.assign(fs, value)
    lastMessage.value = 'LittleFS 状态读取成功'
  } catch (error) {
    lastMessage.value = 'LittleFS 状态读取失败'
  }
}

onMounted(function () {
  connect()
  loadFsInfo()
})

onBeforeUnmount(function () {
  if (reconnectTimer) window.clearTimeout(reconnectTimer)
  if (ws) ws.close()
})
</script>
