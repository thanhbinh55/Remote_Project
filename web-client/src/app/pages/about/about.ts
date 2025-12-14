import { Component } from '@angular/core';
import { Location } from '@angular/common';

@Component({
  selector: 'app-about',
  standalone: true,
  templateUrl: './about.html',
  styleUrls: ['./about.css']
})
export class AboutComponent {

  constructor(private location: Location) {}

  goBack() {
    this.location.back();
  }
}
