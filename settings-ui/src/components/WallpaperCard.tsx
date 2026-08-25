import { useState } from "react";
import type { LibraryItem } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import styles from "./WallpaperCard.module.css";

// Distinct brightness bands (light/mid/dark), not just distinct hues,
// since "Blue eclipse" only has four stops to work with — two gradients
// sharing an endpoint would otherwise read as near-identical at a glance.
const TYPE_GRADIENT: Record<string, string> = {
  image: "linear-gradient(135deg, #8686ac 0%, #505081 100%)",
  video: "linear-gradient(135deg, #505081 0%, #17163f 100%)",
  web: "linear-gradient(135deg, #272757 0%, #0f0e47 100%)",
};

const TYPE_LABEL: Record<string, string> = { video: "Video", image: "Image", web: "Web" };

interface WallpaperCardProps {
  item: LibraryItem;
  /** Display labels (e.g. "Primary display") of monitors currently assigned this wallpaper — see WallpaperLibrary's use of findMonitorsReferencingWallpaper. Empty if it isn't assigned anywhere. */
  assignedDisplayLabels: string[];
  onRename: (newTitle: string) => void;
  onRemove: () => void;
}

// "A" / "A and B" / "A, B, and C" — Intl.ListFormat handles the Oxford
// comma and pluralization rules that would otherwise creep back in by
// hand for every new count as this list grows past a couple of monitors.
const listFormatter = new Intl.ListFormat("en", { style: "long", type: "conjunction" });

export function WallpaperCard({
  item,
  assignedDisplayLabels,
  onRename,
  onRemove,
}: WallpaperCardProps) {
  const [editing, setEditing] = useState(false);
  const [draftTitle, setDraftTitle] = useState(item.title);
  const [confirmingRemove, setConfirmingRemove] = useState(false);

  function startEditing() {
    // Re-seed from the current prop rather than trusting whatever
    // draftTitle was left holding from a previous edit session (a
    // rejected rename, or one that was never actually committed).
    setDraftTitle(item.title);
    setEditing(true);
  }

  function commitRename() {
    const trimmed = draftTitle.trim();
    setEditing(false);
    if (trimmed && trimmed !== item.title) {
      setDraftTitle(trimmed);
      onRename(trimmed);
    } else {
      setDraftTitle(item.title);
    }
  }

  return (
    <div className={styles.card}>
      <div
        className={styles.thumb}
        style={{ background: item.thumbnailUrl ? undefined : TYPE_GRADIENT[item.type] }}
      >
        {item.thumbnailUrl && <img src={item.thumbnailUrl} alt="" className={styles.thumbImg} />}
        <span className={styles.typeBadge}>{TYPE_LABEL[item.type]}</span>
      </div>

      {editing ? (
        <input
          autoFocus
          className={styles.titleInput}
          value={draftTitle}
          onChange={(event) => setDraftTitle(event.target.value)}
          onBlur={commitRename}
          onKeyDown={(event) => {
            if (event.key === "Enter") commitRename();
            if (event.key === "Escape") {
              setDraftTitle(item.title);
              setEditing(false);
            }
          }}
        />
      ) : (
        <button type="button" className={styles.title} onClick={startEditing}>
          {item.title}
        </button>
      )}

      <button
        type="button"
        className={styles.removeButton}
        onClick={() => setConfirmingRemove(true)}
        aria-label={`Delete ${item.title}`}
        title="Delete"
      >
        ✕
      </button>

      {confirmingRemove && (
        <Dialog
          title={`Delete "${item.title}"?`}
          onClose={() => setConfirmingRemove(false)}
          footer={
            <>
              <Button variant="ghost" onClick={() => setConfirmingRemove(false)}>
                Cancel
              </Button>
              <Button
                variant="danger"
                onClick={() => {
                  setConfirmingRemove(false);
                  onRemove();
                }}
              >
                Delete
              </Button>
            </>
          }
        >
          <p>This permanently removes the imported file. This can't be undone.</p>
          {assignedDisplayLabels.length > 0 && (
            <p className={styles.assignedWarning}>
              Currently assigned to {listFormatter.format(assignedDisplayLabels)} — deleting it
              will clear that assignment.
            </p>
          )}
        </Dialog>
      )}
    </div>
  );
}
