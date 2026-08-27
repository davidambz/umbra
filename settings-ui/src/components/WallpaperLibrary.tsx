import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";
import { findMonitorsReferencingWallpaper } from "../assignmentUtils";
import { monitorDisplayLabel } from "../monitorLabels";
import { WallpaperCard } from "./WallpaperCard";
import { Button } from "./Button";
import styles from "./WallpaperLibrary.module.css";

interface WallpaperLibraryProps {
  library: LibraryItem[];
  monitors: MonitorInfo[];
  assignments: Record<string, MonitorAssignment>;
  onAdd: () => void;
  onRename: (id: string, newTitle: string) => void;
  onRemove: (id: string) => void;
  onQuickAssign: (item: LibraryItem) => void;
}

export function WallpaperLibrary({
  library,
  monitors,
  assignments,
  onAdd,
  onRename,
  onRemove,
  onQuickAssign,
}: WallpaperLibraryProps) {
  return (
    <section>
      <div className={styles.header}>
        <h2 className={styles.heading}>Library</h2>
        <Button variant="primary" onClick={onAdd}>
          + Add wallpaper
        </Button>
      </div>

      {library.length === 0 ? (
        <p className={styles.empty}>
          Nothing imported yet — add a video, image, or web project to assign it to a monitor.
        </p>
      ) : (
        <div className={styles.grid}>
          {library.map((item) => {
            const assignedMonitorIds = findMonitorsReferencingWallpaper(assignments, item.id);
            // Walking `monitors` (already in display order) rather than
            // mapping over assignedMonitorIds directly keeps the warning's
            // labels in the same left-to-right order MonitorGrid renders
            // them in, regardless of the order assignments happened to be
            // recorded in.
            const assignedDisplayLabels = monitors
              .filter((monitor) => assignedMonitorIds.includes(monitor.id))
              .map((monitor) => monitorDisplayLabel(monitors.indexOf(monitor) + 1));
            // A monitor id an assignment still references but that's no
            // longer in `monitors` (unplugged since) has no display label
            // to show — append a generic one rather than silently dropping
            // it from the warning.
            if (assignedMonitorIds.some((id) => !monitors.some((monitor) => monitor.id === id))) {
              assignedDisplayLabels.push("a disconnected display");
            }
            return (
              <WallpaperCard
                key={item.id}
                item={item}
                assignedDisplayLabels={assignedDisplayLabels}
                onRename={(newTitle) => onRename(item.id, newTitle)}
                onRemove={() => onRemove(item.id)}
                onQuickAssign={() => onQuickAssign(item)}
              />
            );
          })}
        </div>
      )}
    </section>
  );
}
