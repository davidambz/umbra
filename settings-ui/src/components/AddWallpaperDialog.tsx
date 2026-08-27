import { useState } from "react";
import type { WallpaperType } from "../types";
import { Dialog } from "./Dialog";
import { Button } from "./Button";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import { useI18n } from "../i18n/I18nContext";
import styles from "./AddWallpaperDialog.module.css";

interface AddWallpaperDialogProps {
  onClose: () => void;
  /** Resolves to whether the import actually happened — the dialog stays open if not (e.g. the user cancelled the file picker). Throws (e.g. a duplicate title) instead of resolving falsy for an actual failure. */
  onImport: (title: string, type: WallpaperType) => Promise<boolean>;
}

export function AddWallpaperDialog({ onClose, onImport }: AddWallpaperDialogProps) {
  const { t } = useI18n();
  const TYPE_OPTIONS: { value: WallpaperType; label: string; hint: string }[] = [
    { value: "video", label: t.wallpaperType.video, hint: t.addWallpaperDialog.typeHintVideo },
    { value: "image", label: t.wallpaperType.image, hint: t.addWallpaperDialog.typeHintImage },
    { value: "web", label: t.wallpaperType.web, hint: t.addWallpaperDialog.typeHintWeb },
  ];
  const [title, setTitle] = useState("");
  const [type, setType] = useState<WallpaperType>("video");
  const [busy, setBusy] = useState(false);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);
  // Cancelling the file picker isn't an error — only a thrown import
  // failure should read as one (and get role="alert" for immediate
  // announcement instead of the calmer role="status").
  const [statusIsError, setStatusIsError] = useState(false);

  // Ignored while an import is in flight — otherwise closing mid-import
  // doesn't stop it from completing and adding the wallpaper anyway once
  // it resolves after the dialog is already gone.
  function guardedClose() {
    if (!busy) onClose();
  }

  async function handleImport() {
    const trimmed = title.trim();
    if (!trimmed || busy) return;
    setBusy(true);
    setStatusMessage(null);
    setStatusIsError(false);
    try {
      const imported = await onImport(trimmed, type);
      if (imported) {
        onClose();
        return;
      }
      setStatusMessage(t.addWallpaperDialog.cancelledMessage);
    } catch (error) {
      setStatusIsError(true);
      setStatusMessage(error instanceof Error ? error.message : t.addWallpaperDialog.genericFailedMessage);
    } finally {
      setBusy(false);
    }
  }

  return (
    <Dialog
      title={t.addWallpaperDialog.title}
      onClose={guardedClose}
      footer={
        <>
          <Button variant="ghost" onClick={guardedClose} disabled={busy}>
            {t.common.cancel}
          </Button>
          <Button variant="primary" onClick={handleImport} disabled={!title.trim() || busy}>
            {busy ? t.addWallpaperDialog.importing : t.addWallpaperDialog.chooseFileAndImport}
          </Button>
        </>
      }
    >
      <label className={styles.field}>
        <span className={styles.fieldLabel}>{t.addWallpaperDialog.titleFieldLabel}</span>
        <input
          autoFocus
          className={styles.input}
          value={title}
          onChange={(event) => setTitle(event.target.value)}
          placeholder={t.addWallpaperDialog.titlePlaceholder}
        />
      </label>

      <span className={styles.fieldLabel} id="wallpaper-type-label">
        {t.addWallpaperDialog.typeFieldLabel}
      </span>
      <div
        className={styles.typeGrid}
        role="radiogroup"
        aria-labelledby="wallpaper-type-label"
        onKeyDown={handleRadioGroupKeyDown}
      >
        {TYPE_OPTIONS.map((option) => (
          <button
            type="button"
            role="radio"
            aria-checked={type === option.value}
            tabIndex={type === option.value ? 0 : -1}
            key={option.value}
            className={[styles.typeOption, type === option.value ? styles.typeOptionActive : ""]
              .filter(Boolean)
              .join(" ")}
            onClick={() => setType(option.value)}
          >
            <span className={styles.typeOptionLabel}>{option.label}</span>
            <span className={styles.typeOptionHint}>{option.hint}</span>
          </button>
        ))}
      </div>
      <p className={styles.note}>{t.addWallpaperDialog.note}</p>
      {statusMessage && (
        <p
          role={statusIsError ? "alert" : "status"}
          className={statusIsError ? styles.errorNote : styles.cancelledNote}
        >
          {statusMessage}
        </p>
      )}
    </Dialog>
  );
}
