import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";
import { findMonitorsReferencingWallpaper } from "../assignmentUtils";
import { WallpaperCard } from "./WallpaperCard";
import { Button } from "./Button";
import { useI18n } from "../i18n/I18nContext";
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
  const { t } = useI18n();

  return (
    <section>
      <div className={styles.header}>
        <h2 className={styles.heading}>{t.wallpaperLibrary.heading}</h2>
        <Button variant="primary" onClick={onAdd}>
          {t.wallpaperLibrary.addButton}
        </Button>
      </div>

      {library.length === 0 ? (
        <p className={styles.empty}>{t.wallpaperLibrary.empty}</p>
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
              .map((monitor) => t.monitorGrid.displayLabel(monitors.indexOf(monitor) + 1));
            // A monitor id an assignment still references but that's no
            // longer in `monitors` (unplugged since) has no display label
            // to show — append a generic one rather than silently dropping
            // it from the warning.
            if (assignedMonitorIds.some((id) => !monitors.some((monitor) => monitor.id === id))) {
              assignedDisplayLabels.push(t.wallpaperCard.disconnectedDisplay);
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
