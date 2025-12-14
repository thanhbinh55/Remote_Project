import { bootstrapApplication } from '@angular/platform-browser';
import { appConfig } from './app/app.config';
import { RootLayout } from './app/root';

bootstrapApplication(RootLayout, appConfig)
  .catch((err) => console.error(err));
