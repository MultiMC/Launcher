# MultiMC Modern Frontend

A modern, cross-platform frontend for MultiMC built with Tauri, React, and TypeScript.

## 🎨 Features

- **Modern UI Design**: Card-based interface with smooth animations
- **Cross-Platform**: Works on Windows, Linux, and macOS  
- **Lightweight**: Built with Tauri for minimal resource usage
- **Dark Theme**: Eye-friendly dark theme with cyan accents
- **Smooth Animations**: Powered by Framer Motion

## 🏗️ Technology Stack

- React 19 with TypeScript
- Tauri 2.x (Rust-based)
- Vite build tool
- Tailwind CSS for styling
- Framer Motion for animations
- Lucide React for icons

## 🚀 Getting Started

### Prerequisites

- Node.js 18+
- Rust and Cargo
- Platform-specific dependencies (see Tauri docs)

### Installation

```bash
npm install
npm run tauri dev
```

### Build for production

```bash
npm run tauri build
```

## 📁 Project Structure

```
src/
├── components/       # React components
├── services/        # API services  
├── types/          # TypeScript types
├── App.tsx         # Main app
└── index.css       # Global styles

src-tauri/          # Rust backend
```

## 🔌 Backend Integration

The frontend uses Tauri commands to communicate with the backend.
See src/services/api.ts for available API methods.

