import { useEffect, useRef, useState } from "react";
import type { LibraryItem } from "../types";
import { TYPE_GRADIENT } from "../wallpaperTypeStyles";
import styles from "./WallpaperSelect.module.css";

interface WallpaperSelectProps {
  library: LibraryItem[];
  value: string;
  onChange: (id: string) => void;
  label: string;
}

function Thumb({ item }: { item: LibraryItem | undefined }) {
  return (
    <span
      className={styles.thumb}
      style={{ background: item?.thumbnailUrl ? undefined : TYPE_GRADIENT[item?.type ?? "video"] }}
    >
      {item?.thumbnailUrl && <img src={item.thumbnailUrl} alt="" className={styles.thumbImg} />}
    </span>
  );
}

/**
 * Experimental: a custom listbox standing in for the plain <select> in
 * #101 — a native <select>'s <option>s can't render an image, so this
 * shows each wallpaper's actual thumbnail next to its title, at the cost
 * of reimplementing what a native select gives for free (keyboard nav,
 * outside-click/Escape to close).
 */
export function WallpaperSelect({ library, value, onChange, label }: WallpaperSelectProps) {
  const [open, setOpen] = useState(false);
  const rootRef = useRef<HTMLDivElement>(null);
  const selected = library.find((item) => item.id === value);

  useEffect(() => {
    if (!open) return;
    function handlePointerDown(event: MouseEvent) {
      if (rootRef.current && !rootRef.current.contains(event.target as Node)) {
        setOpen(false);
      }
    }
    function handleKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") setOpen(false);
    }
    document.addEventListener("mousedown", handlePointerDown);
    document.addEventListener("keydown", handleKeyDown);
    return () => {
      document.removeEventListener("mousedown", handlePointerDown);
      document.removeEventListener("keydown", handleKeyDown);
    };
  }, [open]);

  function selectItem(id: string) {
    onChange(id);
    setOpen(false);
  }

  return (
    <div className={styles.root} ref={rootRef}>
      <button
        type="button"
        className={styles.trigger}
        aria-haspopup="listbox"
        aria-expanded={open}
        aria-label={label}
        onClick={() => setOpen((current) => !current)}
      >
        <Thumb item={selected} />
        <span className={styles.triggerTitle}>{selected?.title ?? "Choose a wallpaper"}</span>
        <span className={styles.caret} aria-hidden="true">
          ▾
        </span>
      </button>

      {open && (
        <ul className={styles.listbox} role="listbox" aria-label={label}>
          {library.map((item) => (
            <li key={item.id}>
              <button
                type="button"
                role="option"
                aria-selected={item.id === value}
                className={item.id === value ? styles.optionActive : styles.option}
                onClick={() => selectItem(item.id)}
              >
                <Thumb item={item} />
                <span className={styles.optionTitle}>{item.title}</span>
              </button>
            </li>
          ))}
        </ul>
      )}
    </div>
  );
}
