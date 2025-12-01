import { Routes } from '@angular/router';
import { RootLayout } from './root';
import { App } from './app';
import { AgentDetailPage } from './pages/agent-detail/agent-detail';

export const routes: Routes = [
  {
    path: '',
    component: RootLayout,
    children: [
      { path: '', component: App },
      { path: 'agent/:id', component: AgentDetailPage }
    ]
  }
];
