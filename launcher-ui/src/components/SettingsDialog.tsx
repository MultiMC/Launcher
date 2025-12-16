import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { User, Coffee, Folder, Globe, Settings as SettingsIcon } from 'lucide-react';
import { SettingsAPI, AccountAPI } from '../services/api';

interface SettingsDialogProps {
  isOpen: boolean;
  onClose: () => void;
}

export function SettingsDialog({ isOpen, onClose }: SettingsDialogProps) {
  const [activeTab, setActiveTab] = useState('general');
  const [accounts, setAccounts] = useState<Array<{ id: string; username: string; type: string }>>([]);

  useEffect(() => {
    if (isOpen) {
      loadAccounts();
    }
  }, [isOpen]);

  const loadAccounts = async () => {
    const accountList = await AccountAPI.getAccounts();
    setAccounts(accountList);
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
                    <input type="checkbox" className="w-5 h-5" />
                  </div>

                  <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                    <div>
                      <div className="text-white font-medium">Close launcher after launch</div>
                      <div className="text-sm text-gray-400 mt-1">Minimize launcher when game starts</div>
                    </div>
                    <input type="checkbox" className="w-5 h-5" />
                  </div>

                  <div className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                    <div>
                      <div className="text-white font-medium">Check for updates</div>
                      <div className="text-sm text-gray-400 mt-1">Automatically check for launcher updates</div>
                    </div>
                    <input type="checkbox" defaultChecked className="w-5 h-5" />
                  </div>
                </div>
              </div>
            </div>
          )}

          {activeTab === 'accounts' && (
            <div className="space-y-6">
              <div className="flex items-center justify-between mb-4">
                <h3 className="text-lg font-semibold text-white">Microsoft Accounts</h3>
                <button className="px-4 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors">
                  Add Account
                </button>
              </div>

              {accounts.length === 0 ? (
                <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
                  <User size={48} className="mx-auto text-gray-600 mb-3" />
                  <p className="text-gray-400">No accounts added</p>
                  <p className="text-sm text-gray-500 mt-1">Add a Microsoft account to play Minecraft</p>
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
                      <button className="text-red-400 hover:text-red-300">Remove</button>
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
                      defaultValue="/usr/lib/jvm/java-17-openjdk"
                      className="flex-1 px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50"
                    />
                    <button className="px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600">
                      Browse
                    </button>
                  </div>
                </div>

                <div className="p-4 rounded-lg bg-slate-800/30 border border-slate-700/30">
                  <label className="block text-white font-medium mb-2">Memory Allocation (RAM)</label>
                  <input
                    type="range"
                    min="1024"
                    max="8192"
                    step="512"
                    defaultValue="4096"
                    className="w-full"
                  />
                  <div className="flex justify-between text-sm text-gray-400 mt-2">
                    <span>1 GB</span>
                    <span className="text-sky-400 font-medium">4 GB</span>
                    <span>8 GB</span>
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
                <select className="w-full px-4 py-2 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50">
                  <option>English (US)</option>
                  <option>English (UK)</option>
                  <option>Español</option>
                  <option>Français</option>
                  <option>Deutsch</option>
                </select>
              </div>
            </div>
          )}
        </div>
      </div>
    </Modal>
  );
}
