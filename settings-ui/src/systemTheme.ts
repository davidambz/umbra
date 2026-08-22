import type { Theme } from "./types";

/** The one place both the mock bridge and useSystemTheme's initial fallback ask the browser for its color scheme, so a future change to how that's done can't drift between them. */
export function prefersDarkMediaQuery(): MediaQueryList | undefined {
  return window.matchMedia?.("(prefers-color-scheme: dark)");
}

export function systemThemeFromMediaQuery(media: MediaQueryList | undefined): Theme {
  return media?.matches ? "dark" : "light";
}
