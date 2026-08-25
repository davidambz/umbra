import type { AppSettings } from "../types";
import { Toggle } from "./Toggle";
import styles from "./SettingsPanel.module.css";

interface SettingsPanelProps {
  settings: AppSettings;
  onChange: (patch: Partial<AppSettings>) => void;
}

export function SettingsPanel({ settings, onChange }: SettingsPanelProps) {
  return (
    <section>
      <h2 className={styles.heading}>General</h2>
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
        <div className={styles.divider} />
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
  );
}
