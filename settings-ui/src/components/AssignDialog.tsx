import { useState } from "react";
import type { LibraryItem, MonitorAssignment, MonitorInfo, Playlist } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import { PlaylistEditor } from "./PlaylistEditor";
import styles from "./AssignDialog.module.css";

type Mode = "none" | "single" | "playlist";

interface AssignDialogProps {
  monitor: MonitorInfo;
  displayIndex: number;
  assignment: MonitorAssignment;
  library: LibraryItem[];
  onClose: () => void;
  onSave: (assignment: MonitorAssignment) => void;
}

const FPS_OPTIONS = [15, 30, 60];

function defaultPlaylist(seed: MonitorAssignment): Playlist {
  if (seed.kind === "playlist") return seed.playlist;
  return { wallpaperIds: [], intervalSeconds: 300, mode: "sequential" };
}

export function AssignDialog({
  monitor,
  displayIndex,
  assignment,
  library,
  onClose,
  onSave,
}: AssignDialogProps) {
  const [mode, setMode] = useState<Mode>(assignment.kind);
  const [wallpaperId, setWallpaperId] = useState(
    assignment.kind === "single" ? assignment.wallpaperId : library[0]?.id ?? "",
  );
  const [playlist, setPlaylist] = useState<Playlist>(defaultPlaylist(assignment));
  const [fpsCap, setFpsCap] = useState(assignment.kind === "none" ? 30 : assignment.fpsCap);

  function handleSave() {
    if (mode === "none") {
      onSave({ kind: "none" });
    } else if (mode === "single") {
      if (!wallpaperId) return;
      onSave({ kind: "single", wallpaperId, fpsCap });
    } else {
      if (playlist.wallpaperIds.length === 0) return;
      onSave({ kind: "playlist", playlist, fpsCap });
    }
    onClose();
  }

  const label = monitor.isPrimary ? "Primary display" : `Display ${displayIndex}`;
  const saveDisabled =
    (mode === "single" && !wallpaperId) || (mode === "playlist" && playlist.wallpaperIds.length === 0);

  return (
    <Dialog
      title={label}
      onClose={onClose}
      footer={
        <>
          <Button variant="ghost" onClick={onClose}>
            Cancel
          </Button>
          <Button variant="primary" onClick={handleSave} disabled={saveDisabled}>
            Save
          </Button>
        </>
      }
    >
      <div className={styles.modeTabs}>
        <button
          type="button"
          className={mode === "none" ? styles.modeTabActive : styles.modeTab}
          onClick={() => setMode("none")}
        >
          None
        </button>
        <button
          type="button"
          className={mode === "single" ? styles.modeTabActive : styles.modeTab}
          onClick={() => setMode("single")}
          disabled={library.length === 0}
        >
          Single wallpaper
        </button>
        <button
          type="button"
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
        <label className={styles.fpsField}>
          <span className={styles.fpsLabel}>FPS cap</span>
          <div className={styles.fpsOptions}>
            {FPS_OPTIONS.map((option) => (
              <button
                type="button"
                key={option}
                className={fpsCap === option ? styles.fpsOptionActive : styles.fpsOption}
                onClick={() => setFpsCap(option)}
              >
                {option}
              </button>
            ))}
          </div>
        </label>
      )}
    </Dialog>
  );
}
