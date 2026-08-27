import type { LibraryItem, Playlist, PlaylistMode } from "../types";
import { useI18n } from "../i18n/I18nContext";
import type { Strings } from "../i18n";
import styles from "./PlaylistEditor.module.css";

interface PlaylistEditorProps {
  playlist: Playlist;
  library: LibraryItem[];
  onChange: (playlist: Playlist) => void;
}

function titleFor(library: LibraryItem[], id: string, t: Strings): string {
  return library.find((item) => item.id === id)?.title ?? t.playlistEditor.unknownWallpaper;
}

export function PlaylistEditor({ playlist, library, onChange }: PlaylistEditorProps) {
  const { t } = useI18n();

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
        <span className={styles.sectionLabel}>{t.playlistEditor.rotationOrder}</span>
        {playlist.wallpaperIds.length === 0 ? (
          <p className={styles.hint}>{t.playlistEditor.rotationEmptyHint}</p>
        ) : (
          <ol className={styles.order}>
            {playlist.wallpaperIds.map((id, index) => (
              <li key={`${id}-${index}`} className={styles.orderItem}>
                <span className={styles.orderTitle}>{titleFor(library, id, t)}</span>
                <span className={styles.orderControls}>
                  <button
                    type="button"
                    onClick={() => move(index, -1)}
                    disabled={index === 0}
                    aria-label={t.playlistEditor.moveEarlier}
                  >
                    ↑
                  </button>
                  <button
                    type="button"
                    onClick={() => move(index, 1)}
                    disabled={index === playlist.wallpaperIds.length - 1}
                    aria-label={t.playlistEditor.moveLater}
                  >
                    ↓
                  </button>
                  <button
                    type="button"
                    onClick={() => toggleWallpaper(id)}
                    aria-label={t.playlistEditor.removeFromRotation(titleFor(library, id, t))}
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
        <span className={styles.sectionLabel}>{t.playlistEditor.availableWallpapers}</span>
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
          <span className={styles.sectionLabel}>{t.playlistEditor.changeEvery}</span>
          <div className={styles.intervalInput}>
            <input
              type="number"
              min={1}
              // Shows the true stored value even if it's below the
              // 1-minute floor below (e.g. legacy/native data saved before
              // that floor existed) — the floor only applies going
              // forward, to an actual edit, not to what's already there.
              // The floor itself only exists to keep the rotation timer
              // meaningful (Playlist::isValid() just requires > 0) — the
              // user picks whatever interval they want above that.
              value={Math.round(playlist.intervalSeconds / 60)}
              onChange={(event) =>
                onChange({
                  ...playlist,
                  intervalSeconds: Math.max(1, Number(event.target.value)) * 60,
                })
              }
            />
            <span>{t.playlistEditor.minutes}</span>
          </div>
        </label>

        <label className={styles.field}>
          <span className={styles.sectionLabel}>{t.playlistEditor.order}</span>
          <select
            value={playlist.mode}
            onChange={(event) => onChange({ ...playlist, mode: event.target.value as PlaylistMode })}
          >
            <option value="sequential">{t.playlistEditor.sequential}</option>
            <option value="shuffle">{t.playlistEditor.shuffle}</option>
          </select>
        </label>
      </div>
    </div>
  );
}
