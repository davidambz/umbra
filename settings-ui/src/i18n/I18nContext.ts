import { createContext, useContext } from "react";
import type { Locale } from "../types";
import type { Strings } from "./strings";
import { LOCALES } from "./index";

interface I18nContextValue {
  locale: Locale;
  t: Strings;
}

// Defaults to English so any component rendered outside I18nProvider in a
// test (most existing component tests don't wrap in one) still gets real
// strings instead of throwing or reading undefined.
export const I18nContext = createContext<I18nContextValue>({ locale: "en", t: LOCALES.en });

export function useI18n(): I18nContextValue {
  return useContext(I18nContext);
}
