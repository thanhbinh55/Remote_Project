import { Injectable, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { Agent } from '../models/agent';

@Injectable({
  providedIn: 'root'
})
export class AgentService {

  private apiUrl = 'http://localhost:3000/api/agents';

  private http = inject(HttpClient);

  getAgents(): Observable<Agent[]> {
    return this.http.get<Agent[]>(this.apiUrl);
  }

  getAgent(id: string): Observable<Agent> {
    return this.http.get<Agent>(`${this.apiUrl}/${id}`);
  }

}
