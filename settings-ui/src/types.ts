// Mirrors the C++ core types this UI is a view over — src/config/wallpaper_profile.h,
// src/config/settings.h, and src/playlist/playlist.h — so the shapes ui_bridge (#9)
// sends across stay in sync with what the native side actually persists.

export type WallpaperType = "video" | "image" | "web";

export interface MonitorInfo {
  id: string;
  x: number;
  y: number;
  width: number;
  height: number;
  isPrimary: boolean;
}

/** One entry in the imported wallpaper library (independent of any monitor assignment). */
export interface LibraryItem {
  id: string;
  title: string;
  type: WallpaperType;
  /** Data URL or file:// path to a preview frame; undefined until a thumbnail exists. */
  thumbnailUrl?: string;
}

export type PlaylistMode = "sequential" | "shuffle";

export interface Playlist {
  wallpaperIds: string[];
  intervalSeconds: number;
  mode: PlaylistMode;
}

/** What's currently assigned to one monitor: nothing, a single wallpaper, or a playlist. */
export type MonitorAssignment =
  | { kind: "none" }
  | { kind: "single"; wallpaperId: string; fpsCap: number }
  | { kind: "playlist"; playlist: Playlist; fpsCap: number };

export interface AppSettings {
  launchOnStartup: boolean;
  pauseOnFullscreen: boolean;
  pauseOnBattery: boolean;
  syncLockScreen: boolean;
  /** While on, assigning a wallpaper to any monitor mirrors it to every other connected monitor. */
  syncMonitors: boolean;
  /** "system" (default) follows the live Windows theme; "light"/"dark" pins it. */
  themeOverride: ThemeOverride;
  /** "system" (default) follows the OS UI language; otherwise pins one Locale. */
  languageOverride: LanguageOverride;
}

export type Theme = "light" | "dark";
export type ThemeOverride = Theme | "system";

/** Every locale settings-ui ships strings for — src/i18n/locales/*.ts, one file per tag. */
export type Locale = "en" | "pt-BR" | "es" | "zh-CN" | "fr" | "ru" | "ja" | "ko";
export type LanguageOverride = Locale | "system";
