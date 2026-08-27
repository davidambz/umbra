import { useEffect, useState } from "react";
import type { UiBridge } from "./uiBridge";
import type { LanguageOverride, Locale } from "../types";
import { resolveSupportedLocale } from "../i18n";

/**
 * Follows the OS UI language via the bridge, mirroring useSystemTheme.ts's
 * own shape for themeOverride — see that file's doc comment for the
 * override-vs-live-value reasoning, which applies here identically.
 * Unlike theme, the OS UI language can't change live during a running
 * session in any way that matters here, so there's no onLanguageChange
 * subscription to keep open — just one getLanguage() call at mount.
 */
export function useSystemLanguage(bridge: UiBridge, override: LanguageOverride = "system"): Locale {
  const [osLanguage, setOsLanguage] = useState<Locale>(() =>
    resolveSupportedLocale(navigator.language),
  );

  useEffect(() => {
    let unsubscribed = false;
    bridge
      .getLanguage()
      .then((raw) => {
        if (!unsubscribed) setOsLanguage(resolveSupportedLocale(raw));
      })
      .catch((error) => {
        // Leave the navigator.language-derived fallback in place — a
        // failed getLanguage() shouldn't leave the UI stuck with no
        // language resolved at all.
        console.error("Failed to resolve the system language from the bridge", error);
      });
    return () => {
      unsubscribed = true;
    };
  }, [bridge]);

  return override === "system" ? osLanguage : override;
}
