import { describe, expect, it } from "vitest";
import { LOCALES, LOCALE_NAMES, SORTED_LOCALES, SUPPORTED_LOCALES, resolveSupportedLocale } from "./index";

describe("resolveSupportedLocale", () => {
  it("matches a shipped locale tag exactly, case-insensitively", () => {
    expect(resolveSupportedLocale("pt-BR")).toBe("pt-BR");
    expect(resolveSupportedLocale("PT-br")).toBe("pt-BR");
  });

  it("falls back to the base language when the region doesn't match a shipped locale", () => {
    expect(resolveSupportedLocale("pt")).toBe("pt-BR");
    expect(resolveSupportedLocale("pt-PT")).toBe("pt-BR");
    expect(resolveSupportedLocale("en-GB")).toBe("en");
    expect(resolveSupportedLocale("en-US")).toBe("en");
  });

  it("falls back to English for a language we don't ship strings for", () => {
    expect(resolveSupportedLocale("de")).toBe("en");
    expect(resolveSupportedLocale("de-DE")).toBe("en");
    expect(resolveSupportedLocale("")).toBe("en");
  });
});

describe("LOCALES", () => {
  // Every key TypeScript's Strings interface requires is caught at compile
  // time, but not whether a translator accidentally left a value empty —
  // walk every locale's strings and make sure nothing resolved to "".
  function collectLeafValues(node: unknown, path: string, out: Array<[string, unknown]>) {
    if (typeof node === "function") {
      out.push([path, node("x" as never)]);
      return;
    }
    if (typeof node === "object" && node !== null) {
      for (const [key, value] of Object.entries(node)) {
        collectLeafValues(value, path ? `${path}.${key}` : key, out);
      }
      return;
    }
    out.push([path, node]);
  }

  it("has no empty string values in any shipped locale", () => {
    for (const locale of SUPPORTED_LOCALES) {
      const leaves: Array<[string, unknown]> = [];
      collectLeafValues(LOCALES[locale], "", leaves);
      for (const [path, value] of leaves) {
        expect(typeof value, `${locale}:${path} should be a string`).toBe("string");
        expect((value as string).length, `${locale}:${path} should not be empty`).toBeGreaterThan(0);
      }
    }
  });

  it("gives every shipped locale a display name", () => {
    for (const locale of SUPPORTED_LOCALES) {
      expect(LOCALE_NAMES[locale]).toBeTruthy();
    }
  });

  it("sorts SORTED_LOCALES by each locale's own displayed name", () => {
    expect(SORTED_LOCALES).toHaveLength(SUPPORTED_LOCALES.length);
    expect(new Set(SORTED_LOCALES)).toEqual(new Set(SUPPORTED_LOCALES));

    const names = SORTED_LOCALES.map((locale) => LOCALE_NAMES[locale]);
    const expected = [...names].sort((a, b) => a.localeCompare(b));
    expect(names).toEqual(expected);
  });
});
