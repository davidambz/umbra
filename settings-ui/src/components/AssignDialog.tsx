import { useState } from "react";
import type { LibraryItem, MonitorAssignment, Playlist } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import { PlaylistEditor } from "./PlaylistEditor";
import { scrubStaleReferences } from "../assignmentUtils";
import { monitorDisplayLabel } from "../monitorLabels";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import styles from "./AssignDialog.module.css";

type Mode = "none" | "single" | "playlist";

interface AssignDialogProps {
  displayIndex: number;
  assignment: MonitorAssignment;
  library: LibraryItem[];
  onClose: () => void;
  /** Resolves to whether the save actually succeeded — the dialog stays open on failure. */
  onSave: (assignment: MonitorAssignment) => Promise<boolean>;
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
  displayIndex,
  assignment,
  library,
  onClose,
  onSave,
}: AssignDialogProps) {
  const [mode, setMode] = useState<Mode>(assignment.kind);
  const [wallpaperId, setWallpaperId] = useState(() => initialWallpaperId(assignment, library));
  const [playlist, setPlaylist] = useState<Playlist>(() => initialPlaylist(assignment, library));
  const [fpsCap, setFpsCap] = useState(assignment.kind === "none" ? 30 : assignment.fpsCap);
  const [saving, setSaving] = useState(false);
  const [saveError, setSaveError] = useState(false);

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
    const succeeded = await onSave(next);
    setSaving(false);
    if (succeeded) {
      onClose();
    } else {
      setSaveError(true);
    }
  }

  const label = monitorDisplayLabel(displayIndex);
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

      {library.length === 0 && (
        <p className={styles.hint}>Add a wallpaper to the library first, then assign it here.</p>
      )}

      {mode === "single" && library.length > 0 && (
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
