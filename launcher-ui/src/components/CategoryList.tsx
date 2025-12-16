import { motion } from 'framer-motion';
import * as Icons from 'lucide-react';

interface Category {
  id: string;
  name: string;
  icon: string;
  count?: number;
}

interface CategoryListProps {
  title: string;
  categories: Category[];
  activeCategory?: string;
  onCategoryClick?: (id: string) => void;
}

export function CategoryList({ title, categories, activeCategory, onCategoryClick }: CategoryListProps) {
  const getIcon = (iconName: string) => {
    const Icon = (Icons as any)[iconName];
    return Icon || Icons.Folder;
  };

  return (
    <div className="glass-effect rounded-3xl p-6">
      <h3 className="text-sm font-semibold text-gray-400 mb-4 uppercase tracking-wide">
        {title}
      </h3>

      <div className="space-y-2">
        {categories.map((category) => {
          const Icon = getIcon(category.icon);
          const isActive = activeCategory === category.id;

          return (
            <motion.button
              key={category.id}
              whileHover={{ x: 4 }}
              whileTap={{ scale: 0.98 }}
              onClick={() => onCategoryClick?.(category.id)}
              className={`
                w-full flex items-center gap-3 px-4 py-3 rounded-xl
                transition-all duration-200
                ${isActive
                  ? 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/50'
                  : 'text-gray-400 hover:text-gray-200 hover:bg-navy-800/50'
                }
              `}
            >
              <Icon size={20} />
              <span className="flex-1 text-left font-medium">{category.name}</span>
              {category.count !== undefined && (
                <span className={`
                  text-xs px-2 py-1 rounded-full
                  ${isActive ? 'bg-cyan-500/30 text-cyan-300' : 'bg-navy-700/50 text-gray-500'}
                `}>
                  {category.count}
                </span>
              )}
            </motion.button>
          );
        })}
      </div>
    </div>
  );
}
