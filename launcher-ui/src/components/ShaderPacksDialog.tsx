import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Image, Upload, Trash2, FolderOpen, Loader2 } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface ShaderPack {
  id: string;
  name: string;
  fileName: string;
}

interface ShaderPacksDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instanceId: string;
  instanceName: string;
}

export function ShaderPacksDialog({
  isOpen,
  onClose,
  instanceId,
  instanceName,
}: ShaderPacksDialogProps) {
  const [packs, setPacks] = useState<ShaderPack[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadShaderPacks();
    }
  }, [isOpen, instanceId]);

  const loadShaderPacks = async () => {
    setLoading(true);
    const packList = await MinecraftAPI.getInstanceShaderPacks(instanceId);
    setPacks(packList);
    setLoading(false);
  };

  const handleOpenFolder = async () => {
    await MinecraftAPI.openInstanceFolder(instanceId);
  };

  const handleUpload = () => {
    alert('File picker would open here to select shader pack files (.zip)');
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Shader Packs - ${instanceName}`} size="lg">
      <div className="space-y-6">
        {/* Info Banner */}
        <div className="bg-yellow-500/10 border border-yellow-500/30 rounded-lg p-4">
          <p className="text-sm text-yellow-200">
            <strong>Note:</strong> Shader packs require OptiFine or Iris to be installed.
          </p>
        </div>

        {/* Action Buttons */}
        <div className="flex gap-3">
          <button
            onClick={handleUpload}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors"
          >
            <Upload size={18} />
            Add Shader Pack
          </button>
          <button
            onClick={handleOpenFolder}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600 transition-colors"
          >
            <FolderOpen size={18} />
            Open Folder
          </button>
        </div>

        {/* Shader Packs List */}
        {loading ? (
          <div className="flex items-center justify-center py-12">
            <Loader2 className="animate-spin text-sky-400" size={32} />
          </div>
        ) : packs.length === 0 ? (
          <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
            <Image size={48} className="mx-auto text-gray-600 mb-3" />
            <p className="text-gray-400">No shader packs installed</p>
            <p className="text-sm text-gray-500 mt-1">Add shader packs for enhanced graphics and lighting</p>
          </div>
        ) : (
          <div className="space-y-2">
            {packs.map(pack => (
              <div
                key={pack.id}
                className="flex items-center justify-between p-4 rounded-lg bg-slate-800/30 border border-slate-700/30"
              >
                <div className="flex items-center gap-3">
                  <div className="w-12 h-12 rounded-lg bg-gradient-to-br from-cyan-500 to-cyan-600 flex items-center justify-center">
                    <Image size={24} className="text-white" />
                  </div>
                  <div>
                    <div className="text-white font-medium">{pack.name}</div>
                    <div className="text-sm text-gray-400">{pack.fileName}</div>
                  </div>
                </div>
                <button
                  className="p-2 rounded-lg bg-red-500/20 text-red-400 hover:bg-red-500/30 transition-colors"
                  title="Remove shader pack"
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
