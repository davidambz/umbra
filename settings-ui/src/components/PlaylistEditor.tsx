import type { LibraryItem, Playlist, PlaylistMode } from "../types";
import styles from "./PlaylistEditor.module.css";

interface PlaylistEditorProps {
  playlist: Playlist;
  library: LibraryItem[];
  onChange: (playlist: Playlist) => void;
}

function titleFor(library: LibraryItem[], id: string): string {
  return library.find((item) => item.id === id)?.title ?? "Unknown wallpaper";
}

export function PlaylistEditor({ playlist, library, onChange }: PlaylistEditorProps) {
  function toggleWallpaper(id: string) {
    const included = playlist.wallpaperIds.includes(id);
    const wallpaperIds = included
      ? playlist.wallpaperIds.filter((entry) => entry !== id)
      : [...playlist.wallpaperIds, id];
    onChange({ ...playlist, wallpaperIds });
  }

  function move(index: number, direction: -1 | 1) {
    const target = index + direction;
    if (target < 0 || target >= playlist.wallpaperIds.length) return;
    const wallpaperIds = [...playlist.wallpaperIds];
    [wallpaperIds[index], wallpaperIds[target]] = [wallpaperIds[target], wallpaperIds[index]];
    onChange({ ...playlist, wallpaperIds });
  }

  return (
    <div className={styles.wrapper}>
      <div className={styles.section}>
        <span className={styles.sectionLabel}>Rotation order</span>
        {playlist.wallpaperIds.length === 0 ? (
          <p className={styles.hint}>Pick at least one wallpaper below to build the rotation.</p>
        ) : (
          <ol className={styles.order}>
            {playlist.wallpaperIds.map((id, index) => (
              <li key={`${id}-${index}`} className={styles.orderItem}>
                <span className={styles.orderTitle}>{titleFor(library, id)}</span>
                <span className={styles.orderControls}>
                  <button
                    type="button"
                    onClick={() => move(index, -1)}
                    disabled={index === 0}
                    aria-label="Move earlier"
                  >
                    ↑
                  </button>
                  <button
                    type="button"
                    onClick={() => move(index, 1)}
                    disabled={index === playlist.wallpaperIds.length - 1}
                    aria-label="Move later"
                  >
                    ↓
                  </button>
                  <button
                    type="button"
                    onClick={() => toggleWallpaper(id)}
                    aria-label={`Remove ${titleFor(library, id)} from rotation`}
                  >
                    ✕
                  </button>
                </span>
              </li>
            ))}
          </ol>
        )}
      </div>

      <div className={styles.section}>
        <span className={styles.sectionLabel}>Available wallpapers</span>
        <div className={styles.checkList}>
          {library.map((item) => (
            <label key={item.id} className={styles.checkRow}>
              <input
                type="checkbox"
                checked={playlist.wallpaperIds.includes(item.id)}
                onChange={() => toggleWallpaper(item.id)}
              />
              {item.title}
            </label>
          ))}
        </div>
      </div>

      <div className={styles.row}>
        <label className={styles.field}>
          <span className={styles.sectionLabel}>Change every</span>
          <div className={styles.intervalInput}>
            <input
              type="number"
              min={5}
              // Shows the true stored value even if it's below the 5-minute
              // floor below (e.g. legacy/native data saved before that
              // floor existed) — the floor only applies going forward, to
              // an actual edit, not to what's already there.
              value={Math.round(playlist.intervalSeconds / 60)}
              onChange={(event) =>
                onChange({
                  ...playlist,
                  intervalSeconds: Math.max(5, Number(event.target.value)) * 60,
                })
              }
            />
            <span>minutes</span>
          </div>
        </label>

        <label className={styles.field}>
          <span className={styles.sectionLabel}>Order</span>
          <select
            value={playlist.mode}
            onChange={(event) => onChange({ ...playlist, mode: event.target.value as PlaylistMode })}
          >
            <option value="sequential">Sequential</option>
            <option value="shuffle">Shuffle</option>
          </select>
        </label>
      </div>
    </div>
  );
}
