import { Button } from "./Button";
import { useI18n } from "../i18n/I18nContext";
import type { UpdateCheckResult } from "../types";
import styles from "./SettingsPanel.module.css";
import ownStyles from "./UpdatesPanel.module.css";

interface UpdatesPanelProps {
  /** null until App.tsx's initial load resolves it. */
  appVersion: string | null;
  /** null before the first check this session ever completes. */
  updateCheck: UpdateCheckResult | null;
  checking: boolean;
  applying: boolean;
  onCheckForUpdate: () => void;
  onApplyUpdate: () => void;
}

export function UpdatesPanel({
  appVersion,
  updateCheck,
  checking,
  applying,
  onCheckForUpdate,
  onApplyUpdate,
}: UpdatesPanelProps) {
  const { t } = useI18n();

  return (
    <section>
      <h2 className={styles.heading}>{t.updates.heading}</h2>
      <div className={styles.card}>
        <div className={styles.row}>
          <div className={styles.rowText}>
            <span className={styles.rowLabel}>
              {appVersion ? t.updates.currentVersion(appVersion) : ""}
            </span>
          </div>
          <Button variant="secondary" onClick={onCheckForUpdate} disabled={checking || applying}>
            {checking ? t.updates.checking : t.updates.checkButton}
          </Button>
        </div>

        {updateCheck && (
          <>
            <div className={styles.divider} />
            <div className={styles.row}>
              <div className={styles.rowText}>
                {!updateCheck.checkSucceeded && (
                  <span className={ownStyles.error}>{t.updates.checkFailed}</span>
                )}
                {updateCheck.checkSucceeded && !updateCheck.updateAvailable && (
                  <span className={styles.rowDescription}>{t.updates.upToDate}</span>
                )}
                {updateCheck.checkSucceeded && updateCheck.updateAvailable && (
                  <span className={styles.rowLabel}>
                    {applying
                      ? t.updates.installing
                      : t.updates.updateAvailable(updateCheck.latestVersion)}
                  </span>
                )}
              </div>
              {updateCheck.checkSucceeded && updateCheck.updateAvailable && (
                <Button variant="primary" onClick={onApplyUpdate} disabled={applying}>
                  {t.updates.updateButton}
                </Button>
              )}
            </div>
          </>
        )}
      </div>
    </section>
  );
}
