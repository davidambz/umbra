import { useEffect, useState } from "react";
import type { UiBridge } from "./uiBridge";
import type { Theme } from "../types";

/**
 * Follows the Windows system theme (AppsUseLightTheme) via the bridge, per
 * ARCHITECTURE.md's "Theme: follows the Windows system theme, not forced
 * dark" — and keeps `<html data-theme>` in sync so tokens.css picks the
 * right palette. Falls back to the mock bridge's prefers-color-scheme
 * shim until a real bridge answers.
 */
export function useSystemTheme(bridge: UiBridge): Theme {
  const [theme, setTheme] = useState<Theme>(() =>
    window.matchMedia?.("(prefers-color-scheme: dark)").matches ? "dark" : "light",
  );

  useEffect(() => {
    let unsubscribed = false;

    bridge.getTheme().then((initial) => {
      if (!unsubscribed) setTheme(initial);
    });

    const unsubscribe = bridge.onThemeChange((next) => {
      if (!unsubscribed) setTheme(next);
    });

    return () => {
      unsubscribed = true;
      unsubscribe();
    };
  }, [bridge]);

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme);
  }, [theme]);

  return theme;
}
