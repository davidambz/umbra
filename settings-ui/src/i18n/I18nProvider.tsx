import type { ReactNode } from "react";
import type { Locale } from "../types";
import { I18nContext } from "./I18nContext";
import { LOCALES } from "./index";

interface I18nProviderProps {
  locale: Locale;
  children: ReactNode;
}

export function I18nProvider({ locale, children }: I18nProviderProps) {
  return <I18nContext.Provider value={{ locale, t: LOCALES[locale] }}>{children}</I18nContext.Provider>;
}
