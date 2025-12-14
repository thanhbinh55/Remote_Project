import { Component, inject, signal } from '@angular/core';
import { NgFor } from '@angular/common';
import { Router } from '@angular/router';
import { AgentService } from '../../services/agent.service';

@Component({
  selector: 'app-agents',
  standalone: true,
  imports: [NgFor],
  templateUrl: './agents.html',
  styleUrl: './agents.css'
})
export class AgentsComponent {
  agents = signal<any[]>([]);
  router = inject(Router);
  service = inject(AgentService);

  ngOnInit() {
    this.service.getAgents().subscribe(a => this.agents.set(a));
  }

  open(a: any) {
    this.router.navigate(['/agent', a.machineId]);
  }
}
