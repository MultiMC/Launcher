export interface Device {
  id: string;
  name: string;
  status: 'active' | 'deactivated';
  icon: string;
  category: string;
  lastUpdate?: string;
}

export interface Instance {
  id: string;
  name: string;
  version: string;
  status: 'running' | 'stopped' | 'error';
  icon?: string;
  lastPlayed?: Date;
  modCount?: number;
  playTime?: number;
}

export interface DeviceCategory {
  id: string;
  name: string;
  icon: string;
  devices: Device[];
}
