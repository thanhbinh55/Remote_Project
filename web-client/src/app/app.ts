// src/app/app.ts
import { Component, OnInit, inject, signal } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { NgFor, JsonPipe, NgClass } from '@angular/common';

import { AgentService } from './services/agent.service';
import { Agent } from './models/agent';
import { Router } from '@angular/router';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [NgFor, JsonPipe, NgClass],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class App implements OnInit {

  // signal lưu danh sách agents
  agents = signal<Agent[]>([]);
  router = inject(Router);
  // inject service
  private agentService = inject(AgentService);

  ngOnInit(): void {
    this.agentService.getAgents().subscribe({
      next: (data) => {
        this.agents.set(data);
        console.log('Agents from server:', data);
      },
      error: (err) => {
        console.error('Error loading agents', err);
      }
    });
  }
  
connectToAgent(agent: Agent) {
  this.router.navigate(['/agent', agent.machineId]);
}

}

