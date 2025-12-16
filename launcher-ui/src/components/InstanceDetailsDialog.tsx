import { useState } from 'react';
import { Modal } from './Modal';
import { Play, Settings, Trash2, FolderOpen, Package, Loader2, Copy, Image, Box, Globe } from 'lucide-react';
import { Instance } from '../types';
import { motion } from 'framer-motion';
import { MinecraftAPI } from '../services/api';
import { ModManagementDialog } from './ModManagementDialog';
import { ResourcePacksDialog } from './ResourcePacksDialog';
import { ShaderPacksDialog } from './ShaderPacksDialog';
import { WorldsDialog } from './WorldsDialog';
import { ScreenshotsDialog } from './ScreenshotsDialog';
import { InstanceSettingsDialog } from './InstanceSettingsDialog';

interface InstanceDetailsDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instance: Instance | null;
  onLaunch: (instanceId: string) => Promise<void>;
  onDelete: (instanceId: string) => Promise<void>;
  onUpdate?: () => void;
}

export function InstanceDetailsDialog({
  isOpen,
  onClose,
  instance,
  onLaunch,
  onDelete,
  onUpdate,
}: InstanceDetailsDialogProps) {
  const [launching, setLaunching] = useState(false);
  const [deleting, setDeleting] = useState(false);
  const [showModsDialog, setShowModsDialog] = useState(false);
  const [showResourcePacksDialog, setShowResourcePacksDialog] = useState(false);
  const [showShadersDialog, setShowShadersDialog] = useState(false);
  const [showWorldsDialog, setShowWorldsDialog] = useState(false);
  const [showScreenshotsDialog, setShowScreenshotsDialog] = useState(false);
  const [showSettingsDialog, setShowSettingsDialog] = useState(false);

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

  const handleOpenFolder = async () => {
    await MinecraftAPI.openInstanceFolder(instance.id);
  };

  const handleCopyInstance = async () => {
    const newName = prompt(`Enter a name for the copy of "${instance.name}":`, `${instance.name} (Copy)`);
    if (!newName) return;

    const newId = await MinecraftAPI.copyInstance(instance.id, newName);
    if (newId) {
      onUpdate?.();
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

          <motion.button
            whileHover={{ scale: 1.02 }}
            whileTap={{ scale: 0.98 }}
            onClick={handleOpenFolder}
            className="flex items-center justify-center gap-2 px-6 py-4 rounded-lg bg-slate-700/50 text-gray-300 hover:bg-slate-700 hover:text-white transition-colors"
          >
            <FolderOpen size={20} />
            Open Folder
          </motion.button>
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
          <button
            onClick={() => setShowModsDialog(true)}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Package size={20} />
            <span>Manage Mods</span>
          </button>
          <button
            onClick={() => setShowResourcePacksDialog(true)}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Box size={20} />
            <span>Resource Packs</span>
          </button>
          <button
            onClick={() => setShowShadersDialog(true)}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Image size={20} />
            <span>Shader Packs</span>
          </button>
          <button
            onClick={() => setShowWorldsDialog(true)}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Globe size={20} />
            <span>Worlds</span>
          </button>
          <button
            onClick={() => setShowScreenshotsDialog(true)}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Image size={20} />
            <span>Screenshots</span>
          </button>
          <button
            onClick={() => setShowSettingsDialog(true)}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Settings size={20} />
            <span>Instance Settings</span>
          </button>
          <button
            onClick={handleCopyInstance}
            className="w-full flex items-center gap-3 px-4 py-3 rounded-lg bg-slate-800/30 hover:bg-slate-800/50 text-gray-300 hover:text-white transition-colors"
          >
            <Copy size={20} />
            <span>Duplicate Instance</span>
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

      {/* Sub-Dialogs */}
      {showModsDialog && instance && (
        <ModManagementDialog
          isOpen={showModsDialog}
          onClose={() => setShowModsDialog(false)}
          instanceId={instance.id}
          instanceName={instance.name}
          minecraftVersion={instance.version}
        />
      )}
      {showResourcePacksDialog && instance && (
        <ResourcePacksDialog
          isOpen={showResourcePacksDialog}
          onClose={() => setShowResourcePacksDialog(false)}
          instanceId={instance.id}
          instanceName={instance.name}
        />
      )}
      {showShadersDialog && instance && (
        <ShaderPacksDialog
          isOpen={showShadersDialog}
          onClose={() => setShowShadersDialog(false)}
          instanceId={instance.id}
          instanceName={instance.name}
        />
      )}
      {showWorldsDialog && instance && (
        <WorldsDialog
          isOpen={showWorldsDialog}
          onClose={() => setShowWorldsDialog(false)}
          instanceId={instance.id}
          instanceName={instance.name}
        />
      )}
      {showScreenshotsDialog && instance && (
        <ScreenshotsDialog
          isOpen={showScreenshotsDialog}
          onClose={() => setShowScreenshotsDialog(false)}
          instanceId={instance.id}
          instanceName={instance.name}
        />
      )}
      {showSettingsDialog && instance && (
        <InstanceSettingsDialog
          isOpen={showSettingsDialog}
          onClose={() => setShowSettingsDialog(false)}
          instanceId={instance.id}
          instanceName={instance.name}
        />
      )}
    </Modal>
  );
}
