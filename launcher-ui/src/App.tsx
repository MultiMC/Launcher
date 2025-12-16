import { useState, useEffect } from 'react';
import { Sidebar } from './components/Sidebar';
import { Header } from './components/Header';
import { DevicePanel } from './components/DevicePanel';
import { CategoryList } from './components/CategoryList';
import { StatusPanel } from './components/StatusPanel';
import { NewInstanceDialog } from './components/NewInstanceDialog';
import { InstanceDetailsDialog } from './components/InstanceDetailsDialog';
import { SettingsDialog } from './components/SettingsDialog';
import { Instance } from './types';
import { Plus } from 'lucide-react';
import { motion } from 'framer-motion';
import { MinecraftAPI } from './services/api';

// Convert Instance to Device for compatibility with existing components
function instanceToDevice(instance: Instance) {
  return {
    id: instance.id,
    name: instance.name,
    status: instance.status === 'running' ? 'active' as const : 'deactivated' as const,
    icon: 'Package',
    category: 'vanilla', // You could add this to Instance type
    lastUpdate: instance.lastPlayed ? `Last played ${formatDate(instance.lastPlayed)}` : 'Never played',
  };
}

function formatDate(date: Date): string {
  const now = new Date();
  const diffMs = now.getTime() - new Date(date).getTime();
  const diffHours = Math.floor(diffMs / (1000 * 60 * 60));

  if (diffHours < 1) return 'just now';
  if (diffHours < 24) return `${diffHours}h ago`;
  if (diffHours < 48) return 'yesterday';
  return `${Math.floor(diffHours / 24)}d ago`;
}

function App() {
  // State
  const [instances, setInstances] = useState<Instance[]>([]);
  const [activeCategory, setActiveCategory] = useState('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [activeSection, setActiveSection] = useState('instances');
  const [loading, setLoading] = useState(true);

  // Dialog state
  const [showNewInstance, setShowNewInstance] = useState(false);
  const [showSettings, setShowSettings] = useState(false);
  const [selectedInstance, setSelectedInstance] = useState<Instance | null>(null);

  // Load instances on mount
  useEffect(() => {
    loadInstances();
  }, []);

  // Handle settings dialog when user clicks settings in sidebar
  useEffect(() => {
    if (activeSection === 'settings') {
      setShowSettings(true);
      setActiveSection('instances'); // Reset to instances
    }
  }, [activeSection]);

  const loadInstances = async () => {
    setLoading(true);
    try {
      const data = await MinecraftAPI.getInstances();
      setInstances(data);
    } catch (error) {
      console.error('Failed to load instances:', error);
      // Fallback to mock data
      setInstances([
        {
          id: '1',
          name: 'Vanilla 1.20.4',
          version: '1.20.4',
          status: 'stopped',
          lastPlayed: new Date(Date.now() - 2 * 60 * 60 * 1000),
          modCount: 0,
          playTime: 7200,
        },
        {
          id: '2',
          name: 'Modded SkyFactory',
          version: '1.12.2',
          status: 'stopped',
          lastPlayed: new Date(Date.now() - 24 * 60 * 60 * 1000),
          modCount: 156,
          playTime: 48600,
        },
        {
          id: '3',
          name: 'FTB Ultimate',
          version: '1.16.5',
          status: 'stopped',
          lastPlayed: new Date(Date.now() - 3 * 24 * 60 * 60 * 1000),
          modCount: 203,
          playTime: 32400,
        },
        {
          id: '4',
          name: 'Fabric 1.20',
          version: '1.20.1',
          status: 'running',
          lastPlayed: new Date(),
          modCount: 42,
          playTime: 18000,
        },
      ]);
    } finally {
      setLoading(false);
    }
  };

  const handleCreateInstance = async (name: string, version: string) => {
    const newId = await MinecraftAPI.createInstance(name, version);
    if (newId) {
      await loadInstances();
    }
  };

  const handleLaunchInstance = async (instanceId: string) => {
    const success = await MinecraftAPI.launchInstance(instanceId);
    if (success) {
      // Update instance status
      setInstances(prev =>
        prev.map(inst =>
          inst.id === instanceId
            ? { ...inst, status: 'running' as const }
            : inst
        )
      );
    }
  };

  const handleDeleteInstance = async (instanceId: string) => {
    const success = await MinecraftAPI.deleteInstance(instanceId);
    if (success) {
      await loadInstances();
    }
  };

  const handleInstanceClick = (device: any) => {
    const instance = instances.find(i => i.id === device.id);
    if (instance) {
      setSelectedInstance(instance);
    }
  };

  // Filter and convert instances
  const filteredInstances = instances
    .filter((instance) => {
      const matchesSearch = instance.name.toLowerCase().includes(searchQuery.toLowerCase());
      // For now, show all in all categories
      return matchesSearch;
    })
    .map(instanceToDevice);

  // Calculate category counts
  const categories = [
    { id: 'all', name: 'All Instances', icon: 'LayoutGrid', count: instances.length },
    { id: 'vanilla', name: 'Vanilla', icon: 'Home', count: instances.filter(i => (i.modCount || 0) === 0).length },
    { id: 'modded', name: 'Modded', icon: 'Lightbulb', count: instances.filter(i => (i.modCount || 0) > 0).length },
    { id: 'running', name: 'Running', icon: 'Play', count: instances.filter(i => i.status === 'running').length },
  ];

  return (
    <div className="flex h-screen bg-gradient-to-br from-slate-950 via-slate-900 to-slate-950 overflow-hidden">
      {/* Sidebar */}
      <Sidebar activeItem={activeSection} onItemSelect={setActiveSection} />

      {/* Main Content */}
      <div className="flex-1 flex overflow-hidden">
        {/* Left Panel - Categories */}
        <div className="w-80 p-6 overflow-y-auto">
          <CategoryList
            title="Categories"
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
            onClick={() => setShowNewInstance(true)}
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

          {/* Loading State */}
          {loading ? (
            <div className="flex items-center justify-center h-64">
              <div className="text-gray-400">Loading instances...</div>
            </div>
          ) : filteredInstances.length === 0 ? (
            <div className="glass-effect rounded-3xl p-12 text-center">
              <h3 className="text-xl font-semibold text-white mb-2">No Instances Yet</h3>
              <p className="text-gray-400 mb-6">Create your first Minecraft instance to get started!</p>
              <button
                onClick={() => setShowNewInstance(true)}
                className="px-6 py-3 rounded-xl bg-gradient-to-r from-sky-500 to-sky-600 text-white font-semibold hover:shadow-glow transition-all"
              >
                Create Instance
              </button>
            </div>
          ) : (
            <DevicePanel
              title={`Your Instances (${filteredInstances.length})`}
              devices={filteredInstances}
              onDeviceClick={handleInstanceClick}
            />
          )}
        </div>

        {/* Right Panel - Status & Controls */}
        <div className="w-80 p-6 overflow-y-auto">
          <StatusPanel />
        </div>
      </div>

      {/* Dialogs */}
      <NewInstanceDialog
        isOpen={showNewInstance}
        onClose={() => setShowNewInstance(false)}
        onCreateInstance={handleCreateInstance}
      />

      <InstanceDetailsDialog
        isOpen={selectedInstance !== null}
        onClose={() => setSelectedInstance(null)}
        instance={selectedInstance}
        onLaunch={handleLaunchInstance}
        onDelete={handleDeleteInstance}
      />

      <SettingsDialog
        isOpen={showSettings}
        onClose={() => setShowSettings(false)}
      />
    </div>
  );
}

export default App;
