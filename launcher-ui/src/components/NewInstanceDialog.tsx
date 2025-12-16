import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Package, Loader2 } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface NewInstanceDialogProps {
  isOpen: boolean;
  onClose: () => void;
  onCreateInstance: (name: string, version: string) => Promise<void>;
}

export function NewInstanceDialog({ isOpen, onClose, onCreateInstance }: NewInstanceDialogProps) {
  const [name, setName] = useState('');
  const [version, setVersion] = useState('');
  const [versions, setVersions] = useState<string[]>([]);
  const [loading, setLoading] = useState(false);
  const [loadingVersions, setLoadingVersions] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadVersions();
    }
  }, [isOpen]);

  const loadVersions = async () => {
    setLoadingVersions(true);
    const availableVersions = await MinecraftAPI.getAvailableVersions();
    setVersions(availableVersions);
    if (availableVersions.length > 0) {
      setVersion(availableVersions[0]);
    }
    setLoadingVersions(false);
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!name.trim() || !version) return;

    setLoading(true);
    try {
      await onCreateInstance(name.trim(), version);
      setName('');
      setVersion(versions[0] || '');
      onClose();
    } catch (error) {
      console.error('Failed to create instance:', error);
    } finally {
      setLoading(false);
    }
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title="Create New Instance" size="md">
      <form onSubmit={handleSubmit} className="space-y-6">
        {/* Instance Name */}
        <div>
          <label className="block text-sm font-medium text-gray-300 mb-2">
            Instance Name
          </label>
          <input
            type="text"
            value={name}
            onChange={(e) => setName(e.target.value)}
            placeholder="My Awesome Instance"
            className="w-full px-4 py-3 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white placeholder-gray-500 focus:outline-none focus:border-sky-500/50 focus:ring-1 focus:ring-sky-500/50 transition-all"
            required
            autoFocus
          />
        </div>

        {/* Minecraft Version */}
        <div>
          <label className="block text-sm font-medium text-gray-300 mb-2">
            Minecraft Version
          </label>
          {loadingVersions ? (
            <div className="flex items-center justify-center py-3">
              <Loader2 className="animate-spin text-sky-400" size={20} />
              <span className="ml-2 text-gray-400">Loading versions...</span>
            </div>
          ) : (
            <select
              value={version}
              onChange={(e) => setVersion(e.target.value)}
              className="w-full px-4 py-3 rounded-lg bg-slate-800/50 border border-slate-700/50 text-white focus:outline-none focus:border-sky-500/50 focus:ring-1 focus:ring-sky-500/50 transition-all"
              required
            >
              {versions.map((v) => (
                <option key={v} value={v}>
                  {v}
                </option>
              ))}
            </select>
          )}
        </div>

        {/* Instance Type Info */}
        <div className="bg-slate-800/30 rounded-lg p-4 border border-slate-700/30">
          <div className="flex items-start gap-3">
            <Package className="text-sky-400 mt-1" size={20} />
            <div>
              <h4 className="text-white font-medium mb-1">Vanilla Instance</h4>
              <p className="text-sm text-gray-400">
                A standard Minecraft installation. You can add mods and resource packs after creation.
              </p>
            </div>
          </div>
        </div>

        {/* Actions */}
        <div className="flex gap-3 justify-end pt-4 border-t border-slate-700/50">
          <button
            type="button"
            onClick={onClose}
            className="px-6 py-2.5 rounded-lg bg-slate-700/50 text-gray-300 hover:bg-slate-700 transition-colors"
            disabled={loading}
          >
            Cancel
          </button>
          <button
            type="submit"
            disabled={loading || !name.trim() || !version}
            className="px-6 py-2.5 rounded-lg bg-gradient-to-r from-sky-500 to-sky-600 text-white font-medium hover:shadow-glow disabled:opacity-50 disabled:cursor-not-allowed transition-all flex items-center gap-2"
          >
            {loading ? (
              <>
                <Loader2 className="animate-spin" size={16} />
                Creating...
              </>
            ) : (
              'Create Instance'
            )}
          </button>
        </div>
      </form>
    </Modal>
  );
}
