import type { AppSettings, LanguageOverride, ThemeOverride } from "../types";
import { Toggle } from "./Toggle";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import { useI18n } from "../i18n/I18nContext";
import { LOCALE_NAMES, SUPPORTED_LOCALES } from "../i18n";
import styles from "./SettingsPanel.module.css";

interface SettingsPanelProps {
  settings: AppSettings;
  onChange: (patch: Partial<AppSettings>) => void;
}

export function SettingsPanel({ settings, onChange }: SettingsPanelProps) {
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
              {SUPPORTED_LOCALES.map((locale) => (
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
        </div>
      </section>
    </div>
  );
}
