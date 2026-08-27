import { useState } from "react";
import type { LibraryItem, MonitorAssignment, MonitorInfo, Playlist } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import { PlaylistEditor } from "./PlaylistEditor";
import { scrubStaleReferences } from "../assignmentUtils";
import { monitorDisplayLabel } from "../monitorLabels";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import styles from "./AssignDialog.module.css";

type Mode = "none" | "single" | "playlist";

interface AssignDialogProps {
  monitors: MonitorInfo[];
  initialMonitorId: string;
  /** Every monitor's current assignment, keyed by monitor id — not just initialMonitorId's,
   * since monitorSelectable lets the target change after mount and the FPS cap default
   * needs to follow whichever monitor is actually selected at that point. */
  assignments: Record<string, MonitorAssignment>;
  library: LibraryItem[];
  /** Shows a monitor picker so the target isn't locked to initialMonitorId — used when
   * opening this dialog from the library (WallpaperCard double-click) rather than from
   * a specific MonitorGrid tile, where there's no monitor context to begin with. */
  monitorSelectable?: boolean;
  /** Hides the None/Single/Playlist mode tabs — used by the library quick-assign flow,
   * which is always "single" and has no reason to offer switching away from that. */
  modeSelectable?: boolean;
  /** Pre-selects a mode/wallpaper other than what `assignment` implies — used by the
   * library quick-assign flow to jump straight to "single" with that wallpaper chosen. */
  initialMode?: Mode;
  initialWallpaperId?: string;
  onClose: () => void;
  /** Resolves to whether the save actually succeeded — the dialog stays open on failure. */
  onSave: (monitorId: string, assignment: MonitorAssignment) => Promise<boolean>;
}

const FPS_OPTIONS = [15, 30, 60];

function initialWallpaperId(assignment: MonitorAssignment, library: LibraryItem[]): string {
  const scrubbed = scrubStaleReferences(assignment, library);
  return scrubbed.kind === "single" ? scrubbed.wallpaperId : library[0]?.id ?? "";
}

function initialPlaylist(assignment: MonitorAssignment, library: LibraryItem[]): Playlist {
  const scrubbed = scrubStaleReferences(assignment, library);
  return scrubbed.kind === "playlist"
    ? scrubbed.playlist
    : { wallpaperIds: [], intervalSeconds: 300, mode: "sequential" };
}

