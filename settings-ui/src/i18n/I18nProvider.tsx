import { useMemo, type ReactNode } from "react";
import type { Locale } from "../types";
import { I18nContext } from "./I18nContext";
import { LOCALES } from "./index";

interface I18nProviderProps {
  locale: Locale;
  children: ReactNode;
}

export function I18nProvider({ locale, children }: I18nProviderProps) {
  // Memoized so every useI18n() consumer only re-renders when locale
  // actually changes — this wraps the whole App tree, and an unmemoized
  // object literal here would give every consumer (Dialog, MonitorCard,
  // WallpaperCard, SettingsPanel, ...) a "new" context value, and re-render
  // them, on every unrelated App state change.
  const value = useMemo(() => ({ locale, t: LOCALES[locale] }), [locale]);
  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>;
}
