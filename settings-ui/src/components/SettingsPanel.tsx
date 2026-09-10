import type { AppSettings, LanguageOverride, ThemeOverride, UpdateCheckResult } from "../types";
import { Toggle } from "./Toggle";
import { UpdatesPanel } from "./UpdatesPanel";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import { useI18n } from "../i18n/I18nContext";
import { LOCALE_NAMES, SORTED_LOCALES } from "../i18n";
import styles from "./SettingsPanel.module.css";

interface SettingsPanelProps {
  settings: AppSettings;
  onChange: (patch: Partial<AppSettings>) => void;
  appVersion: string | null;
  updateCheck: UpdateCheckResult | null;
  checkingForUpdate: boolean;
  applyingUpdate: boolean;
  onCheckForUpdate: () => void;
  onApplyUpdate: () => void;
}

export function SettingsPanel({
  settings,
  onChange,
  appVersion,
  updateCheck,
  checkingForUpdate,
  applyingUpdate,
  onCheckForUpdate,
  onApplyUpdate,
}: SettingsPanelProps) {
  const { t } = useI18n();

  const THEME_OPTIONS: Array<{ value: ThemeOverride; label: string }> = [
    { value: "system", label: t.settingsPanel.themeSystem },
    { value: "light", label: t.settingsPanel.themeLight },
    { value: "dark", label: t.settingsPanel.themeDark },
  ];

  return (
    <div className={styles.categories}>
      <section>
        <h2 className={styles.heading}>{t.settingsPanel.appearanceHeading}</h2>
        <div className={styles.card}>
          <div className={styles.row}>
            <div className={styles.rowText}>
              <span className={styles.rowLabel}>{t.settingsPanel.themeLabel}</span>
              <span className={styles.rowDescription}>{t.settingsPanel.themeDescription}</span>
            </div>
            <div
              className={styles.themeTabs}
              role="radiogroup"
              aria-label={t.settingsPanel.themeLabel}
              onKeyDown={handleRadioGroupKeyDown}
            >
              {THEME_OPTIONS.map((option) => (
                <button
                  key={option.value}
                  type="button"
                  role="radio"
                  aria-checked={settings.themeOverride === option.value}
                  tabIndex={settings.themeOverride === option.value ? 0 : -1}
                  className={
                    settings.themeOverride === option.value ? styles.themeTabActive : styles.themeTab
                  }
                  onClick={() => onChange({ themeOverride: option.value })}
                >
                  {option.label}
                </button>
              ))}
            </div>
          </div>

          <div className={styles.divider} />

          <div className={styles.row}>
            <div className={styles.rowText}>
              <span className={styles.rowLabel}>{t.settingsPanel.languageLabel}</span>
              <span className={styles.rowDescription}>{t.settingsPanel.languageDescription}</span>
            </div>
            <select
              className={styles.languageSelect}
              aria-label={t.settingsPanel.languageLabel}
              value={settings.languageOverride}
              onChange={(event) =>
                onChange({ languageOverride: event.target.value as LanguageOverride })
              }
            >
              <option value="system">{t.settingsPanel.languageSystem}</option>
              {SORTED_LOCALES.map((locale) => (
                <option key={locale} value={locale}>
                  {LOCALE_NAMES[locale]}
                </option>
              ))}
            </select>
          </div>
        </div>
      </section>

      <section>
        <h2 className={styles.heading}>{t.settingsPanel.displayHeading}</h2>
        <div className={styles.card}>
          <Toggle
            label={t.settingsPanel.syncLockScreenLabel}
            description={t.settingsPanel.syncLockScreenDescription}
            checked={settings.syncLockScreen}
            onChange={(checked) => onChange({ syncLockScreen: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label={t.settingsPanel.syncMonitorsLabel}
            description={t.settingsPanel.syncMonitorsDescription}
            checked={settings.syncMonitors}
            onChange={(checked) => onChange({ syncMonitors: checked })}
          />
        </div>
      </section>

      <section>
        <h2 className={styles.heading}>{t.settingsPanel.startupHeading}</h2>
        <div className={styles.card}>
          <Toggle
            label={t.settingsPanel.launchOnStartupLabel}
            description={t.settingsPanel.launchOnStartupDescription}
            checked={settings.launchOnStartup}
            onChange={(checked) => onChange({ launchOnStartup: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label={t.settingsPanel.pauseOnFullscreenLabel}
            description={t.settingsPanel.pauseOnFullscreenDescription}
            checked={settings.pauseOnFullscreen}
            onChange={(checked) => onChange({ pauseOnFullscreen: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label={t.settingsPanel.pauseOnBatteryLabel}
            description={t.settingsPanel.pauseOnBatteryDescription}
            checked={settings.pauseOnBattery}
            onChange={(checked) => onChange({ pauseOnBattery: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label={t.settingsPanel.pauseOnBatterySaverLabel}
            description={t.settingsPanel.pauseOnBatterySaverDescription}
            checked={settings.pauseOnBatterySaver}
            onChange={(checked) => onChange({ pauseOnBatterySaver: checked })}
          />
          <div className={styles.divider} />
          <div className={styles.row}>
            <div className={styles.rowText}>
              <span className={styles.rowLabel}>{t.settingsPanel.reducedFpsCapLabel}</span>
              <span className={styles.rowDescription}>
                {t.settingsPanel.reducedFpsCapDescription}
              </span>
            </div>
            <input
              type="number"
              min={1}
              className={styles.numberInput}
              aria-label={t.settingsPanel.reducedFpsCapLabel}
              value={settings.reducedFpsCap}
              onChange={(event) =>
                onChange({ reducedFpsCap: Math.max(1, Number(event.target.value)) })
              }
            />
          </div>
          <div className={styles.divider} />
          <Toggle
            label={t.settingsPanel.pauseBelowBatteryPercentLabel}
            description={t.settingsPanel.pauseBelowBatteryPercentDescription}
            checked={settings.pauseBelowBatteryPercent >= 0}
            onChange={(checked) => onChange({ pauseBelowBatteryPercent: checked ? 20 : -1 })}
          />
          {settings.pauseBelowBatteryPercent >= 0 && (
            <div className={styles.row}>
              <input
                type="number"
                min={0}
                max={100}
                className={styles.numberInput}
                aria-label={t.settingsPanel.pauseBelowBatteryPercentLabel}
                value={settings.pauseBelowBatteryPercent}
                onChange={(event) =>
                  onChange({
                    pauseBelowBatteryPercent: Math.min(
                      100,
                      Math.max(0, Number(event.target.value))
                    ),
                  })
                }
              />
              <span>%</span>
            </div>
          )}
        </div>
      </section>

      <UpdatesPanel
        appVersion={appVersion}
        updateCheck={updateCheck}
        checking={checkingForUpdate}
        applying={applyingUpdate}
        onCheckForUpdate={onCheckForUpdate}
        onApplyUpdate={onApplyUpdate}
      />
    </div>
  );
}
