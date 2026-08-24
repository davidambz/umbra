import type {
  AppSettings,
  LibraryItem,
  MonitorAssignment,
  MonitorInfo,
  Playlist,
  Theme,
  WallpaperType,
} from "../types";
import { scrubWallpaperFromAssignment } from "../assignmentUtils";
import { prefersDarkMediaQuery, systemThemeFromMediaQuery } from "../systemTheme";

/**
 * The contract this UI needs from the native side. src/ui/ui_bridge.* (#9)
 * implements the real version, injecting itself as `window.umbra` before
 * this page loads (mirroring how WebView2's `window.chrome.webview` host
 * objects are exposed). Until #9 exists — and for plain `npm run dev` in a
 * browser — createUiBridge() falls back to an in-memory mock so every
 * screen here is buildable and demoable without a native host at all.
 */
export interface UiBridge {
  getMonitors(): Promise<MonitorInfo[]>;
  getLibrary(): Promise<LibraryItem[]>;
  getAssignment(monitorId: string): Promise<MonitorAssignment>;
  getSettings(): Promise<AppSettings>;
  getTheme(): Promise<Theme>;
  /** Fires whenever the native host's Windows theme changes live. */
  onThemeChange(callback: (theme: Theme) => void): () => void;

  assignSingle(monitorId: string, wallpaperId: string, fpsCap: number): Promise<void>;
  assignPlaylist(monitorId: string, playlist: Playlist, fpsCap: number): Promise<void>;
  clearAssignment(monitorId: string): Promise<void>;

  /**
   * Kicks off the native "Add wallpaper" flow (file/folder picker + import
   * into library/, per library_manager.h). Returns the new item, or null
   * if the user cancelled or the import failed.
   */
  importWallpaper(title: string, type: WallpaperType): Promise<LibraryItem | null>;
  renameWallpaper(id: string, newTitle: string): Promise<void>;
  removeWallpaper(id: string): Promise<void>;

  updateSettings(patch: Partial<AppSettings>): Promise<void>;
}

declare global {
  interface Window {
    umbra?: UiBridge;
  }
}

export function createUiBridge(): UiBridge {
  return window.umbra ?? createMockUiBridge();
}

// ---- Mock implementation (dev/demo only) ------------------------------

const STORAGE_KEY = "umbra.mock-bridge.v1";

interface MockState {
  monitors: MonitorInfo[];
  library: LibraryItem[];
  assignments: Record<string, MonitorAssignment>;
  settings: AppSettings;
}

function defaultState(): MockState {
  return {
    monitors: [
      { id: "\\\\.\\DISPLAY1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
      { id: "\\\\.\\DISPLAY2", x: -1920, y: 0, width: 1920, height: 1080, isPrimary: false },
    ],
    library: [
      { id: "wp-nebula", title: "Nebula Drift", type: "video" },
      { id: "wp-rain", title: "Rainy Window", type: "video" },
      { id: "wp-pixel-forest", title: "Pixel Forest", type: "image" },
      { id: "wp-clock", title: "Minimal Clock", type: "web" },
    ],
    assignments: {
      "\\\\.\\DISPLAY1": { kind: "single", wallpaperId: "wp-nebula", fpsCap: 30 },
      "\\\\.\\DISPLAY2": { kind: "none" },
    },
    settings: {
      launchOnStartup: true,
      pauseOnFullscreen: true,
      pauseOnBattery: false,
      syncLockScreen: false,
    },
  };
}

function loadState(): MockState {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return defaultState();
    return { ...defaultState(), ...JSON.parse(raw) } as MockState;
  } catch {
    return defaultState();
  }
}

function saveState(state: MockState) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  } catch {
    // Best-effort only — a private/incognito webview shouldn't break the UI.
  }
}

// Not a counter: a counter reset by a page/module reload would collide
// with an id already sitting in localStorage from a previous session.
function nextMockId(): string {
  return `wp-mock-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
}

// Module-scoped (not per-bridge-instance) so creating multiple mock
// bridges — e.g. across a dev-server HMR reload — attaches the
// underlying matchMedia listener exactly once rather than stacking a new
// one every time that's never torn down.
const globalThemeListeners = new Set<(theme: Theme) => void>();
const media = prefersDarkMediaQuery();
media?.addEventListener?.("change", (event) => {
  const theme: Theme = event.matches ? "dark" : "light";
  globalThemeListeners.forEach((listener) => listener(theme));
});

function createMockUiBridge(): UiBridge {
  const state = loadState();

  return {
    async getMonitors() {
      return state.monitors;
    },
    async getLibrary() {
      // A copy, not the live array — importWallpaper/removeWallpaper
      // mutate state.library in place, and callers (App.tsx) build their
      // own next-state array by spreading whatever this returns; sharing
      // the same array reference would double an entry that's already
      // been pushed by the time the caller spreads it.
      return [...state.library];
    },
    async getAssignment(monitorId) {
      return state.assignments[monitorId] ?? { kind: "none" };
    },
    async getSettings() {
      return state.settings;
    },
    async getTheme() {
      return systemThemeFromMediaQuery(media);
    },
    onThemeChange(callback) {
      globalThemeListeners.add(callback);
      return () => globalThemeListeners.delete(callback);
    },
    async assignSingle(monitorId, wallpaperId, fpsCap) {
      state.assignments[monitorId] = { kind: "single", wallpaperId, fpsCap };
      saveState(state);
    },
    async assignPlaylist(monitorId, playlist, fpsCap) {
      state.assignments[monitorId] = { kind: "playlist", playlist, fpsCap };
      saveState(state);
    },
    async clearAssignment(monitorId) {
      state.assignments[monitorId] = { kind: "none" };
      saveState(state);
    },
    async importWallpaper(title, type) {
      // Mirrors library_manager.cpp's real ImportError::DestinationAlreadyExists
      // check (a title becomes a folder name, which can't collide) — thrown
      // rather than returning null, so callers (AddWallpaperDialog) can tell
      // this apart from a cancelled picker, which is what null means here.
      if (state.library.some((entry) => entry.title === title)) {
        throw new Error(`A wallpaper named "${title}" already exists.`);
      }
      const item: LibraryItem = { id: nextMockId(), title, type };
      state.library.push(item);
      saveState(state);
      return item;
    },
    async renameWallpaper(id, newTitle) {
      const item = state.library.find((entry) => entry.id === id);
      if (item) {
        item.title = newTitle;
        saveState(state);
      }
    },
    async removeWallpaper(id) {
      state.library = state.library.filter((entry) => entry.id !== id);
      for (const monitorId of Object.keys(state.assignments)) {
        state.assignments[monitorId] = scrubWallpaperFromAssignment(
          state.assignments[monitorId],
          id,
        );
      }
      saveState(state);
    },
    async updateSettings(patch) {
      state.settings = { ...state.settings, ...patch };
      saveState(state);
    },
  };
}
