import { motion } from 'framer-motion';
import { Card } from './Card';
import { Device } from '../types';
import * as Icons from 'lucide-react';

interface DevicePanelProps {
  title: string;
  devices: Device[];
  onDeviceClick?: (device: Device) => void;
}

export function DevicePanel({ title, devices, onDeviceClick }: DevicePanelProps) {
  const getIcon = (iconName: string) => {
    const Icon = (Icons as any)[iconName];
    return Icon || Icons.Box;
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.4 }}
      className="glass-effect rounded-3xl p-8"
    >
      <h2 className="text-xl font-semibold text-white mb-6 uppercase tracking-wide">
        {title}
      </h2>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {devices.map((device, index) => (
          <motion.div
            key={device.id}
            initial={{ opacity: 0, scale: 0.9 }}
            animate={{ opacity: 1, scale: 1 }}
            transition={{ delay: index * 0.1 }}
          >
            <Card
              title={device.name}
              subtitle={device.lastUpdate}
              icon={getIcon(device.icon)}
              status={device.status}
              onClick={() => onDeviceClick?.(device)}
            />
          </motion.div>
        ))}
      </div>
    </motion.div>
  );
}
