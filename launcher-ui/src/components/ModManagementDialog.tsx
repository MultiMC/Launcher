import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Package, Upload, Trash2, Power, PowerOff, Download, Loader2, Plus } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface Mod {
  id: string;
  name: string;
  version: string;
  enabled: boolean;
  fileName: string;
}

interface ModManagementDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instanceId: string;
  instanceName: string;
  minecraftVersion: string;
}

export function ModManagementDialog({
  isOpen,
  onClose,
  instanceId,
  instanceName,
  minecraftVersion,
}: ModManagementDialogProps) {
  const [mods, setMods] = useState<Mod[]>([]);
  const [loading, setLoading] = useState(false);
  const [activeTab, setActiveTab] = useState<'installed' | 'modloader'>('installed');
  const [selectedModLoader, setSelectedModLoader] = useState<'forge' | 'fabric' | 'quilt' | 'liteloader'>('forge');
  const [modLoaderVersions, setModLoaderVersions] = useState<string[]>([]);
  const [selectedVersion, setSelectedVersion] = useState('');
  const [installing, setInstalling] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadMods();
    }
  }, [isOpen, instanceId]);

  useEffect(() => {
    if (activeTab === 'modloader') {
      loadModLoaderVersions();
    }
  }, [activeTab, selectedModLoader, minecraftVersion]);

  const loadMods = async () => {
    setLoading(true);
    const modList = await MinecraftAPI.getInstanceMods(instanceId);
    setMods(modList);
    setLoading(false);
  };

  const loadModLoaderVersions = async () => {
    const versions = await MinecraftAPI.getModLoaderVersions(selectedModLoader, minecraftVersion);
    setModLoaderVersions(versions);
    if (versions.length > 0) {
      setSelectedVersion(versions[0]);
    }
  };

  const handleToggleMod = async (modId: string, enabled: boolean) => {
    const success = await MinecraftAPI.toggleMod(instanceId, modId, enabled);
    if (success) {
      setMods(prev =>
        prev.map(mod =>
          mod.id === modId ? { ...mod, enabled } : mod
        )
      );
    }
  };

  const handleRemoveMod = async (modId: string) => {
    if (!confirm('Are you sure you want to remove this mod?')) return;

    const success = await MinecraftAPI.removeMod(instanceId, modId);
    if (success) {
      await loadMods();
    }
  };

  const handleInstallModLoader = async () => {
    if (!selectedVersion) return;

    setInstalling(true);
    const success = await MinecraftAPI.installModLoader(instanceId, selectedModLoader, selectedVersion);
    if (success) {
      alert(`${selectedModLoader} ${selectedVersion} installed successfully!`);
      setActiveTab('installed');
      await loadMods();
    } else {
      alert(`Failed to install ${selectedModLoader}`);
    }
    setInstalling(false);
  };

  const handleUploadMod = () => {
    // In a real implementation, this would open a file picker
    alert('File picker would open here to select mod files (.jar)');
  };

  const handleBrowseMods = () => {
    // This would open a mod browser/downloader
    alert('Mod browser feature coming soon! This would allow you to browse and install mods from CurseForge, Modrinth, etc.');
  };

  const enabledMods = mods.filter(m => m.enabled);
  const disabledMods = mods.filter(m => !m.enabled);

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Mods - ${instanceName}`} size="xl">
      <div className="space-y-6">
        {/* Tabs */}
        <div className="flex gap-2 border-b border-slate-700/50">
          <button
            onClick={() => setActiveTab('installed')}
            className={`px-4 py-2 font-medium transition-colors ${
              activeTab === 'installed'
                ? 'text-sky-400 border-b-2 border-sky-400'
                : 'text-gray-400 hover:text-gray-200'
            }`}
          >
            Installed Mods ({mods.length})
          </button>
          <button
            onClick={() => setActiveTab('modloader')}
            className={`px-4 py-2 font-medium transition-colors ${
              activeTab === 'modloader'
                ? 'text-sky-400 border-b-2 border-sky-400'
                : 'text-gray-400 hover:text-gray-200'
            }`}
          >
            Install Mod Loader
          </button>
        </div>

        {activeTab === 'installed' ? (
          <>
            {/* Action Buttons */}
            <div className="flex gap-3">
              <button
                onClick={handleUploadMod}
                className="flex items-center gap-2 px-4 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors"
              >
                <Upload size={18} />
                Add Mod File
              </button>
              <button
                onClick={handleBrowseMods}
                className="flex items-center gap-2 px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600 transition-colors"
              >
                <Download size={18} />
                Browse Mods
              </button>
            </div>

            {/* Mods List */}
            {loading ? (
              <div className="flex items-center justify-center py-12">
                <Loader2 className="animate-spin text-sky-400" size={32} />
              </div>
            ) : mods.length === 0 ? (
              <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
                <Package size={48} className="mx-auto text-gray-600 mb-3" />
                <p className="text-gray-400">No mods installed</p>
                <p className="text-sm text-gray-500 mt-1">Add mods to enhance your Minecraft experience</p>
              </div>
            ) : (
              <div className="space-y-4">
                {/* Enabled Mods */}
                {enabledMods.length > 0 && (
                  <div>
                    <h4 className="text-sm font-medium text-gray-400 mb-2">Enabled ({enabledMods.length})</h4>
                    <div className="space-y-2">
                      {enabledMods.map(mod => (
                        <div
                          key={mod.id}
                          className="flex items-center justify-between p-3 rounded-lg bg-slate-800/30 border border-slate-700/30"
                        >
                          <div className="flex items-center gap-3 flex-1">
                            <div className="w-10 h-10 rounded-lg bg-gradient-to-br from-sky-500 to-sky-600 flex items-center justify-center">
                              <Package size={20} className="text-white" />
                            </div>
                            <div>
                              <div className="text-white font-medium">{mod.name}</div>
                              <div className="text-sm text-gray-400">{mod.version} • {mod.fileName}</div>
                            </div>
                          </div>
                          <div className="flex items-center gap-2">
                            <button
                              onClick={() => handleToggleMod(mod.id, false)}
                              className="p-2 rounded-lg bg-green-500/20 text-green-400 hover:bg-green-500/30 transition-colors"
                              title="Disable mod"
                            >
                              <Power size={18} />
                            </button>
                            <button
                              onClick={() => handleRemoveMod(mod.id)}
                              className="p-2 rounded-lg bg-red-500/20 text-red-400 hover:bg-red-500/30 transition-colors"
                              title="Remove mod"
                            >
                              <Trash2 size={18} />
                            </button>
                          </div>
                        </div>
                      ))}
                    </div>
                  </div>
                )}

                {/* Disabled Mods */}
                {disabledMods.length > 0 && (
                  <div>
                    <h4 className="text-sm font-medium text-gray-400 mb-2">Disabled ({disabledMods.length})</h4>
                    <div className="space-y-2">
                      {disabledMods.map(mod => (
                        <div
                          key={mod.id}
                          className="flex items-center justify-between p-3 rounded-lg bg-slate-800/30 border border-slate-700/30 opacity-60"
                        >
                          <div className="flex items-center gap-3 flex-1">
                            <div className="w-10 h-10 rounded-lg bg-slate-700 flex items-center justify-center">
                              <Package size={20} className="text-gray-500" />
                            </div>
                            <div>
                              <div className="text-gray-400 font-medium">{mod.name}</div>
                              <div className="text-sm text-gray-500">{mod.version} • {mod.fileName}</div>
                            </div>
                          </div>
                          <div className="flex items-center gap-2">
                            <button
                              onClick={() => handleToggleMod(mod.id, true)}
                              className="p-2 rounded-lg bg-slate-700/50 text-gray-400 hover:bg-slate-700 hover:text-white transition-colors"
                              title="Enable mod"
                            >
                              <PowerOff size={18} />
                            </button>
                            <button
                              onClick={() => handleRemoveMod(mod.id)}
                              className="p-2 rounded-lg bg-red-500/20 text-red-400 hover:bg-red-500/30 transition-colors"
                              title="Remove mod"
                            >
                              <Trash2 size={18} />
                            </button>
                          </div>
                        </div>
                      ))}
                    </div>
                  </div>
                )}
              </div>
            )}
          </>
        ) : (
          /* Mod Loader Tab */
          <div className="space-y-6">
            <div className="bg-slate-800/30 rounded-lg p-4 border border-slate-700/30">
              <h4 className="text-white font-medium mb-3">Select Mod Loader</h4>
              <p className="text-sm text-gray-400 mb-4">
                Choose a mod loader to install. This allows you to run mods in your Minecraft instance.
              </p>

              <div className="grid grid-cols-2 gap-3 mb-4">
                {(['forge', 'fabric', 'quilt', 'liteloader'] as const).map(loader => (
                  <button
                    key={loader}
                    onClick={() => setSelectedModLoader(loader)}
                    className={`p-4 rounded-lg border-2 transition-all ${
                      selectedModLoader === loader
                        ? 'border-sky-500 bg-sky-500/20'
                        : 'border-slate-700/50 bg-slate-800/50 hover:border-slate-600'
                    }`}
                  >
                    <div className="text-white font-medium capitalize">{loader}</div>
                    <div className="text-xs text-gray-400 mt-1">
                      {loader === 'forge' && 'Most popular mod loader'}
                      {loader === 'fabric' && 'Lightweight & modern'}
                      {loader === 'quilt' && 'Fork of Fabric'}
                      {loader === 'liteloader' && 'Lightweight loader'}
                    </div>
                  </button>
                ))}
              </div>

              {modLoaderVersions.length > 0 && (
                <div>
                  <label className="block text-sm font-medium text-gray-300 mb-2">
                    Version
                  </label>
                  <select
                    value={selectedVersion}
                    onChange={e => setSelectedVersion(e.target.value)}
                    className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                  >
                    {modLoaderVersions.map(version => (
                      <option key={version} value={version}>
                        {version}
                      </option>
                    ))}
                  </select>
                </div>
              )}
            </div>

            <button
              onClick={handleInstallModLoader}
              disabled={installing || !selectedVersion}
              className="w-full flex items-center justify-center gap-2 px-6 py-3 rounded-lg bg-gradient-to-r from-sky-500 to-sky-600 text-white font-medium hover:shadow-glow disabled:opacity-50 transition-all"
            >
              {installing ? (
                <>
                  <Loader2 className="animate-spin" size={20} />
                  Installing...
                </>
              ) : (
                <>
                  <Plus size={20} />
                  Install {selectedModLoader.charAt(0).toUpperCase() + selectedModLoader.slice(1)}
                </>
              )}
            </button>
          </div>
        )}
      </div>
    </Modal>
  );
}
