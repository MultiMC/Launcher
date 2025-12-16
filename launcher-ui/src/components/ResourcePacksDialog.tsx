import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Box, Upload, Trash2, FolderOpen, Loader2 } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface ResourcePack {
  id: string;
  name: string;
  fileName: string;
}

interface ResourcePacksDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instanceId: string;
  instanceName: string;
}

export function ResourcePacksDialog({
  isOpen,
  onClose,
  instanceId,
  instanceName,
}: ResourcePacksDialogProps) {
  const [packs, setPacks] = useState<ResourcePack[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadResourcePacks();
    }
  }, [isOpen, instanceId]);

  const loadResourcePacks = async () => {
    setLoading(true);
    const packList = await MinecraftAPI.getInstanceResourcePacks(instanceId);
    setPacks(packList);
    setLoading(false);
  };

  const handleOpenFolder = async () => {
    // Open resource packs folder
    await MinecraftAPI.openInstanceFolder(instanceId);
  };

  const handleUpload = () => {
    alert('File picker would open here to select resource pack files (.zip)');
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Resource Packs - ${instanceName}`} size="lg">
      <div className="space-y-6">
        {/* Action Buttons */}
        <div className="flex gap-3">
          <button
            onClick={handleUpload}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors"
          >
            <Upload size={18} />
            Add Resource Pack
          </button>
          <button
            onClick={handleOpenFolder}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600 transition-colors"
          >
            <FolderOpen size={18} />
            Open Folder
          </button>
        </div>

        {/* Resource Packs List */}
        {loading ? (
          <div className="flex items-center justify-center py-12">
            <Loader2 className="animate-spin text-sky-400" size={32} />
          </div>
        ) : packs.length === 0 ? (
          <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
            <Box size={48} className="mx-auto text-gray-600 mb-3" />
            <p className="text-gray-400">No resource packs installed</p>
            <p className="text-sm text-gray-500 mt-1">Add resource packs to customize textures and sounds</p>
          </div>
        ) : (
          <div className="space-y-2">
            {packs.map(pack => (
              <div
                key={pack.id}
                className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30"
              >
                <div className="flex items-center gap-3">
                  <div className="w-12 h-12 rounded-lg bg-gradient-to-br from-purple-500 to-purple-600 flex items-center justify-center">
                    <Box size={24} className="text-white" />
                  </div>
                  <div>
                    <div className="text-white font-medium">{pack.name}</div>
                    <div className="text-sm text-gray-400">{pack.fileName}</div>
                  </div>
                </div>
                <button
                  className="p-2 rounded-lg bg-red-500/20 text-red-400 hover:bg-red-500/30 transition-colors"
                  title="Remove resource pack"
                >
                  <Trash2 size={18} />
                </button>
              </div>
            ))}
          </div>
        )}
      </div>
    </Modal>
  );
}
