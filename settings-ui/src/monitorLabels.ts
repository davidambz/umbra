/**
 * "Display N" (1-based, per monitors' display order) — the same label
 * MonitorCard shows for a monitor, factored out so other callers (the
 * delete-wallpaper confirmation's "assigned to ..." warning) can't drift
 * from it. Deliberately uniform (no special-cased "Primary display") so
 * every monitor reads the same way regardless of which one Windows
 * considers primary.
 */
export function monitorDisplayLabel(displayIndex: number): string {
  return `Display ${displayIndex}`;
}
