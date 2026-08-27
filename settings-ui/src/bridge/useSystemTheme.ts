import { useEffect, useState } from "react";
import type { UiBridge } from "./uiBridge";
import type { Theme, ThemeOverride } from "../types";
import { prefersDarkMediaQuery, systemThemeFromMediaQuery } from "../systemTheme";

/**
 * Follows the Windows system theme (AppsUseLightTheme) via the bridge, per
 * ARCHITECTURE.md's "Theme: follows the Windows system theme, not forced
 * dark" — and keeps `<html data-theme>` in sync so tokens.css picks the
 * right palette. Falls back to the mock bridge's prefers-color-scheme
 * shim until a real bridge answers.
 *
 * `override` (Settings.themeOverride, surfaced by the Appearance section in
 * SettingsPanel) wins over the live system theme whenever it isn't
 * "system" — this hook still keeps tracking the live OS theme underneath
 * so switching the override back to "system" reflects it immediately,
 * with no extra round trip.
 */
export function useSystemTheme(bridge: UiBridge, override: ThemeOverride = "system"): Theme {
  const [systemTheme, setSystemTheme] = useState<Theme>(() =>
    systemThemeFromMediaQuery(prefersDarkMediaQuery()),
  );

  useEffect(() => {
    let unsubscribed = false;

    bridge
      .getTheme()
      .then((initial) => {
        if (!unsubscribed) setSystemTheme(initial);
      })
      .catch((error) => {
        // Leave the prefers-color-scheme fallback in place — a failed
        // getTheme() shouldn't leave the UI stuck with no theme at all.
        console.error("Failed to resolve the system theme from the bridge", error);
      });

    const unsubscribe = bridge.onThemeChange((next) => {
      if (!unsubscribed) setSystemTheme(next);
    });

    return () => {
      unsubscribed = true;
      unsubscribe();
    };
  }, [bridge]);

  const theme = override === "system" ? systemTheme : override;

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

  return theme;
}
