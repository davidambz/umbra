import type { Locale } from "../types";
import type { Strings } from "./strings";
import { en } from "./locales/en";
import { ptBR } from "./locales/pt-BR";
import { es } from "./locales/es";
import { zhCN } from "./locales/zh-CN";
import { fr } from "./locales/fr";
import { ru } from "./locales/ru";
import { ja } from "./locales/ja";
import { ko } from "./locales/ko";

export type { Strings } from "./strings";

export const LOCALES: Record<Locale, Strings> = {
  en,
  "pt-BR": ptBR,
  es,
  "zh-CN": zhCN,
  fr,
  ru,
  ja,
  ko,
};

/**
 * Each language's own name, in its own script — shown in the Settings
 * language picker regardless of which language the UI currently happens
 * to be in, so someone can always find their language even if the UI is
 * showing the wrong one right now.
 */
export const LOCALE_NAMES: Record<Locale, string> = {
  en: "English",
  "pt-BR": "Português (Brasil)",
  es: "Español",
  "zh-CN": "简体中文",
  fr: "Français",
  ru: "Русский",
  ja: "日本語",
  ko: "한국어",
};

export const SUPPORTED_LOCALES: Locale[] = Object.keys(LOCALES) as Locale[];

/**
 * Maps a raw BCP-47-ish tag — navigator.language, or whatever the bridge's
 * getLanguage() reports as the OS UI language — to the closest locale
 * settings-ui actually ships strings for, falling back to "en". "pt" or
 * "pt-PT" both resolve to "pt-BR" since that's the only Portuguese we
 * have; "en-GB" resolves to "en"; an unsupported language (e.g. "de")
 * falls all the way back to "en".
 */
export function resolveSupportedLocale(tag: string): Locale {
  const normalized = tag.trim();
  const exactMatch = SUPPORTED_LOCALES.find(
    (locale) => locale.toLowerCase() === normalized.toLowerCase(),
  );
  if (exactMatch) return exactMatch;

  const language = normalized.split(/[-_]/)[0]?.toLowerCase();
  const languageMatch = SUPPORTED_LOCALES.find(
    (locale) => locale.split("-")[0].toLowerCase() === language,
  );
  return languageMatch ?? "en";
}
