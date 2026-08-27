import { useState } from "react";
import type { WallpaperType } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import styles from "./AddWallpaperDialog.module.css";

interface AddWallpaperDialogProps {
  onClose: () => void;
  /** Resolves to whether the import actually happened — the dialog stays open if not (e.g. the user cancelled the file picker). Throws (e.g. a duplicate title) instead of resolving falsy for an actual failure. */
  onImport: (title: string, type: WallpaperType) => Promise<boolean>;
}

const TYPE_OPTIONS: { value: WallpaperType; label: string; hint: string }[] = [
  { value: "video", label: "Video", hint: "An mp4 or webm file" },
  { value: "image", label: "Image", hint: "A gif, apng, or still image" },
  { value: "web", label: "Web", hint: "A folder or .zip with an index.html" },
];

export function AddWallpaperDialog({ onClose, onImport }: AddWallpaperDialogProps) {
  const [title, setTitle] = useState("");
  const [type, setType] = useState<WallpaperType>("video");
  const [busy, setBusy] = useState(false);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);
  // Cancelling the file picker isn't an error — only a thrown import
  // failure should read as one (and get role="alert" for immediate
  // announcement instead of the calmer role="status").
  const [statusIsError, setStatusIsError] = useState(false);

  // Ignored while an import is in flight — otherwise closing mid-import
  // doesn't stop it from completing and adding the wallpaper anyway once
  // it resolves after the dialog is already gone.
  function guardedClose() {
    if (!busy) onClose();
  }

  async function handleImport() {
    const trimmed = title.trim();
    if (!trimmed || busy) return;
    setBusy(true);
    setStatusMessage(null);
    setStatusIsError(false);
    try {
      const imported = await onImport(trimmed, type);
      if (imported) {
        onClose();
        return;
      }
      setStatusMessage("Import didn't complete — no file was chosen.");
    } catch (error) {
      setStatusIsError(true);
      setStatusMessage(error instanceof Error ? error.message : "Import failed.");
    } finally {
      setBusy(false);
    }
  }

  return (
    <Dialog
      title="Add wallpaper"
      onClose={guardedClose}
      footer={
        <>
          <Button variant="ghost" onClick={guardedClose} disabled={busy}>
            Cancel
          </Button>
          <Button variant="primary" onClick={handleImport} disabled={!title.trim() || busy}>
            {busy ? "Importing…" : "Choose file & import"}
          </Button>
        </>
      }
    >
      <label className={styles.field}>
        <span className={styles.fieldLabel}>Title</span>
        <input
          autoFocus
          className={styles.input}
          value={title}
          onChange={(event) => setTitle(event.target.value)}
          placeholder="e.g. Nebula Drift"
        />
      </label>

      <span className={styles.fieldLabel} id="wallpaper-type-label">
        Type
      </span>
      <div
        className={styles.typeGrid}
        role="radiogroup"
        aria-labelledby="wallpaper-type-label"
        onKeyDown={handleRadioGroupKeyDown}
      >
        {TYPE_OPTIONS.map((option) => (
          <button
            type="button"
            role="radio"
            aria-checked={type === option.value}
            tabIndex={type === option.value ? 0 : -1}
            key={option.value}
            className={[styles.typeOption, type === option.value ? styles.typeOptionActive : ""]
              .filter(Boolean)
              .join(" ")}
            onClick={() => setType(option.value)}
          >
            <span className={styles.typeOptionLabel}>{option.label}</span>
            <span className={styles.typeOptionHint}>{option.hint}</span>
          </button>
        ))}
      </div>
      <p className={styles.note}>
        Choosing "Choose file &amp; import" opens the file picker and copies your content into
        Umbra's own library — the original file isn't moved or modified.
      </p>
      {statusMessage && (
        <p
          role={statusIsError ? "alert" : "status"}
          className={statusIsError ? styles.errorNote : styles.cancelledNote}
        >
          {statusMessage}
        </p>
      )}
    </Dialog>
  );
}
