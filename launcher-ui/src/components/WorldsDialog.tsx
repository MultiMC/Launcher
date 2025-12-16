import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Globe, Trash2, FolderOpen, Loader2, Copy } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface World {
  id: string;
  name: string;
  lastPlayed: Date;
  gameMode: string;
}

interface WorldsDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instanceId: string;
  instanceName: string;
}

export function WorldsDialog({
  isOpen,
  onClose,
  instanceId,
  instanceName,
}: WorldsDialogProps) {
  const [worlds, setWorlds] = useState<World[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadWorlds();
    }
  }, [isOpen, instanceId]);

  const loadWorlds = async () => {
    setLoading(true);
    const worldList = await MinecraftAPI.getInstanceWorlds(instanceId);
    setWorlds(worldList);
    setLoading(false);
  };

  const handleOpenFolder = async () => {
    await MinecraftAPI.openInstanceFolder(instanceId);
  };

  const handleCopyWorld = (_worldId: string) => {
    alert('Copy world feature coming soon!');
  };

  const handleDeleteWorld = (_worldId: string) => {
    if (confirm('Are you sure you want to delete this world? This cannot be undone!')) {
      alert('Delete world feature coming soon!');
    }
  };

  const formatDate = (date: Date) => {
    const now = new Date();
    const diffMs = now.getTime() - new Date(date).getTime();
    const diffHours = Math.floor(diffMs / (1000 * 60 * 60));

    if (diffHours < 1) return 'Just now';
    if (diffHours < 24) return `${diffHours}h ago`;
    if (diffHours < 48) return 'Yesterday';
    return `${Math.floor(diffHours / 24)}d ago`;
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Worlds - ${instanceName}`} size="lg">
      <div className="space-y-6">
        {/* Action Buttons */}
        <div className="flex gap-3">
          <button
            onClick={handleOpenFolder}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600 transition-colors"
          >
            <FolderOpen size={18} />
            Open Saves Folder
          </button>
        </div>

        {/* Worlds List */}
        {loading ? (
          <div className="flex items-center justify-center py-12">
            <Loader2 className="animate-spin text-sky-400" size={32} />
          </div>
        ) : worlds.length === 0 ? (
          <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
            <Globe size={48} className="mx-auto text-gray-600 mb-3" />
            <p className="text-gray-400">No worlds found</p>
            <p className="text-sm text-gray-500 mt-1">Create a world in-game to see it here</p>
          </div>
        ) : (
          <div className="space-y-2">
            {worlds.map(world => (
              <div
                key={world.id}
                className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30"
              >
                <div className="flex items-center gap-3 flex-1">
                  <div className="w-12 h-12 rounded-lg bg-gradient-to-br from-green-500 to-green-600 flex items-center justify-center">
                    <Globe size={24} className="text-white" />
                  </div>
                  <div>
                    <div className="text-white font-medium">{world.name}</div>
                    <div className="text-sm text-gray-400">
                      {world.gameMode} • Last played {formatDate(world.lastPlayed)}
                    </div>
                  </div>
                </div>
                <div className="flex items-center gap-2">
                  <button
                    onClick={() => handleCopyWorld(world.id)}
                    className="p-2 rounded-lg bg-slate-700/50 text-gray-400 hover:bg-slate-700 hover:text-white transition-colors"
                    title="Copy world"
                  >
                    <Copy size={18} />
                  </button>
                  <button
                    onClick={() => handleDeleteWorld(world.id)}
                    className="p-2 rounded-lg bg-red-500/20 text-red-400 hover:bg-red-500/30 transition-colors"
                    title="Delete world"
                  >
                    <Trash2 size={18} />
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    </Modal>
  );
}
