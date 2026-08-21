import { useEffect, useMemo, useState } from "react";
import { createUiBridge } from "./bridge/uiBridge";
import { useSystemTheme } from "./bridge/useSystemTheme";
import { scrubWallpaperFromAssignment } from "./assignmentUtils";
import type { AppSettings, LibraryItem, MonitorAssignment, MonitorInfo } from "./types";
import { MonitorGrid } from "./components/MonitorGrid";
import { WallpaperLibrary } from "./components/WallpaperLibrary";
import { SettingsPanel } from "./components/SettingsPanel";
import { AssignDialog } from "./components/AssignDialog";
import { AddWallpaperDialog } from "./components/AddWallpaperDialog";
import styles from "./App.module.css";

export default function App() {
  const bridge = useMemo(() => createUiBridge(), []);
  useSystemTheme(bridge);

  const [monitors, setMonitors] = useState<MonitorInfo[]>([]);
  const [library, setLibrary] = useState<LibraryItem[]>([]);
  const [assignments, setAssignments] = useState<Record<string, MonitorAssignment>>({});
  const [settings, setSettings] = useState<AppSettings | null>(null);
  const [editingMonitor, setEditingMonitor] = useState<MonitorInfo | null>(null);
  const [addingWallpaper, setAddingWallpaper] = useState(false);
  const [loading, setLoading] = useState(true);
  const [loadError, setLoadError] = useState(false);

  useEffect(() => {
    let cancelled = false;
    async function load() {
      try {
        const [loadedMonitors, loadedLibrary, loadedSettings] = await Promise.all([
          bridge.getMonitors(),
          bridge.getLibrary(),
          bridge.getSettings(),
        ]);
        if (cancelled) return;

        const assignmentEntries = await Promise.all(
          loadedMonitors.map(
            async (monitor) => [monitor.id, await bridge.getAssignment(monitor.id)] as const,
          ),
        );
        if (cancelled) return;

        setMonitors(loadedMonitors);
        setLibrary(loadedLibrary);
        setSettings(loadedSettings);
        setAssignments(Object.fromEntries(assignmentEntries));
        setLoading(false);
      } catch (error) {
        if (cancelled) return;
        console.error("Failed to load settings data", error);
        setLoadError(true);
        setLoading(false);
      }
    }
    load();
    return () => {
      cancelled = true;
    };
  }, [bridge]);

  async function handleSaveAssignment(
    monitor: MonitorInfo,
    assignment: MonitorAssignment,
  ): Promise<boolean> {
    try {
      if (assignment.kind === "none") {
        await bridge.clearAssignment(monitor.id);
      } else if (assignment.kind === "single") {
        await bridge.assignSingle(monitor.id, assignment.wallpaperId, assignment.fpsCap);
      } else {
        await bridge.assignPlaylist(monitor.id, assignment.playlist, assignment.fpsCap);
      }
    } catch (error) {
      console.error("Failed to save monitor assignment", error);
      return false;
    }
    setAssignments((prev) => ({ ...prev, [monitor.id]: assignment }));
    return true;
  }

  async function handleImportWallpaper(
    title: string,
    type: "video" | "image" | "web",
  ): Promise<boolean> {
    let item;
    try {
      item = await bridge.importWallpaper(title, type);
    } catch (error) {
      console.error("Failed to import wallpaper", error);
      return false;
    }
    if (!item) return false;
    setLibrary((prev) => [...prev, item]);
    return true;
  }

  async function handleRenameWallpaper(id: string, newTitle: string) {
    try {
      await bridge.renameWallpaper(id, newTitle);
    } catch (error) {
      console.error("Failed to rename wallpaper", error);
      return;
    }
    setLibrary((prev) => prev.map((item) => (item.id === id ? { ...item, title: newTitle } : item)));
  }

  async function handleRemoveWallpaper(id: string) {
    try {
      await bridge.removeWallpaper(id);
    } catch (error) {
      console.error("Failed to remove wallpaper", error);
      return;
    }
    setLibrary((prev) => prev.filter((item) => item.id !== id));
    setAssignments((prev) => {
      const next: Record<string, MonitorAssignment> = {};
      for (const [monitorId, assignment] of Object.entries(prev)) {
        next[monitorId] = scrubWallpaperFromAssignment(assignment, id);
      }
      return next;
    });
  }

  async function handleSettingsChange(patch: Partial<AppSettings>) {
    try {
      await bridge.updateSettings(patch);
    } catch (error) {
      console.error("Failed to update settings", error);
      return;
    }
    setSettings((prev) => (prev ? { ...prev, ...patch } : prev));
  }

  if (loadError) {
    return (
      <div className={styles.app}>
        <p className={styles.loading}>Couldn't load Umbra's settings. Try reopening this window.</p>
      </div>
    );
  }

  if (loading || !settings) {
    return (
      <div className={styles.app}>
        <p className={styles.loading}>Loading…</p>
      </div>
    );
  }

  return (
    <div className={styles.app}>
      <header className={styles.header}>
        <h1 className={styles.wordmark}>Umbra</h1>
      </header>

      <main className={styles.main}>
        <section>
          <h2 className={styles.sectionHeading}>Your displays</h2>
          <MonitorGrid
            monitors={monitors}
            assignments={assignments}
            library={library}
            onEditMonitor={setEditingMonitor}
          />
        </section>

        <WallpaperLibrary
          library={library}
          onAdd={() => setAddingWallpaper(true)}
          onRename={handleRenameWallpaper}
          onRemove={handleRemoveWallpaper}
        />

        <SettingsPanel settings={settings} onChange={handleSettingsChange} />
      </main>

      {editingMonitor && (
        <AssignDialog
          monitor={editingMonitor}
          displayIndex={monitors.findIndex((m) => m.id === editingMonitor.id) + 1}
          assignment={assignments[editingMonitor.id] ?? { kind: "none" }}
          library={library}
          onClose={() => setEditingMonitor(null)}
          onSave={(assignment) => handleSaveAssignment(editingMonitor, assignment)}
        />
      )}

      {addingWallpaper && (
        <AddWallpaperDialog
          onClose={() => setAddingWallpaper(false)}
          onImport={handleImportWallpaper}
        />
      )}
    </div>
  );
}
