import { motion } from 'framer-motion';
import { Plus, Minus, Zap, Battery } from 'lucide-react';
import { useState } from 'react';

export function StatusPanel() {
  const [temperature, setTemperature] = useState(25);
  const [autoMode, setAutoMode] = useState(true);
  const [solarPanels, setSolarPanels] = useState(true);
  const [powerReserve, setPowerReserve] = useState(false);

  return (
    <div className="space-y-4">
      {/* Temperature Control */}
      <motion.div
        initial={{ opacity: 0, x: 20 }}
        animate={{ opacity: 1, x: 0 }}
        className="glass-effect rounded-3xl p-6"
      >
        <h3 className="text-sm font-semibold text-gray-400 mb-4 uppercase tracking-wide">
          System Control
        </h3>

        <div className="space-y-4">
          {/* Temperature Display */}
          <div className="text-center mb-6">
            <div className="text-5xl font-bold text-white mb-2">
              {temperature}°C
            </div>
            <div className="text-gray-400 text-sm">Current: 19°C</div>
          </div>

          {/* Temperature Controls */}
          <div className="flex items-center justify-center gap-4 mb-4">
            <motion.button
              whileHover={{ scale: 1.1 }}
              whileTap={{ scale: 0.9 }}
              onClick={() => setTemperature(Math.max(10, temperature - 1))}
              className="w-12 h-12 rounded-xl bg-navy-800/50 border border-navy-700/50 flex items-center justify-center text-gray-400 hover:text-cyan-400 hover:border-cyan-500/50 transition-colors"
            >
              <Minus size={20} />
            </motion.button>

            <motion.button
              whileHover={{ scale: 1.1 }}
              whileTap={{ scale: 0.9 }}
              onClick={() => setTemperature(Math.min(35, temperature + 1))}
              className="w-12 h-12 rounded-xl bg-navy-800/50 border border-navy-700/50 flex items-center justify-center text-gray-400 hover:text-cyan-400 hover:border-cyan-500/50 transition-colors"
            >
              <Plus size={20} />
            </motion.button>
          </div>

          {/* Auto Mode Toggle */}
          <div className="flex items-center justify-between p-3 rounded-xl bg-navy-800/30">
            <span className="text-gray-300 text-sm">Automatic regulation</span>
            <button
              onClick={() => setAutoMode(!autoMode)}
              className={`
                relative w-12 h-6 rounded-full transition-colors
                ${autoMode ? 'bg-cyan-500' : 'bg-navy-700'}
              `}
            >
              <motion.div
                animate={{ x: autoMode ? 24 : 2 }}
                transition={{ type: 'spring', stiffness: 500, damping: 30 }}
                className="absolute top-1 w-4 h-4 rounded-full bg-white"
              />
            </button>
          </div>
        </div>
      </motion.div>

      {/* Energy Panel */}
      <motion.div
        initial={{ opacity: 0, x: 20 }}
        animate={{ opacity: 1, x: 0 }}
        transition={{ delay: 0.1 }}
        className="glass-effect rounded-3xl p-6"
      >
        <div className="flex items-center justify-between mb-4">
          <h3 className="text-sm font-semibold text-gray-400 uppercase tracking-wide">
            Energy
          </h3>
          <span className="text-cyan-400 text-sm">12.4 kWh</span>
        </div>

        <div className="space-y-3">
          {/* Solar Panels */}
          <div className="flex items-center justify-between p-3 rounded-xl bg-navy-800/30">
            <div className="flex items-center gap-3">
              <Zap size={18} className={solarPanels ? 'text-cyan-400' : 'text-gray-500'} />
              <span className="text-gray-300 text-sm">Solar panels</span>
            </div>
            <button
              onClick={() => setSolarPanels(!solarPanels)}
              className={`
                relative w-12 h-6 rounded-full transition-colors
                ${solarPanels ? 'bg-cyan-500' : 'bg-navy-700'}
              `}
            >
              <motion.div
                animate={{ x: solarPanels ? 24 : 2 }}
                transition={{ type: 'spring', stiffness: 500, damping: 30 }}
                className="absolute top-1 w-4 h-4 rounded-full bg-white"
              />
            </button>
          </div>

          {/* Power Reserve */}
          <div className="flex items-center justify-between p-3 rounded-xl bg-navy-800/30">
            <div className="flex items-center gap-3">
              <Battery size={18} className={powerReserve ? 'text-cyan-400' : 'text-gray-500'} />
              <span className="text-gray-300 text-sm">Power reserve</span>
            </div>
            <button
              onClick={() => setPowerReserve(!powerReserve)}
              className={`
                relative w-12 h-6 rounded-full transition-colors
                ${powerReserve ? 'bg-cyan-500' : 'bg-navy-700'}
              `}
            >
              <motion.div
                animate={{ x: powerReserve ? 24 : 2 }}
                transition={{ type: 'spring', stiffness: 500, damping: 30 }}
                className="absolute top-1 w-4 h-4 rounded-full bg-white"
              />
            </button>
          </div>
        </div>
      </motion.div>

      {/* Stays Panel */}
      <motion.div
        initial={{ opacity: 0, x: 20 }}
        animate={{ opacity: 1, x: 0 }}
        transition={{ delay: 0.2 }}
        className="glass-effect rounded-3xl p-6"
      >
        <h3 className="text-sm font-semibold text-gray-400 mb-4 uppercase tracking-wide">
          Quick Access
        </h3>

        <div className="space-y-2">
          {['Instances', 'Mods', 'Resource Packs', 'Screenshots', 'Logs'].map((item) => (
            <button
              key={item}
              className="w-full text-left px-4 py-3 rounded-xl text-gray-400 hover:text-gray-200 hover:bg-navy-800/50 transition-colors text-sm"
            >
              {item}
            </button>
          ))}
        </div>
      </motion.div>
    </div>
  );
}
