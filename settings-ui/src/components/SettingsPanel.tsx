import type { AppSettings, ThemeOverride } from "../types";
import { Toggle } from "./Toggle";
import { handleRadioGroupKeyDown } from "../radioGroupNav";
import styles from "./SettingsPanel.module.css";

interface SettingsPanelProps {
  settings: AppSettings;
  onChange: (patch: Partial<AppSettings>) => void;
}

const THEME_OPTIONS: Array<{ value: ThemeOverride; label: string }> = [
  { value: "system", label: "System" },
  { value: "light", label: "Light" },
  { value: "dark", label: "Dark" },
];

export function SettingsPanel({ settings, onChange }: SettingsPanelProps) {
  return (
    <div className={styles.categories}>
      <section>
        <h2 className={styles.heading}>Appearance</h2>
        <div className={styles.card}>
          <div className={styles.row}>
            <div className={styles.rowText}>
              <span className={styles.rowLabel}>Theme</span>
              <span className={styles.rowDescription}>
                Follows Windows by default — pin it to always use one theme instead
              </span>
            </div>
            <div
              className={styles.themeTabs}
              role="radiogroup"
              aria-label="Theme"
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
        </div>
      </section>

      <section>
        <h2 className={styles.heading}>Startup &amp; power</h2>
        <div className={styles.card}>
          <Toggle
            label="Launch on startup"
            description="Start Umbra automatically when you sign in to Windows"
            checked={settings.launchOnStartup}
            onChange={(checked) => onChange({ launchOnStartup: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label="Pause when an app is fullscreen"
            description="Free up GPU/CPU while a game or video is fullscreen"
            checked={settings.pauseOnFullscreen}
            onChange={(checked) => onChange({ pauseOnFullscreen: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label="Pause on battery"
            description="Stop rendering wallpapers whenever unplugged"
            checked={settings.pauseOnBattery}
            onChange={(checked) => onChange({ pauseOnBattery: checked })}
          />
        </div>
      </section>

      <section>
        <h2 className={styles.heading}>Display</h2>
        <div className={styles.card}>
          <Toggle
            label="Use as lock screen background"
            description="Show a snapshot of your primary display's wallpaper on the lock screen"
            checked={settings.syncLockScreen}
            onChange={(checked) => onChange({ syncLockScreen: checked })}
          />
          <div className={styles.divider} />
          <Toggle
            label="Sync monitors"
            description="Keep every display showing the same wallpaper — assigning one assigns them all"
            checked={settings.syncMonitors}
            onChange={(checked) => onChange({ syncMonitors: checked })}
          />
        </div>
      </section>
    </div>
  );
}
