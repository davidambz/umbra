import { useState } from "react";
import type { WallpaperType } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import styles from "./AddWallpaperDialog.module.css";

interface AddWallpaperDialogProps {
  onClose: () => void;
  onImport: (title: string, type: WallpaperType) => Promise<void>;
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

  async function handleImport() {
    const trimmed = title.trim();
    if (!trimmed || busy) return;
    setBusy(true);
    await onImport(trimmed, type);
    setBusy(false);
    onClose();
  }

  return (
    <Dialog
      title="Add wallpaper"
      onClose={onClose}
      footer={
        <>
          <Button variant="ghost" onClick={onClose}>
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

      <span className={styles.fieldLabel}>Type</span>
      <div className={styles.typeGrid}>
        {TYPE_OPTIONS.map((option) => (
          <button
            type="button"
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
    </Dialog>
  );
}
