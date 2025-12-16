import { useState } from 'react';
import { Modal } from './Modal';
import { Play, Settings, Trash2, FolderOpen, Package, Loader2 } from 'lucide-react';
import { Instance } from '../types';
import { motion } from 'framer-motion';

interface InstanceDetailsDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instance: Instance | null;
  onLaunch: (instanceId: string) => Promise<void>;
  onDelete: (instanceId: string) => Promise<void>;
}

export function InstanceDetailsDialog({
  isOpen,
  onClose,
  instance,
  onLaunch,
  onDelete,
}: InstanceDetailsDialogProps) {
  const [launching, setLaunching] = useState(false);
  const [deleting, setDeleting] = useState(false);

  if (!instance) return null;

  const handleLaunch = async () => {
    setLaunching(true);
    try {
      await onLaunch(instance.id);
      onClose();
    } catch (error) {
      console.error('Failed to launch:', error);
    } finally {
      setLaunching(false);
    }
  };

  const handleDelete = async () => {
    if (!confirm(`Are you sure you want to delete "${instance.name}"?`)) return;

    setDeleting(true);
    try {
      await onDelete(instance.id);
      onClose();
    } catch (error) {
      console.error('Failed to delete:', error);
    } finally {
      setDeleting(false);
    }
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={instance.name} size="lg">
      <div className="space-y-6">
        {/* Status Banner */}
        <div className={`
          p-4 rounded-lg flex items-center justify-between
          ${instance.status === 'running'
            ? 'bg-green-500/20 border border-green-500/50'
            : 'bg-slate-800/30 border border-slate-700/30'
          }
        `}>
          <div>
            <div className="flex items-center gap-2">
              <div className={`w-2 h-2 rounded-full ${
                instance.status === 'running' ? 'bg-green-400 animate-pulse' : 'bg-gray-500'
              }`} />
              <span className="text-white font-medium">
                {instance.status === 'running' ? 'Running' : 'Stopped'}
              </span>
            </div>
            <p className="text-sm text-gray-400 mt-1">
              Minecraft {instance.version}
            </p>
          </div>
          {instance.lastPlayed && (
            <span className="text-sm text-gray-400">
              Last played: {new Date(instance.lastPlayed).toLocaleDateString()}
            </span>
          )}
        </div>

        {/* Quick Actions */}
        <div className="grid grid-cols-2 gap-3">
          <motion.button
            whileHover={{ scale: 1.02 }}
            whileTap={{ scale: 0.98 }}
            onClick={handleLaunch}
            disabled={launching || instance.status === 'running'}
            className="flex items-center justify-center gap-2 px-6 py-4 rounded-lg bg-gradient-to-r from-sky-500 to-sky-600 text-white font-medium hover:shadow-glow disabled:opacity-50 disabled:cursor-not-allowed transition-all"
          >
            {launching ? (
              <>
                <Loader2 className="animate-spin" size={20} />
                Launching...
              </>
            ) : (
              <>
                <Play size={20} />
                {instance.status === 'running' ? 'Already Running' : 'Launch'}
              </>
            )}
          </motion.button>

          <button
            className="flex items-center justify-center gap-2 px-6 py-4 rounded-lg bg-slate-700/50 text-gray-300 hover:bg-slate-700 hover:text-white transition-colors"
          >
            <FolderOpen size={20} />
            Open Folder
          </button>
        </div>

        {/* Stats */}
        <div className="grid grid-cols-3 gap-4">
          <div className="bg-slate-800/30 rounded-lg p-4 border border-slate-700/30">
            <div className="text-2xl font-bold text-white">
              {instance.modCount || 0}
            </div>
            <div className="text-sm text-gray-400 mt-1">Mods</div>
          </div>
          <div className="bg-slate-800/30 rounded-lg p-4 border border-slate-700/30">
            <div className="text-2xl font-bold text-white">
              {instance.playTime ? Math.floor(instance.playTime / 60) : 0}h
            </div>
            <div className="text-sm text-gray-400 mt-1">Play Time</div>
          </div>
          <div className="bg-slate-800/30 rounded-lg p-4 border border-slate-700/30">
            <div className="text-2xl font-bold text-white">
              {instance.version}
            </div>
            <div className="text-sm text-gray-400 mt-1">Version</div>
          </div>
        </div>

        {/* Management Options */}
        <div className="space-y-2">
          <button className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors">
            <Package size={20} />
            <span>Manage Mods</span>
          </button>
          <button className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors">
            <Settings size={20} />
            <span>Instance Settings</span>
          </button>
          <button
            onClick={handleDelete}
            disabled={deleting}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-red-500/10 hover:bg-red-500/20 text-red-400 hover:text-red-300 transition-colors disabled:opacity-50"
          >
            {deleting ? (
              <>
                <Loader2 className="animate-spin" size={20} />
                <span>Deleting...</span>
              </>
            ) : (
              <>
                <Trash2 size={20} />
                <span>Delete Instance</span>
              </>
            )}
          </button>
        </div>
      </div>
    </Modal>
  );
}
