import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";
import { resolveAssignmentPreview } from "../assignmentUtils";
import { TYPE_GRADIENT } from "../wallpaperTypeStyles";
import { useI18n } from "../i18n/I18nContext";
import type { Strings } from "../i18n";
import styles from "./MonitorCard.module.css";

interface MonitorCardProps {
  monitor: MonitorInfo;
  displayIndex: number;
  assignment: MonitorAssignment;
  library: LibraryItem[];
  onEdit: () => void;
}

function assignmentLabel(assignment: MonitorAssignment, library: LibraryItem[], t: Strings): string {
  if (assignment.kind === "none") return t.monitorCard.noWallpaper;
  if (assignment.kind === "single") {
    const item = library.find((entry) => entry.id === assignment.wallpaperId);
    return item?.title ?? t.monitorCard.unknownWallpaper;
  }
  return t.monitorCard.playlistSummary(assignment.playlist.wallpaperIds.length);
}

export function MonitorCard({ monitor, displayIndex, assignment, library, onEdit }: MonitorCardProps) {
  const { t } = useI18n();
  const aspectRatio = `${monitor.width} / ${monitor.height}`;
  const previewItem = resolveAssignmentPreview(assignment, library);

  return (
    <button type="button" className={styles.card} onClick={onEdit}>
      <div className={styles.bezel}>
        <div
          className={styles.preview}
          style={{
            aspectRatio,
            background: previewItem?.thumbnailUrl
              ? undefined
              : TYPE_GRADIENT[previewItem?.type ?? "video"],
          }}
        >
          {previewItem?.thumbnailUrl && (
            <img src={previewItem.thumbnailUrl} alt="" className={styles.thumbnail} />
          )}
          {!previewItem && <span className={styles.emptyGlyph}>—</span>}
        </div>
        <div className={styles.bezelChin}>
          <span
            className={assignment.kind !== "none" ? styles.lampLit : styles.lamp}
            aria-hidden="true"
          />
        </div>
      </div>
      <div className={styles.stand} />
      <div className={styles.caption}>
        <span className={styles.name}>{t.monitorGrid.displayLabel(displayIndex)}</span>
        <span className={styles.assignment}>{assignmentLabel(assignment, library, t)}</span>
      </div>
    </button>
  );
}
