import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { User, Coffee, Folder, Globe, Settings as SettingsIcon, Loader2, LogIn } from 'lucide-react';
import { SettingsAPI, AccountAPI } from '../services/api';

interface SettingsDialogProps {
  isOpen: boolean;
  onClose: () => void;
}

interface Settings {
  showConsole: boolean;
  closeLauncher: boolean;
  checkUpdates: boolean;
  javaPath: string;
  memoryAllocation: number;
  language: string;
}

export function SettingsDialog({ isOpen, onClose }: SettingsDialogProps) {
  const [activeTab, setActiveTab] = useState('general');
  const [accounts, setAccounts] = useState<Array<{ id: string; username: string; type: string }>>([]);
  const [settings, setSettings] = useState<Settings>({
    showConsole: false,
    closeLauncher: false,
    checkUpdates: true,
    javaPath: '/usr/lib/jvm/java-17-openjdk',
    memoryAllocation: 4096,
    language: 'en_US',
  });
  const [saving, setSaving] = useState(false);
  const [hasChanges, setHasChanges] = useState(false);
  const [addingAccount, setAddingAccount] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadAccounts();
      loadSettings();
    }
  }, [isOpen]);

  const loadAccounts = async () => {
    const accountList = await AccountAPI.getAccounts();
    setAccounts(accountList);
  };

  const loadSettings = async () => {
    const loadedSettings = await SettingsAPI.getSettings();
    if (loadedSettings) {
      setSettings(prev => ({ ...prev, ...loadedSettings }));
    }
  };

  const handleSettingChange = (key: keyof Settings, value: any) => {
    setSettings(prev => ({ ...prev, [key]: value }));
    setHasChanges(true);
  };

  const handleSave = async () => {
    setSaving(true);
    try {
      await SettingsAPI.updateSettings(settings as any);
      setHasChanges(false);
      // Show success notification or toast here
    } catch (error) {
      console.error('Failed to save settings:', error);
      alert('Failed to save settings');
    } finally {
      setSaving(false);
    }
  };

  const handleAddMicrosoftAccount = async () => {
    setAddingAccount(true);
    try {
      // In a real implementation, this would open Microsoft OAuth flow
      // For now, we'll simulate it
      const success = await AccountAPI.addAccount('microsoft', '');
      if (success) {
        await loadAccounts();
      } else {
        alert('Failed to add Microsoft account. Please try again.');
      }
    } catch (error) {
      console.error('Failed to add account:', error);
      alert('Failed to add Microsoft account');
    } finally {
      setAddingAccount(false);
    }
  };

  const handleRemoveAccount = async (accountId: string) => {
    if (!confirm('Are you sure you want to remove this account?')) return;

    const success = await AccountAPI.removeAccount(accountId);
    if (success) {
      await loadAccounts();
    }
  };

  const tabs = [
    { id: 'general', label: 'General', icon: SettingsIcon },
    { id: 'accounts', label: 'Accounts', icon: User },
    { id: 'java', label: 'Java', icon: Coffee },
    { id: 'folders', label: 'Folders', icon: Folder },
    { id: 'language', label: 'Language', icon: Globe },
  ];

  return (
    <Modal isOpen={isOpen} onClose={onClose} title="Settings" size="xl">
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
          {activeTab === 'general' && (
            <div className="space-y-6">
              <div>
                <h3 className="text-lg font-semibold text-white mb-4">General Settings</h3>

                <div className="space-y-4">
                  <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                    <div>
                      <div className="text-white font-medium">Show console on launch</div>
                      <div className="text-sm text-gray-400 mt-1">Display game output window</div>
                    </div>
                    <input
                      type="checkbox"
                      checked={settings.showConsole}
                      onChange={(e) => handleSettingChange('showConsole', e.target.checked)}
                      className="w-5 h-5"
                    />
                  </div>

                  <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                    <div>
                      <div className="text-white font-medium">Close launcher after launch</div>
                      <div className="text-sm text-gray-400 mt-1">Minimize launcher when game starts</div>
                    </div>
                    <input
                      type="checkbox"
                      checked={settings.closeLauncher}
                      onChange={(e) => handleSettingChange('closeLauncher', e.target.checked)}
                      className="w-5 h-5"
                    />
                  </div>

                  <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                    <div>
                      <div className="text-white font-medium">Check for updates</div>
                      <div className="text-sm text-gray-400 mt-1">Automatically check for launcher updates</div>
                    </div>
                    <input
                      type="checkbox"
                      checked={settings.checkUpdates}
                      onChange={(e) => handleSettingChange('checkUpdates', e.target.checked)}
                      className="w-5 h-5"
                    />
                  </div>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'accounts' && (
            <div className="space-y-6">
              <div className="flex items-center justify-between mb-4">
                <div>
                  <h3 className="text-lg font-semibold text-white">Microsoft Accounts</h3>
                  <p className="text-sm text-gray-400 mt-1">Sign in with your Microsoft account to play Minecraft</p>
                </div>
                <button
                  onClick={handleAddMicrosoftAccount}
                  disabled={addingAccount}
                  className="px-4 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors flex items-center gap-2 disabled:opacity-50"
                >
                  {addingAccount ? (
                    <>
                      <Loader2 size={16} className="animate-spin" />
                      Adding...
                    </>
                  ) : (
                    <>
                      <LogIn size={16} />
                      Add Account
                    </>
                  )}
                </button>
              </div>

              {accounts.length === 0 ? (
                <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
                  <User size={48} className="mx-auto text-gray-600 mb-3" />
                  <p className="text-gray-400">No accounts added</p>
                  <p className="text-sm text-gray-500 mt-1">Add a Microsoft account to play Minecraft</p>
                  <button
                    onClick={handleAddMicrosoftAccount}
                    disabled={addingAccount}
                    className="mt-4 px-6 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors"
                  >
                    Sign in with Microsoft
                  </button>
                </div>
              ) : (
                <div className="space-y-2">
                  {accounts.map((account) => (
                    <div
                      key={account.id}
                      className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30"
                    >
                      <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-lg bg-gradient-to-br from-sky-500 to-sky-600 flex items-center justify-center text-white font-bold">
                          {account.username[0].toUpperCase()}
                        </div>
                        <div>
                          <div className="text-white font-medium">{account.username}</div>
                          <div className="text-sm text-gray-400">{account.type}</div>
                        </div>
                      </div>
                      <button
                        onClick={() => handleRemoveAccount(account.id)}
                        className="text-red-400 hover:text-red-300"
                      >
                        Remove
                      </button>
                    </div>
                  ))}
                </div>
              )}
            </div>
          )}

          {activeTab === 'java' && (
            <div className="space-y-6">
              <h3 className="text-lg font-semibold text-white mb-4">Java Settings</h3>

              <div className="space-y-4">
                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Java Path</label>
                  <div className="flex gap-2">
                    <input
                      type="text"
                      value={settings.javaPath}
                      onChange={(e) => handleSettingChange('javaPath', e.target.value)}
                      className="flex-1 px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                    />
                    <button className="px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600">
                      Browse
                    </button>
                  </div>
                </div>

                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">
                    Memory Allocation (RAM): {settings.memoryAllocation} MB
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
                      {(settings.memoryAllocation / 1024).toFixed(1)} GB
                    </span>
                    <span>16 GB</span>
                  </div>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'folders' && (
            <div className="space-y-6">
              <h3 className="text-lg font-semibold text-white mb-4">Folder Locations</h3>

              <div className="space-y-4">
                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Instances Folder</label>
                  <div className="flex gap-2">
                    <input
                      type="text"
                      defaultValue="~/minecraft/instances"
                      className="flex-1 px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                      readOnly
                    />
                    <button className="px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600">
                      Open
                    </button>
                  </div>
                </div>

                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Mods Folder</label>
                  <div className="flex gap-2">
                    <input
                      type="text"
                      defaultValue="~/minecraft/mods"
                      className="flex-1 px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                      readOnly
                    />
                    <button className="px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600">
                      Open
                    </button>
                  </div>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'language' && (
            <div className="space-y-6">
              <h3 className="text-lg font-semibold text-white mb-4">Language & Region</h3>

              <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                <label className="block text-white font-medium mb-2">Display Language</label>
                <select
                  value={settings.language}
                  onChange={(e) => handleSettingChange('language', e.target.value)}
                  className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                >
                  <option value="en_US">English (US)</option>
                  <option value="en_GB">English (UK)</option>
                  <option value="es_ES">Español</option>
                  <option value="fr_FR">Français</option>
                  <option value="de_DE">Deutsch</option>
                </select>
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
