import { useState } from "react";
import type { LibraryItem } from "../types";
import styles from "./WallpaperCard.module.css";

const TYPE_GRADIENT: Record<string, string> = {
  video: "linear-gradient(135deg, #6c4fe0 0%, #2a2360 100%)",
  image: "linear-gradient(135deg, #e07a5f 0%, #6c4fe0 100%)",
  web: "linear-gradient(135deg, #3fa9c9 0%, #6c4fe0 100%)",
};

const TYPE_LABEL: Record<string, string> = { video: "Video", image: "Image", web: "Web" };

interface WallpaperCardProps {
  item: LibraryItem;
  onRename: (newTitle: string) => void;
  onRemove: () => void;
}

export function WallpaperCard({ item, onRename, onRemove }: WallpaperCardProps) {
  const [editing, setEditing] = useState(false);
  const [draftTitle, setDraftTitle] = useState(item.title);

  function commitRename() {
    const trimmed = draftTitle.trim();
    setEditing(false);
    if (trimmed && trimmed !== item.title) {
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
        <button type="button" className={styles.title} onClick={() => setEditing(true)}>
          {item.title}
        </button>
      )}

      <button
        type="button"
        className={styles.removeButton}
        onClick={onRemove}
        aria-label={`Delete ${item.title}`}
        title="Delete"
      >
        ✕
      </button>
    </div>
  );
}
