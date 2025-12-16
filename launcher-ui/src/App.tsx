import { useState } from 'react';
import { Sidebar } from './components/Sidebar';
import { Header } from './components/Header';
import { DevicePanel } from './components/DevicePanel';
import { CategoryList } from './components/CategoryList';
import { StatusPanel } from './components/StatusPanel';
import { Device } from './types';
import { Plus } from 'lucide-react';
import { motion } from 'framer-motion';

// Mock data for demonstration
const mockInstances: Device[] = [
  { id: '1', name: 'Vanilla 1.20.4', status: 'active', icon: 'Pickaxe', category: 'vanilla', lastUpdate: 'Last played 2h ago' },
  { id: '2', name: 'Modded SkyFactory', status: 'deactivated', icon: 'Factory', category: 'modded', lastUpdate: 'Last played yesterday' },
  { id: '3', name: 'FTB Ultimate', status: 'deactivated', icon: 'Puzzle', category: 'modpack', lastUpdate: 'Deactivated 3d ago' },
  { id: '4', name: 'Fabric 1.20', status: 'active', icon: 'Package', category: 'vanilla', lastUpdate: 'Currently running' },
  { id: '5', name: 'Forge 1.19.2', status: 'deactivated', icon: 'Hammer', category: 'modded', lastUpdate: 'Deactivated 1w ago' },
  { id: '6', name: 'Create Mod Pack', status: 'active', icon: 'Cog', category: 'modpack', lastUpdate: 'Last played 5h ago' },
];

const categories = [
  { id: 'all', name: 'All Instances', icon: 'LayoutGrid', count: 6 },
  { id: 'vanilla', name: 'Vanilla', icon: 'Home', count: 2 },
  { id: 'modded', name: 'Modded', icon: 'Lightbulb', count: 2 },
  { id: 'modpack', name: 'Modpacks', icon: 'Package', count: 2 },
];

function App() {
  const [activeCategory, setActiveCategory] = useState('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [activeSection, setActiveSection] = useState('home');

  const filteredInstances = mockInstances.filter((instance) => {
    const matchesCategory = activeCategory === 'all' || instance.category === activeCategory;
    const matchesSearch = instance.name.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesCategory && matchesSearch;
  });

  return (
    <div className="flex h-screen bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950 overflow-hidden">
      {/* Sidebar */}
      <Sidebar activeItem={activeSection} onItemSelect={setActiveSection} />

      {/* Main Content */}
      <div className="flex-1 flex overflow-hidden">
        {/* Left Panel - Categories */}
        <div className="w-80 p-6 overflow-y-auto">
          <CategoryList
            title="Devices"
            categories={categories}
            activeCategory={activeCategory}
            onCategoryClick={setActiveCategory}
          />
        </div>

        {/* Center Panel - Main Content */}
        <div className="flex-1 p-8 overflow-y-auto">
          <Header
            title="Minecraft Launcher"
            onSearch={setSearchQuery}
          />

          {/* Add Instance Button */}
          <motion.button
            whileHover={{ scale: 1.02 }}
            whileTap={{ scale: 0.98 }}
            className="
              mb-6 px-6 py-3 rounded-xl
              bg-gradient-to-r from-sky-500 to-sky-600
              text-white font-semibold
              shadow-lg hover:shadow-glow
              transition-all duration-200
              flex items-center gap-2
            "
          >
            <Plus size={20} />
            Add Instance
          </motion.button>

          {/* Instances Grid */}
          <DevicePanel
            title="Your Instances"
            devices={filteredInstances}
            onDeviceClick={(device) => console.log('Clicked:', device)}
          />
        </div>

        {/* Right Panel - Status & Controls */}
        <div className="w-80 p-6 overflow-y-auto">
          <StatusPanel />
        </div>
      </div>
    </div>
  );
}

export default App;
