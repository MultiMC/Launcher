import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Settings, Coffee, Terminal, Loader2 } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface InstanceSettings {
  javaPath?: string;
  memoryAllocation?: number;
  jvmArguments?: string;
  gameArguments?: string;
  preLaunchCommand?: string;
  postExitCommand?: string;
  overrideGlobalJava?: boolean;
  overrideGlobalMemory?: boolean;
  wrapperCommand?: string;
}

interface InstanceSettingsDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instanceId: string;
  instanceName: string;
}

export function InstanceSettingsDialog({
  isOpen,
  onClose,
  instanceId,
  instanceName,
}: InstanceSettingsDialogProps) {
  const [settings, setSettings] = useState<InstanceSettings>({
    overrideGlobalJava: false,
    overrideGlobalMemory: false,
    memoryAllocation: 4096,
    javaPath: '',
    jvmArguments: '',
    gameArguments: '',
    preLaunchCommand: '',
    postExitCommand: '',
    wrapperCommand: '',
  });
  const [activeTab, setActiveTab] = useState('java');
  const [saving, setSaving] = useState(false);
  const [hasChanges, setHasChanges] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadSettings();
    }
  }, [isOpen, instanceId]);

  const loadSettings = async () => {
    // Load instance-specific settings
    const details = await MinecraftAPI.getInstanceDetails(instanceId);
    if (details) {
      // Settings would be loaded from instance details
      setSettings(prev => ({ ...prev }));
    }
  };

  const handleSettingChange = (key: keyof InstanceSettings, value: any) => {
    setSettings(prev => ({ ...prev, [key]: value }));
    setHasChanges(true);
  };

  const handleSave = async () => {
    setSaving(true);
    try {
      await MinecraftAPI.updateInstance(instanceId, settings as any);
      setHasChanges(false);
      alert('Settings saved successfully!');
    } catch (error) {
      console.error('Failed to save settings:', error);
      alert('Failed to save settings');
    } finally {
      setSaving(false);
    }
  };

  const tabs = [
    { id: 'java', label: 'Java', icon: Coffee },
    { id: 'commands', label: 'Commands', icon: Terminal },
    { id: 'advanced', label: 'Advanced', icon: Settings },
  ];

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Instance Settings - ${instanceName}`} size="xl">
      <div className="flex gap-6 min-h-[500px]">
        {/* Sidebar */}
        <div className="w-48 space-y-1">
          {tabs.map((tab) => {
            const Icon = tab.icon;
            const isActive = activeTab === tab.id;
            return (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className={`
                  w-full flex items-center gap-3 px-4 py-3 rounded-lg transition-colors
                  ${isActive
                    ? 'bg-sky-500/20 text-sky-400 border border-sky-500/50'
                    : 'text-gray-400 hover:text-gray-200 hover:bg-slate-800/50'
                  }
                `}
              >
                <Icon size={18} />
                <span className="text-sm font-medium">{tab.label}</span>
              </button>
            );
          })}
        </div>

        {/* Content */}
        <div className="flex-1">
          {activeTab === 'java' && (
            <div className="space-y-6">
              <h3 className="text-lg font-semibold text-white mb-4">Java Settings</h3>

              <div className="space-y-4">
                <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <div>
                    <div className="text-white font-medium">Override global Java settings</div>
                    <div className="text-sm text-gray-400 mt-1">Use instance-specific Java configuration</div>
                  </div>
                  <input
                    type="checkbox"
                    checked={settings.overrideGlobalJava}
                    onChange={(e) => handleSettingChange('overrideGlobalJava', e.target.checked)}
                    className="w-5 h-5"
                  />
                </div>

                {settings.overrideGlobalJava && (
                  <>
                    <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                      <label className="block text-white font-medium mb-2">Java Path</label>
                      <div className="flex gap-2">
                        <input
                          type="text"
                          value={settings.javaPath}
                          onChange={(e) => handleSettingChange('javaPath', e.target.value)}
                          placeholder="/usr/lib/jvm/java-17-openjdk"
                          className="flex-1 px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                        />
                        <button className="px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600">
                          Auto-detect
                        </button>
                      </div>
                    </div>

                    <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                      <div>
                        <div className="text-white font-medium">Override memory settings</div>
                        <div className="text-sm text-gray-400 mt-1">Use custom memory allocation</div>
                      </div>
                      <input
                        type="checkbox"
                        checked={settings.overrideGlobalMemory}
                        onChange={(e) => handleSettingChange('overrideGlobalMemory', e.target.checked)}
                        className="w-5 h-5"
                      />
                    </div>

                    {settings.overrideGlobalMemory && (
                      <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                        <label className="block text-white font-medium mb-2">
                          Memory Allocation: {settings.memoryAllocation} MB
                        </label>
                        <input
                          type="range"
                          min="1024"
                          max="16384"
                          step="512"
                          value={settings.memoryAllocation}
                          onChange={(e) => handleSettingChange('memoryAllocation', parseInt(e.target.value))}
                          className="w-full"
                        />
                        <div className="flex justify-between text-sm text-gray-400 mt-2">
                          <span>1 GB</span>
                          <span className="text-sky-400 font-medium">
                            {((settings.memoryAllocation || 0) / 1024).toFixed(1)} GB
                          </span>
                          <span>16 GB</span>
                        </div>
                      </div>
                    )}

                    <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                      <label className="block text-white font-medium mb-2">JVM Arguments</label>
                      <textarea
                        value={settings.jvmArguments}
                        onChange={(e) => handleSettingChange('jvmArguments', e.target.value)}
                        placeholder="-XX:+UseG1GC -Dsun.rmi.dgc.server.gcInterval=2147483646"
                        rows={3}
                        className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white placeholder-gray-500 focus:outline-none focus:border-sky-500/50 font-mono text-sm"
                      />
                      <p className="text-xs text-gray-400 mt-2">Advanced JVM tuning parameters</p>
                    </div>
                  </>
                )}
              </div>
            </div>
          )}

          {activeTab === 'commands' && (
            <div className="space-y-6">
              <h3 className="text-lg font-semibold text-white mb-4">Custom Commands</h3>

              <div className="space-y-4">
                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Pre-launch Command</label>
                  <input
                    type="text"
                    value={settings.preLaunchCommand}
                    onChange={(e) => handleSettingChange('preLaunchCommand', e.target.value)}
                    placeholder="echo 'Starting Minecraft...'"
                    className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white placeholder-gray-500 focus:outline-none focus:border-sky-500/50 font-mono text-sm"
                  />
                  <p className="text-xs text-gray-400 mt-2">Run before launching the game</p>
                </div>

                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Post-exit Command</label>
                  <input
                    type="text"
                    value={settings.postExitCommand}
                    onChange={(e) => handleSettingChange('postExitCommand', e.target.value)}
                    placeholder="echo 'Game closed'"
                    className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white placeholder-gray-500 focus:outline-none focus:border-sky-500/50 font-mono text-sm"
                  />
                  <p className="text-xs text-gray-400 mt-2">Run after the game exits</p>
                </div>

                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Wrapper Command</label>
                  <input
                    type="text"
                    value={settings.wrapperCommand}
                    onChange={(e) => handleSettingChange('wrapperCommand', e.target.value)}
                    placeholder="gamemoderun"
                    className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white placeholder-gray-500 focus:outline-none focus:border-sky-500/50 font-mono text-sm"
                  />
                  <p className="text-xs text-gray-400 mt-2">Wrap the game launch command (e.g., for gamemode, optirun)</p>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'advanced' && (
            <div className="space-y-6">
              <h3 className="text-lg font-semibold text-white mb-4">Advanced Settings</h3>

              <div className="space-y-4">
                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Game Arguments</label>
                  <textarea
                    value={settings.gameArguments}
                    onChange={(e) => handleSettingChange('gameArguments', e.target.value)}
                    placeholder="--server mc.example.com --port 25565"
                    rows={3}
                    className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white placeholder-gray-500 focus:outline-none focus:border-sky-500/50 font-mono text-sm"
                  />
                  <p className="text-xs text-gray-400 mt-2">Additional arguments passed to Minecraft</p>
                </div>

                <div className="bg-yellow-500/10 border border-yellow-500/30 rounded-lg p-4">
                  <p className="text-sm text-yellow-200">
                    <strong>Warning:</strong> Advanced settings can affect game stability. Only modify if you know what you're doing.
                  </p>
                </div>
              </div>
            </div>
          )}

          {/* Save Button */}
          {hasChanges && (
            <div className="flex justify-end gap-3 pt-6 mt-6 border-t border-slate-700/50">
              <button
                onClick={() => {
                  loadSettings();
                  setHasChanges(false);
                }}
                className="px-6 py-2.5 rounded-lg bg-slate-700/50 text-gray-300 hover:bg-slate-700 transition-colors"
              >
                Discard
              </button>
              <button
                onClick={handleSave}
                disabled={saving}
                className="px-6 py-2.5 rounded-lg bg-gradient-to-r from-sky-500 to-sky-600 text-white font-medium hover:shadow-glow disabled:opacity-50 transition-all flex items-center gap-2"
              >
                {saving ? (
                  <>
                    <Loader2 className="animate-spin" size={16} />
                    Saving...
                  </>
                ) : (
                  'Save Changes'
                )}
              </button>
            </div>
          )}
        </div>
      </div>
    </Modal>
  );
}
