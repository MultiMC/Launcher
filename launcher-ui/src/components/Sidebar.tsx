import { motion } from 'framer-motion';
import { LucideIcon, LayoutGrid, Lock, Settings, Home, Lightbulb, Camera, Wifi, Wind } from 'lucide-react';
import { useState } from 'react';

interface SidebarItem {
  id: string;
  name: string;
  icon: LucideIcon;
}

const sidebarItems: SidebarItem[] = [
  { id: 'home', name: 'Home', icon: LayoutGrid },
  { id: 'instances', name: 'Instances', icon: Home },
  { id: 'lights', name: 'Lights', icon: Lightbulb },
  { id: 'cameras', name: 'Cameras', icon: Camera },
  { id: 'network', name: 'Network', icon: Wifi },
  { id: 'climate', name: 'Climate', icon: Wind },
  { id: 'security', name: 'Security', icon: Lock },
  { id: 'settings', name: 'Settings', icon: Settings },
];

interface SidebarProps {
  activeItem?: string;
  onItemSelect?: (id: string) => void;
}

export function Sidebar({ activeItem = 'home', onItemSelect }: SidebarProps) {
  const [selected, setSelected] = useState(activeItem);

  const handleItemClick = (id: string) => {
    setSelected(id);
    onItemSelect?.(id);
  };

  return (
    <div className="w-20 h-screen bg-slate-900/80 backdrop-blur-xl border-r border-slate-700/50 flex flex-col items-center py-6 gap-6">
      {/* Logo */}
      <div className="w-12 h-12 rounded-xl bg-gradient-to-br from-sky-500 to-sky-600 flex items-center justify-center text-white font-bold text-xl mb-4">
        MC
      </div>

      {/* Navigation Items */}
      <div className="flex flex-col gap-3 flex-1">
        {sidebarItems.map((item) => {
          const Icon = item.icon;
          const isSelected = selected === item.id;

          return (
            <motion.button
              key={item.id}
              whileHover={{ scale: 1.1 }}
              whileTap={{ scale: 0.95 }}
              onClick={() => handleItemClick(item.id)}
              className={`
                relative w-12 h-12 rounded-xl flex items-center justify-center
                transition-all duration-200
                ${isSelected
                  ? 'bg-sky-500/20 text-sky-400'
                  : 'text-gray-400 hover:text-gray-200 hover:bg-slate-800/50'
                }
              `}
              title={item.name}
            >
              {isSelected && (
                <motion.div
                  layoutId="sidebar-active"
                  className="absolute inset-0 bg-sky-500/20 rounded-xl border border-sky-500/50"
                  transition={{ type: 'spring', stiffness: 300, damping: 30 }}
                />
              )}
              <Icon size={20} className="relative z-10" />
            </motion.button>
          );
        })}
      </div>
    </div>
  );
}
