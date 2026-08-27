import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";
import { MonitorCard } from "./MonitorCard";
import { useI18n } from "../i18n/I18nContext";
import styles from "./MonitorGrid.module.css";

interface MonitorGridProps {
  monitors: MonitorInfo[];
  assignments: Record<string, MonitorAssignment>;
  library: LibraryItem[];
  onEditMonitor: (monitor: MonitorInfo) => void;
}

export function MonitorGrid({ monitors, assignments, library, onEditMonitor }: MonitorGridProps) {
  const { t } = useI18n();

  if (monitors.length === 0) {
    return <p className={styles.empty}>{t.monitorGrid.empty}</p>;
  }

  return (
    <div className={styles.grid}>
      {monitors.map((monitor, index) => (
        <MonitorCard
          key={monitor.id}
          monitor={monitor}
          displayIndex={index + 1}
          assignment={assignments[monitor.id] ?? { kind: "none" }}
          library={library}
          onEdit={() => onEditMonitor(monitor)}
        />
      ))}
    </div>
  );
}
