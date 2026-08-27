import { useEffect, useRef, useState } from "react";
import type { LibraryItem } from "../types";
import { TYPE_GRADIENT } from "../wallpaperTypeStyles";
import styles from "./WallpaperSelect.module.css";

interface WallpaperSelectProps {
  library: LibraryItem[];
  value: string;
  onChange: (id: string) => void;
  label: string;
  placeholder: string;
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
export function WallpaperSelect({
  library,
  value,
  onChange,
  label,
  placeholder,
}: WallpaperSelectProps) {
  const [open, setOpen] = useState(false);
  const rootRef = useRef<HTMLDivElement>(null);
  const triggerRef = useRef<HTMLButtonElement>(null);
  const selected = library.find((item) => item.id === value);

  // Closes and, unless the close was itself caused by clicking outside (focus
  // already went wherever the user clicked), returns focus to the trigger —
  // otherwise activating an option unmounts the focused <button role="option">
  // and focus falls to <body>, escaping Dialog's Tab focus trap.
  function close(refocusTrigger: boolean) {
    setOpen(false);
    if (refocusTrigger) triggerRef.current?.focus();
  }

  useEffect(() => {
    if (!open) return;
    function handlePointerDown(event: MouseEvent) {
      if (rootRef.current && !rootRef.current.contains(event.target as Node)) {
        close(false);
      }
    }
    // Registered on the capture phase, and stops propagation, so this fires
    // and consumes Escape before Dialog's own (bubble-phase) document
    // listener gets a chance to treat the same keypress as "close the whole
    // Assign dialog" — otherwise Escape here closed both at once.
    function handleKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") {
        event.stopPropagation();
        close(true);
      }
    }
    document.addEventListener("mousedown", handlePointerDown);
    document.addEventListener("keydown", handleKeyDown, true);
    return () => {
      document.removeEventListener("mousedown", handlePointerDown);
      document.removeEventListener("keydown", handleKeyDown, true);
    };
  }, [open]);

  function selectItem(id: string) {
    onChange(id);
    close(true);
  }

  return (
    <div className={styles.root} ref={rootRef}>
      <button
        type="button"
        ref={triggerRef}
        className={styles.trigger}
        aria-haspopup="listbox"
        aria-expanded={open}
        aria-label={label}
        onClick={() => setOpen((current) => !current)}
      >
        <Thumb item={selected} />
        <span className={styles.triggerTitle}>{selected?.title ?? placeholder}</span>
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
