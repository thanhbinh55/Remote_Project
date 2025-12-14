import { Routes } from '@angular/router';
import { AgentsComponent } from './pages/agents/agents';
import { AgentDetailComponent } from './pages/agent-detail/agent-detail';
import { AboutComponent } from './pages/about/about';

export const routes: Routes = [
  { path: '', component: AgentsComponent },
  { path: 'agent/:id', component: AgentDetailComponent },
  { path: 'about', component: AboutComponent }
];
