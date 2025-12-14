import { Component } from '@angular/core';
import { CommonModule } from '@angular/common';
import { WebSocketService } from '../../services/websocket.service';

@Component({
  selector: 'app-webcam',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './webcam.html',
  styleUrl: './webcam.css'
})
export class WebcamComponent {

  constructor(public ws: WebSocketService) {}

  startStream(): void {
    this.ws.sendJson({
      module: 'WEBCAM',
      command: 'START_STREAM'
    });
  }

  stopStream(): void {
    this.ws.sendJson({
      module: 'WEBCAM',
      command: 'STOP_STREAM'
    });
  }

}
