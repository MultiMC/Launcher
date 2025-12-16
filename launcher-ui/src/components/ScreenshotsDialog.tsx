import { useState, useEffect } from 'react';
import { Modal } from './Modal';
import { Image, FolderOpen, Trash2, Loader2, ExternalLink } from 'lucide-react';
import { MinecraftAPI } from '../services/api';

interface Screenshot {
  id: string;
  fileName: string;
  path: string;
  timestamp: Date;
}

interface ScreenshotsDialogProps {
  isOpen: boolean;
  onClose: () => void;
  instanceId: string;
  instanceName: string;
}

export function ScreenshotsDialog({
  isOpen,
  onClose,
  instanceId,
  instanceName,
}: ScreenshotsDialogProps) {
  const [screenshots, setScreenshots] = useState<Screenshot[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (isOpen) {
      loadScreenshots();
    }
  }, [isOpen, instanceId]);

  const loadScreenshots = async () => {
    setLoading(true);
    const screenshotList = await MinecraftAPI.getInstanceScreenshots(instanceId);
    setScreenshots(screenshotList);
    setLoading(false);
  };

  const handleOpenFolder = async () => {
    await MinecraftAPI.openInstanceFolder(instanceId);
  };

  const handleOpenScreenshot = (path: string) => {
    alert(`Would open screenshot: ${path}`);
  };

  const handleDeleteScreenshot = (_id: string) => {
    if (confirm('Are you sure you want to delete this screenshot?')) {
      alert('Delete screenshot feature coming soon!');
    }
  };

  const formatDate = (date: Date) => {
    return new Date(date).toLocaleString();
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title={`Screenshots - ${instanceName}`} size="xl">
      <div className="space-y-6">
        {/* Action Buttons */}
        <div className="flex gap-3">
          <button
            onClick={handleOpenFolder}
            className="flex items-center gap-2 px-4 py-2 rounded-lg bg-slate-700 text-white hover:bg-slate-600 transition-colors"
          >
            <FolderOpen size={18} />
            Open Screenshots Folder
          </button>
        </div>

        {/* Screenshots Grid */}
        {loading ? (
          <div className="flex items-center justify-center py-12">
            <Loader2 className="animate-spin text-sky-400" size={32} />
          </div>
        ) : screenshots.length === 0 ? (
          <div className="text-center py-12 bg-slate-800/30 rounded-lg border border-slate-700/30">
            <Image size={48} className="mx-auto text-gray-600 mb-3" />
            <p className="text-gray-400">No screenshots found</p>
            <p className="text-sm text-gray-500 mt-1">Take screenshots in-game with F2</p>
          </div>
        ) : (
          <div className="grid grid-cols-2 gap-4">
            {screenshots.map(screenshot => (
              <div
                key={screenshot.id}
                className="group relative rounded-lg overflow-hidden bg-slate-800/30 border border-slate-700/30 hover:border-sky-500/50 transition-all"
              >
                {/* Placeholder for screenshot preview */}
                <div className="aspect-video bg-gradient-to-br from-slate-700 to-slate-800 flex items-center justify-center">
                  <Image size={48} className="text-gray-600" />
                </div>

                {/* Info overlay */}
                <div className="absolute inset-0 bg-gradient-to-t from-black/80 to-transparent opacity-0 group-hover:opacity-100 transition-opacity flex flex-col justify-end p-4">
                  <div className="text-white text-sm font-medium mb-1">{screenshot.fileName}</div>
                  <div className="text-gray-300 text-xs mb-3">{formatDate(screenshot.timestamp)}</div>
                  <div className="flex gap-2">
                    <button
                      onClick={() => handleOpenScreenshot(screenshot.path)}
                      className="flex-1 flex items-center justify-center gap-2 px-3 py-2 rounded-lg bg-sky-500 text-white hover:bg-sky-600 transition-colors text-sm"
                    >
                      <ExternalLink size={14} />
                      Open
                    </button>
                    <button
                      onClick={() => handleDeleteScreenshot(screenshot.id)}
                      className="p-2 rounded-lg bg-red-500 text-white hover:bg-red-600 transition-colors"
                    >
                      <Trash2 size={14} />
                    </button>
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    </Modal>
  );
}