export function AssignDialog({
  monitors,
  initialMonitorId,
  assignments,
  library,
  monitorSelectable = false,
  modeSelectable = true,
  initialMode,
  initialWallpaperId: presetWallpaperId,
  onClose,
  onSave,
}: AssignDialogProps) {
  const initialAssignment = assignments[initialMonitorId] ?? { kind: "none" };
  const [monitorId, setMonitorId] = useState(initialMonitorId);
  const [mode, setMode] = useState<Mode>(initialMode ?? initialAssignment.kind);
  const [wallpaperId, setWallpaperId] = useState(
    () => presetWallpaperId ?? initialWallpaperId(initialAssignment, library),
  );
  const [playlist, setPlaylist] = useState<Playlist>(() =>
    initialPlaylist(initialAssignment, library),
  );
  const [fpsCap, setFpsCap] = useState(
    initialAssignment.kind === "none" ? 30 : initialAssignment.fpsCap,
  );
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState(false);

  // Only relevant when a monitor picker is shown (monitorSelectable) — the FPS cap
  // should follow whichever monitor gets picked rather than staying pinned to
  // whatever initialMonitorId's cap happened to be, so switching to a monitor
  // that already has a custom cap doesn't silently save over it with a stale
  // default. Set from the click that changes monitorId, not a derived effect.
  function selectMonitor(id: string) {
    setMonitorId(id);
    if (!monitorSelectable) return;
    const current = assignments[id];
    setFpsCap(current && current.kind !== "none" ? current.fpsCap : 30);
  }

  // Ignored while a save is in flight — otherwise closing mid-save
  // doesn't stop onSave from still applying the assignment once it
  // resolves after the dialog is already gone.
  function guardedClose() {
    if (!saving) onClose();
  }

  async function handleSave() {
    let next: MonitorAssignment;
    if (mode === "none") {
      next = { kind: "none" };
    } else if (mode === "single") {
      if (!wallpaperId) return;
      next = { kind: "single", wallpaperId, fpsCap };
    } else {
      if (playlist.wallpaperIds.length === 0) return;
      next = { kind: "playlist", playlist, fpsCap };
    }

    setSaving(true);
    setSaveError(false);
    const succeeded = await onSave(monitorId, next);
    setSaving(false);
    if (succeeded) {
      onClose();
    } else {
      setSaveError(true);
    }
  }

  const selectedMonitorIndex = monitors.findIndex((monitor) => monitor.id === monitorId);
  const label = monitorDisplayLabel(selectedMonitorIndex === -1 ? 1 : selectedMonitorIndex + 1);
  const saveDisabled =
    saving ||
    (mode === "single" && !wallpaperId) ||
    (mode === "playlist" && playlist.wallpaperIds.length === 0);

  return (
    <Dialog
      title={label}
      onClose={guardedClose}
      footer={
        <>
          <Button variant="ghost" onClick={guardedClose} disabled={saving}>
            Cancel
          </Button>
          <Button variant="primary" onClick={handleSave} disabled={saveDisabled}>
            {saving ? "Saving…" : "Save"}
          </Button>
        </>
      }
    >
      {monitorSelectable && monitors.length > 1 && (
        <div className={styles.monitorField}>
          <span className={styles.monitorLabel} id="assign-target-monitor-label">
            Assign to
          </span>
          <div
            className={styles.monitorOptions}
            role="radiogroup"
            aria-labelledby="assign-target-monitor-label"
            onKeyDown={handleRadioGroupKeyDown}
          >
            {monitors.map((monitor, index) => (
              <button
                type="button"
                role="radio"
                aria-checked={monitorId === monitor.id}
                tabIndex={monitorId === monitor.id ? 0 : -1}
                key={monitor.id}
                className={monitorId === monitor.id ? styles.monitorOptionActive : styles.monitorOption}
                onClick={() => selectMonitor(monitor.id)}
              >
                {monitorDisplayLabel(index + 1)}
                {monitor.isPrimary && (
                  <span className={styles.monitorPrimaryTag}> Primary</span>
                )}
              </button>
            ))}
          </div>
        </div>
      )}

      {modeSelectable && (
        <div
          className={styles.modeTabs}
          role="radiogroup"
          aria-label="Assignment mode"
          onKeyDown={handleRadioGroupKeyDown}
        >
          <button
            type="button"
            role="radio"
            aria-checked={mode === "none"}
            tabIndex={mode === "none" ? 0 : -1}
            className={mode === "none" ? styles.modeTabActive : styles.modeTab}
            onClick={() => setMode("none")}
          >
            None
          </button>
          <button
            type="button"
            role="radio"
            aria-checked={mode === "single"}
            tabIndex={mode === "single" ? 0 : -1}
            className={mode === "single" ? styles.modeTabActive : styles.modeTab}
            onClick={() => setMode("single")}
            disabled={library.length === 0}
          >
            Single wallpaper
          </button>
          <button
            type="button"
            role="radio"
            aria-checked={mode === "playlist"}
            tabIndex={mode === "playlist" ? 0 : -1}
            className={mode === "playlist" ? styles.modeTabActive : styles.modeTab}
            onClick={() => setMode("playlist")}
            disabled={library.length === 0}
          >
            Playlist
          </button>
        </div>
      )}

      {modeSelectable && library.length === 0 && (
        <p className={styles.hint}>Add a wallpaper to the library first, then assign it here.</p>
      )}

      {modeSelectable && mode === "single" && library.length > 0 && (
        <div className={styles.singlePicker}>
          {library.map((item) => (
            <label key={item.id} className={styles.pickerRow}>
              <input
                type="radio"
                name="wallpaper"
                checked={wallpaperId === item.id}
                onChange={() => setWallpaperId(item.id)}
              />
              {item.title}
            </label>
          ))}
        </div>
      )}

      {mode === "playlist" && (
        <PlaylistEditor playlist={playlist} library={library} onChange={setPlaylist} />
      )}

      {mode !== "none" && (
        <div className={styles.fpsField}>
          <span className={styles.fpsLabel} id="fps-cap-label">
            FPS cap
          </span>
          <div
            className={styles.fpsOptions}
            role="radiogroup"
            aria-labelledby="fps-cap-label"
            onKeyDown={handleRadioGroupKeyDown}
          >
            {FPS_OPTIONS.map((option) => (
              <button
                type="button"
                role="radio"
                aria-checked={fpsCap === option}
                tabIndex={fpsCap === option ? 0 : -1}
                key={option}
                className={fpsCap === option ? styles.fpsOptionActive : styles.fpsOption}
                onClick={() => setFpsCap(option)}
              >
                {option}
              </button>
            ))}
          </div>
        </div>
      )}

      {saveError && (
        <p className={styles.error}>Couldn't save this — check the connection and try again.</p>
      )}
    </Dialog>
  );
}
