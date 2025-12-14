export interface Agent {
  machineId: string;
  ip: string;
  os: string;
  wsPort: number;
  tags?: string[];
  online: boolean;
}
