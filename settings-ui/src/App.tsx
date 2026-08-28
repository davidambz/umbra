import { useEffect, useMemo, useRef, useState } from "react";
import { createUiBridge } from "./bridge/uiBridge";
import { useSystemTheme } from "./bridge/useSystemTheme";
import { useSystemLanguage } from "./bridge/useSystemLanguage";
import { scrubWallpaperFromAssignment } from "./assignmentUtils";
import type {
  AppSettings,
  LibraryItem,
  MonitorAssignment,
  MonitorInfo,
  UpdateCheckResult,
} from "./types";
import { I18nProvider } from "./i18n/I18nProvider";
import { LOCALES } from "./i18n";
import { MonitorGrid } from "./components/MonitorGrid";
import { WallpaperLibrary } from "./components/WallpaperLibrary";
import { SettingsPanel } from "./components/SettingsPanel";
import { AssignDialog } from "./components/AssignDialog";
import { AddWallpaperDialog } from "./components/AddWallpaperDialog";
import { SettingsIcon, UmbraMark, WallpapersIcon } from "./components/icons";
import styles from "./App.module.css";

export default function App() {
  const bridge = useMemo(() => createUiBridge(), []);

  const [monitors, setMonitors] = useState<MonitorInfo[]>([]);
  const [library, setLibrary] = useState<LibraryItem[]>([]);
  const [assignments, setAssignments] = useState<Record<string, MonitorAssignment>>({});
  const [settings, setSettings] = useState<AppSettings | null>(null);
  useSystemTheme(bridge, settings?.themeOverride ?? "system");
  const locale = useSystemLanguage(bridge, settings?.languageOverride ?? "system");
  const t = LOCALES[locale];
  const [editingMonitor, setEditingMonitor] = useState<MonitorInfo | null>(null);
  const [quickAssignItem, setQuickAssignItem] = useState<LibraryItem | null>(null);
  const [addingWallpaper, setAddingWallpaper] = useState(false);
  const [loading, setLoading] = useState(true);
  const [loadError, setLoadError] = useState(false);
  const [appVersion, setAppVersion] = useState<string | null>(null);
  const [updateCheck, setUpdateCheck] = useState<UpdateCheckResult | null>(null);
  const [checkingForUpdate, setCheckingForUpdate] = useState(false);
  const [applyingUpdate, setApplyingUpdate] = useState(false);
  const [activeTab, setActiveTab] = useState<"wallpapers" | "settings">("wallpapers");
  const tabRefs = useRef<Record<"wallpapers" | "settings", HTMLButtonElement | null>>({
    wallpapers: null,
    settings: null,
  });

  // ARIA APG "automatic activation" tabs pattern: arrow keys both move
  // focus and switch the active tab, matching what a screen reader user
  // navigating a native <select>-like tablist expects.
  function handleTabKeyDown(event: React.KeyboardEvent) {
    if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;
    event.preventDefault();
    const next = activeTab === "wallpapers" ? "settings" : "wallpapers";
    setActiveTab(next);
    tabRefs.current[next]?.focus();
  }

  useEffect(() => {
    let cancelled = false;
    async function load() {
      try {
        const [loadedMonitors, loadedLibrary, loadedSettings, loadedAppVersion] = await Promise.all([
          bridge.getMonitors(),
          bridge.getLibrary(),
          bridge.getSettings(),
          bridge.getAppVersion(),
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
        setAppVersion(loadedAppVersion);
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

  // Re-fetches every monitor's assignment from the bridge rather than
  // trusting local state — needed whenever the native side may have
  // changed an assignment this UI didn't directly ask for, which is
  // exactly what settings.syncMonitors being on does (see
  // handleSaveAssignment/handleSettingsChange below).
  async function refreshAllAssignments() {
    const entries = await Promise.all(
      monitors.map(async (monitor) => [monitor.id, await bridge.getAssignment(monitor.id)] as const),
    );
    setAssignments(Object.fromEntries(entries));
  }

  async function handleSaveAssignment(
    monitorId: string,
    assignment: MonitorAssignment,
  ): Promise<boolean> {
    try {
      if (assignment.kind === "none") {
        await bridge.clearAssignment(monitorId);
      } else if (assignment.kind === "single") {
        await bridge.assignSingle(monitorId, assignment.wallpaperId, assignment.fpsCap);
      } else {
        await bridge.assignPlaylist(monitorId, assignment.playlist, assignment.fpsCap);
      }
    } catch (error) {
      console.error("Failed to save monitor assignment", error);
      return false;
    }
    if (settings?.syncMonitors) {
      // The native side just mirrored this to every other monitor too —
      // refetch everyone rather than only patching the one monitor the
      // user actually touched, which would leave every other
      // MonitorCard showing stale content.
      await refreshAllAssignments();
    } else {
      setAssignments((prev) => ({ ...prev, [monitorId]: assignment }));
    }
    return true;
  }

  // Lets a thrown error (e.g. a duplicate title) propagate to
  // AddWallpaperDialog, which needs to tell that apart from a plain
  // cancelled-picker (a falsy return, handled below) to show the right
  // message.
  async function handleImportWallpaper(
    title: string,
    type: "video" | "image" | "web",
  ): Promise<boolean> {
    const item = await bridge.importWallpaper(title, type);
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
    if (patch.syncMonitors) {
      // Turning it on just copied the primary's current assignment to
      // every other monitor on the native side — refetch so MonitorGrid
      // reflects that instead of showing whatever each one had before.
      await refreshAllAssignments();
    }
  }

  async function handleCheckForUpdate() {
    setCheckingForUpdate(true);
    try {
      setUpdateCheck(await bridge.checkForUpdate());
    } catch (error) {
      console.error("Failed to check for updates", error);
      setUpdateCheck({
        checkSucceeded: false,
        updateAvailable: false,
        latestVersion: "",
        downloadUrl: "",
        error: error instanceof Error ? error.message : String(error),
      });
    } finally {
      setCheckingForUpdate(false);
    }
  }

  async function handleApplyUpdate() {
    if (!updateCheck?.downloadUrl) return;
    setApplyingUpdate(true);
    try {
      // The native side kicks the download/install off on a background
      // thread and resolves this call almost immediately (see
      // Application::applyUpdate's comment) — it no longer reports
      // whether the download/install itself succeeds. Deliberately not
      // resetting applyingUpdate back to false here: a resolved call
      // means the update is under way and this window is about to be
      // closed by Restart Manager once the silent install finishes, so
      // re-enabling the button would just let a bored user fire off a
      // second, redundant download while the first is still running.
      // Only a thrown error (the bridge call itself failing, not the
      // update it kicked off) re-enables it, since nothing was started.
      await bridge.applyUpdate(updateCheck.downloadUrl);
    } catch (error) {
      console.error("Failed to apply the update", error);
      setApplyingUpdate(false);
    }
  }

  if (loadError) {
    return (
      <div className={styles.app}>
        <p className={styles.loading}>{t.app.loadError}</p>
      </div>
    );
  }

  if (loading || !settings) {
    return (
      <div className={styles.app}>
        <p className={styles.loading}>{t.app.loading}</p>
      </div>
    );
  }

  return (
    <I18nProvider locale={locale}>
      <div className={styles.app}>
        <header className={styles.header}>
          <div className={styles.nameplate}>
            <UmbraMark />
            <span className={styles.wordmark}>Umbra</span>
          </div>
          <nav
            className={styles.tabs}
            role="tablist"
            aria-label={t.app.tablistAriaLabel}
            onKeyDown={handleTabKeyDown}
          >
            <button
              type="button"
              id="tab-wallpapers"
              role="tab"
              ref={(node) => {
                tabRefs.current.wallpapers = node;
              }}
              aria-selected={activeTab === "wallpapers"}
              aria-controls="panel-wallpapers"
              tabIndex={activeTab === "wallpapers" ? 0 : -1}
              className={activeTab === "wallpapers" ? styles.tabActive : styles.tab}
              onClick={() => setActiveTab("wallpapers")}
            >
              <WallpapersIcon className={styles.tabIcon} />
              {t.app.tabWallpapers}
            </button>
            <button
              type="button"
              id="tab-settings"
              role="tab"
              ref={(node) => {
                tabRefs.current.settings = node;
              }}
              aria-selected={activeTab === "settings"}
              aria-controls="panel-settings"
              tabIndex={activeTab === "settings" ? 0 : -1}
              className={activeTab === "settings" ? styles.tabActive : styles.tab}
              onClick={() => setActiveTab("settings")}
            >
              <SettingsIcon className={styles.tabIcon} />
              {t.app.tabSettings}
            </button>
          </nav>
        </header>

        <main className={styles.main}>
          {/* Both panels stay mounted — switching tabs must not discard
              in-progress state in the hidden one (e.g. WallpaperCard's
              rename input, see settings-ui/src/components/WallpaperCard.tsx). */}
          <div
            id="panel-wallpapers"
            role="tabpanel"
            aria-labelledby="tab-wallpapers"
            hidden={activeTab !== "wallpapers"}
            className={styles.tabPanel}
          >
            <section>
              <MonitorGrid
                monitors={monitors}
                assignments={assignments}
                library={library}
                onEditMonitor={setEditingMonitor}
              />
            </section>

            <WallpaperLibrary
              library={library}
              monitors={monitors}
              assignments={assignments}
              onAdd={() => setAddingWallpaper(true)}
              onRename={handleRenameWallpaper}
              onRemove={handleRemoveWallpaper}
              onQuickAssign={setQuickAssignItem}
            />
          </div>

          <div
            id="panel-settings"
            role="tabpanel"
            aria-labelledby="tab-settings"
            hidden={activeTab !== "settings"}
            className={styles.tabPanel}
          >
            <SettingsPanel
              settings={settings}
              onChange={handleSettingsChange}
              appVersion={appVersion}
              updateCheck={updateCheck}
              checkingForUpdate={checkingForUpdate}
              applyingUpdate={applyingUpdate}
              onCheckForUpdate={handleCheckForUpdate}
              onApplyUpdate={handleApplyUpdate}
            />
          </div>
        </main>

        {editingMonitor && (
          <AssignDialog
            monitors={monitors}
            initialMonitorId={editingMonitor.id}
            assignments={assignments}
            library={library}
            syncMonitors={settings.syncMonitors}
            onSyncMonitorsChange={(checked) => handleSettingsChange({ syncMonitors: checked })}
            onClose={() => setEditingMonitor(null)}
            onSave={handleSaveAssignment}
          />
        )}

        {quickAssignItem && monitors.length > 0 && (
          <AssignDialog
            monitors={monitors}
            initialMonitorId={monitors.find((monitor) => monitor.isPrimary)?.id ?? monitors[0].id}
            assignments={assignments}
            library={library}
            monitorSelectable
            modeSelectable={false}
            initialMode="single"
            initialWallpaperId={quickAssignItem.id}
            syncMonitors={settings.syncMonitors}
            onSyncMonitorsChange={(checked) => handleSettingsChange({ syncMonitors: checked })}
            onClose={() => setQuickAssignItem(null)}
            onSave={handleSaveAssignment}
          />
        )}

        {addingWallpaper && (
          <AddWallpaperDialog
            onClose={() => setAddingWallpaper(false)}
            onImport={handleImportWallpaper}
          />
        )}
      </div>
    </I18nProvider>
  );
}
