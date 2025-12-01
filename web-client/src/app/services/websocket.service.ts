import { Injectable, signal } from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class WebSocketService {
  private socket: WebSocket | null = null;

  private _status = signal<'connected' | 'connecting' | 'disconnected'>('disconnected');
  private _lastMessage = signal<any>(null);

  status() {
    return this._status();
  }

  lastMessage() {
    return this._lastMessage();
  }

  isConnected(): boolean {
    return this._status() === 'connected';
  }

  connect(url: string) {
    this.disconnect();

    this._status.set('connecting');

    this.socket = new WebSocket(url);

    this.socket.onopen = () => {
      console.log('WS connected');
      this._status.set('connected');
    };

    this.socket.onclose = () => {
      console.log('WS closed');
      this._status.set('disconnected');
    };

    this.socket.onerror = () => {
      console.log('WS error');
      this._status.set('disconnected');
    };

    this.socket.onmessage = (event) => {
      try {
        const json = JSON.parse(event.data);
        this._lastMessage.set(json);
      } catch {
        console.error("Invalid JSON:", event.data);
      }
    };
  }

  disconnect() {
    if (this.socket) {
      this.socket.close();
      this.socket = null;
    }
    this._status.set('disconnected');
  }

  // ⭐⭐ HÀM NÀY BẮT BUỘC CÓ – UI ĐANG GỌI NÓ ⭐⭐
  send(data: any) {
    if (!this.socket || this._status() !== 'connected') {
      console.warn("WS not connected");
      return;
    }
    this.socket.send(JSON.stringify(data));
  }
}
