import { describe, expect, it } from "vitest";
import { render, waitFor } from "@testing-library/react";
import { useSystemTheme } from "./useSystemTheme";
import type { UiBridge } from "./uiBridge";

function fakeBridge(theme: "light" | "dark"): UiBridge {
  return {
    getMonitors: async () => [],
    getLibrary: async () => [],
    getAssignment: async () => ({ kind: "none" }),
    getSettings: async () => ({
      launchOnStartup: false,
      pauseOnFullscreen: false,
      pauseOnBattery: false,
    }),
    getTheme: async () => theme,
    onThemeChange: () => () => {},
    assignSingle: async () => {},
    assignPlaylist: async () => {},
    clearAssignment: async () => {},
    importWallpaper: async () => null,
    renameWallpaper: async () => {},
    removeWallpaper: async () => {},
    updateSettings: async () => {},
  };
}

function Probe({ bridge }: { bridge: UiBridge }) {
  const theme = useSystemTheme(bridge);
  return <span data-testid="theme">{theme}</span>;
}

describe("useSystemTheme", () => {
  it("adopts the theme the bridge resolves to and reflects it on <html>", async () => {
    const { getByTestId } = render(<Probe bridge={fakeBridge("dark")} />);

    // Both assertions belong inside waitFor: the state update (the span's
    // text) and the attribute-setting effect it triggers land in the same
    // render's effect flush, but as two separate effects there's no
    // guarantee testing-library's DOM-mutation-driven waitFor wakes up only
    // once both have run — checking the attribute on its own right after
    // could observe it a tick early.
    await waitFor(() => {
      expect(getByTestId("theme").textContent).toBe("dark");
      expect(document.documentElement.getAttribute("data-theme")).toBe("dark");
    });
  });

  it("switches the <html> attribute when the bridge resolves to light", async () => {
    const { getByTestId } = render(<Probe bridge={fakeBridge("light")} />);

    await waitFor(() => {
      expect(getByTestId("theme").textContent).toBe("light");
      expect(document.documentElement.getAttribute("data-theme")).toBe("light");
    });
  });
});
