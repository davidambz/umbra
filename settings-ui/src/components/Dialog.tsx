import { useEffect, useRef, type ReactNode } from "react";
import styles from "./Dialog.module.css";

interface DialogProps {
  title: string;
  onClose: () => void;
  children: ReactNode;
  footer?: ReactNode;
}

export function Dialog({ title, onClose, children, footer }: DialogProps) {
  const panelRef = useRef<HTMLDivElement>(null);

  // Read through a ref rather than depending on onClose directly: callers
  // like AddWallpaperDialog pass an inline-defined onClose that gets a new
  // identity on every one of their own re-renders (e.g. every keystroke
  // into a field), which — with onClose in the dependency array — reran
  // this effect on every keystroke and yanked focus from whatever the user
  // was typing into back to the dialog panel.
  const onCloseRef = useRef(onClose);
  useEffect(() => {
    onCloseRef.current = onClose;
  }, [onClose]);

  useEffect(() => {
    function handleKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") onCloseRef.current();
    }
    document.addEventListener("keydown", handleKeyDown);
    panelRef.current?.focus();
    return () => document.removeEventListener("keydown", handleKeyDown);
  }, []);

  return (
    <div className={styles.overlay} onMouseDown={onClose}>
      <div
        className={styles.panel}
        role="dialog"
        aria-modal="true"
        aria-label={title}
        tabIndex={-1}
        ref={panelRef}
        onMouseDown={(event) => event.stopPropagation()}
      >
        <header className={styles.header}>
          <h2 className={styles.title}>{title}</h2>
          <button
            type="button"
            className={styles.closeButton}
            onClick={onClose}
            aria-label="Close"
          >
            ✕
          </button>
        </header>
        <div className={styles.body}>{children}</div>
        {footer && <footer className={styles.footer}>{footer}</footer>}
      </div>
    </div>
  );
}
