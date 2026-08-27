import type { LibraryItem, MonitorInfo } from "../types";
import { TYPE_GRADIENT } from "../wallpaperTypeStyles";
import { useI18n } from "../i18n/I18nContext";
import styles from "./MonitorPickerOption.module.css";

interface MonitorPickerOptionProps {
  monitor: MonitorInfo;
  displayIndex: number;
  /** What's currently on this monitor, if anything — see assignmentUtils.resolveAssignmentPreview. */
  previewItem?: LibraryItem;
  selected: boolean;
  onSelect: () => void;
}

/**
 * A tiny physical-monitor bezel, exactly like MonitorCard's, sized down
 * to work as one option inside a radiogroup rather than a full grid
 * tile — used by AssignDialog's quick-assign monitor picker so choosing
 * a target reads the same way MonitorGrid already does, instead of a
 * generic pill button.
 */
export function MonitorPickerOption({
  monitor,
  displayIndex,
  previewItem,
  selected,
  onSelect,
}: MonitorPickerOptionProps) {
  const { t } = useI18n();
  const aspectRatio = `${monitor.width} / ${monitor.height}`;

  return (
    <button
      type="button"
      role="radio"
      aria-checked={selected}
      tabIndex={selected ? 0 : -1}
      className={selected ? styles.optionActive : styles.option}
      onClick={onSelect}
    >
      <div className={styles.bezel}>
        <div
          className={styles.preview}
          style={{
            aspectRatio,
            background: previewItem?.thumbnailUrl ? undefined : TYPE_GRADIENT[previewItem?.type ?? "video"],
          }}
        >
          {previewItem?.thumbnailUrl && (
            <img src={previewItem.thumbnailUrl} alt="" className={styles.thumbnail} />
          )}
          {!previewItem && <span className={styles.emptyGlyph}>—</span>}
        </div>
      </div>
      <div className={styles.stand} />
      <span className={styles.caption}>
        {t.monitorGrid.displayLabel(displayIndex)}
        {monitor.isPrimary && (
          <span className={styles.primaryTag}> {t.assignDialog.primaryTag}</span>
        )}
      </span>
    </button>
  );
}
