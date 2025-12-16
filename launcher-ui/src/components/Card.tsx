import { motion } from 'framer-motion';
import { LucideIcon } from 'lucide-react';

interface CardProps {
  title: string;
  subtitle?: string;
  icon: LucideIcon;
  status: 'active' | 'deactivated';
  onClick?: () => void;
  className?: string;
}

export function Card({ title, subtitle, icon: Icon, status, onClick, className = '' }: CardProps) {
  const isActive = status === 'active';

  return (
    <motion.div
      whileHover={{ scale: 1.02, transition: { duration: 0.2 } }}
      whileTap={{ scale: 0.98 }}
      onClick={onClick}
      className={`
        relative overflow-hidden rounded-2xl p-6 cursor-pointer
        transition-all duration-300
        ${isActive
          ? 'bg-gradient-to-br from-slate-800/80 to-slate-900/80 border-2 border-sky-500/50 card-glow-active'
          : 'bg-slate-800/40 border border-slate-700/30 hover:border-slate-600/50'
        }
        backdrop-blur-sm
        ${className}
      `}
    >
      {/* Glow effect for active cards */}
      {isActive && (
        <div className="absolute inset-0 bg-gradient-to-br from-sky-500/10 to-transparent pointer-events-none" />
      )}

      {/* Icon */}
      <div className={`
        mb-4 flex items-center justify-center w-12 h-12 rounded-lg
        ${isActive ? 'bg-sky-500/20 text-sky-400' : 'bg-slate-700/50 text-navy-400'}
      `}>
        <Icon size={24} />
      </div>

      {/* Content */}
      <div className="relative z-10">
        <h3 className={`
          font-semibold text-lg mb-1
          ${isActive ? 'text-white' : 'text-gray-300'}
        `}>
          {title}
        </h3>
        {subtitle && (
          <p className={`
            text-sm
            ${isActive ? 'text-sky-400' : 'text-gray-500'}
          `}>
            {subtitle}
          </p>
        )}
      </div>

      {/* Status indicator */}
      <div className="absolute top-4 right-4">
        <div className={`
          w-2 h-2 rounded-full
          ${isActive ? 'bg-sky-400 shadow-glow-sm animate-pulse' : 'bg-gray-600'}
        `} />
      </div>
    </motion.div>
  );
}
