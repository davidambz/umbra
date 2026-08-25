import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";
import { monitorDisplayLabel } from "../monitorLabels";
import styles from "./MonitorCard.module.css";

// Distinct brightness bands (light/mid/dark), not just distinct hues,
// since "Blue eclipse" only has four stops to work with — two gradients
// sharing an endpoint would otherwise read as near-identical at a glance.
const TYPE_GRADIENT: Record<string, string> = {
  image: "linear-gradient(135deg, #8686ac 0%, #505081 100%)",
  video: "linear-gradient(135deg, #505081 0%, #17163f 100%)",
  web: "linear-gradient(135deg, #272757 0%, #0f0e47 100%)",
};

interface MonitorCardProps {
  monitor: MonitorInfo;
  displayIndex: number;
  assignment: MonitorAssignment;
  library: LibraryItem[];
  onEdit: () => void;
}

function assignmentLabel(assignment: MonitorAssignment, library: LibraryItem[]): string {
  if (assignment.kind === "none") return "No wallpaper";
  if (assignment.kind === "single") {
    const item = library.find((entry) => entry.id === assignment.wallpaperId);
    return item?.title ?? "Unknown wallpaper";
  }
  return `Playlist · ${assignment.playlist.wallpaperIds.length} wallpapers`;
}

export function MonitorCard({ monitor, displayIndex, assignment, library, onEdit }: MonitorCardProps) {
  const aspectRatio = `${monitor.width} / ${monitor.height}`;
  const previewItem =
    assignment.kind === "single"
      ? library.find((entry) => entry.id === assignment.wallpaperId)
      : assignment.kind === "playlist"
        ? library.find((entry) => entry.id === assignment.playlist.wallpaperIds[0])
        : undefined;

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
        <div className={styles.bezelChin} />
      </div>
      <div className={styles.stand} />
      <div className={styles.caption}>
        <span className={styles.name}>{monitorDisplayLabel(monitor, displayIndex)}</span>
        <span className={styles.assignment}>{assignmentLabel(assignment, library)}</span>
      </div>
    </button>
  );
}
