import { useState } from "react";
import type { LibraryItem } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import { TYPE_GRADIENT } from "../wallpaperTypeStyles";
import { useI18n } from "../i18n/I18nContext";
import styles from "./WallpaperCard.module.css";

interface WallpaperCardProps {
  item: LibraryItem;
  /** Display labels (e.g. "Display 1") of monitors currently assigned this wallpaper — see WallpaperLibrary's use of findMonitorsReferencingWallpaper. Empty if it isn't assigned anywhere. */
  assignedDisplayLabels: string[];
  onRename: (newTitle: string) => void;
  onRemove: () => void;
  /** Double-clicking the thumbnail opens AssignDialog pre-loaded with this wallpaper. */
  onQuickAssign: () => void;
}

export function WallpaperCard({
  item,
  assignedDisplayLabels,
  onRename,
  onRemove,
  onQuickAssign,
}: WallpaperCardProps) {
  const { t, locale } = useI18n();
  // "A" / "A and B" / "A, B, and C" — Intl.ListFormat handles the Oxford
  // comma/"y"/"和"/etc. conjunction rules per locale that would otherwise
  // creep back in by hand for every new count as this list grows past a
  // couple of monitors.
  const listFormatter = new Intl.ListFormat(locale, { style: "long", type: "conjunction" });
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
        onDoubleClick={onQuickAssign}
        title={t.wallpaperCard.quickAssignHint}
        role="button"
        tabIndex={0}
        aria-label={t.wallpaperCard.quickAssignAriaLabel(item.title)}
        onKeyDown={(event) => {
          if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            onQuickAssign();
          }
        }}
      >
        {item.thumbnailUrl && <img src={item.thumbnailUrl} alt="" className={styles.thumbImg} />}
        <span className={styles.typeBadge}>{t.wallpaperType[item.type]}</span>
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
        aria-label={t.wallpaperCard.deleteAriaLabel(item.title)}
        title={t.wallpaperCard.deleteTitle}
      >
        ✕
      </button>

      {confirmingRemove && (
        <Dialog
          title={t.wallpaperCard.confirmDeleteTitle(item.title)}
          onClose={() => setConfirmingRemove(false)}
          footer={
            <>
              <Button variant="ghost" onClick={() => setConfirmingRemove(false)}>
                {t.common.cancel}
              </Button>
              <Button
                variant="danger"
                onClick={() => {
                  setConfirmingRemove(false);
                  onRemove();
                }}
              >
                {t.common.delete}
              </Button>
            </>
          }
        >
          <p>{t.wallpaperCard.confirmDeleteBody}</p>
          {assignedDisplayLabels.length > 0 && (
            <p className={styles.assignedWarning}>
              {t.wallpaperCard.confirmDeleteAssignedWarning(listFormatter.format(assignedDisplayLabels))}
            </p>
          )}
        </Dialog>
      )}
    </div>
  );
}
