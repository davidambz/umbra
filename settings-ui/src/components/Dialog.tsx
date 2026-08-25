import { useEffect, useRef, type ReactNode } from "react";
import { createPortal } from "react-dom";
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

  // Portaled to document.body rather than rendered in place: this overlay
  // is `position: fixed`, which only escapes an ancestor when none of them
  // set transform/filter/perspective/contain — but WallpaperCard's own
  // :hover state does exactly that (a translateY), so a Dialog opened from
  // inside it (e.g. the delete confirmation) rendered squeezed into that
  // card's own box instead of centered over the whole window, and the
  // dialog's presence changing the card's hover/mouse-over state fed back
  // into a visible flicker. A portal makes this correct regardless of what
  // any future caller's own ancestors do.
  return createPortal(
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
    </div>,
    document.body,
  );
}
