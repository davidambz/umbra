import type { MonitorInfo } from "./types";

/**
 * "Primary display" or "Display N" (1-based, per monitors' display
 * order) — the same label MonitorCard shows for a monitor, factored out
 * so other callers (the delete-wallpaper confirmation's "assigned to
 * ..." warning) can't drift from it.
 */
export function monitorDisplayLabel(monitor: MonitorInfo, displayIndex: number): string {
  return monitor.isPrimary ? "Primary display" : `Display ${displayIndex}`;
}
