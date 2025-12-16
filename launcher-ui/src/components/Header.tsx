import { Search, User } from 'lucide-react';

interface HeaderProps {
  title: string;
  onSearch?: (query: string) => void;
}

export function Header({ title, onSearch }: HeaderProps) {
  return (
    <div className="flex items-center justify-between mb-8">
      <h1 className="text-3xl font-bold text-white">{title}</h1>

      <div className="flex items-center gap-4">
        {/* Search Bar */}
        <div className="relative">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" size={18} />
          <input
            type="text"
            placeholder="Search..."
            onChange={(e) => onSearch?.(e.target.value)}
            className="
              w-64 pl-10 pr-4 py-2 rounded-lg
              bg-slate-800/50 border border-slate-700/50
              text-gray-200 placeholder-gray-500
              focus:outline-none focus:border-sky-500/50 focus:ring-1 focus:ring-sky-500/50
              transition-all duration-200
            "
          />
        </div>

        {/* User Profile */}
        <button className="w-10 h-10 rounded-full bg-gradient-to-br from-sky-500 to-sky-600 flex items-center justify-center text-white hover:shadow-glow transition-all duration-200">
          <User size={20} />
        </button>
      </div>
    </div>
  );
}
