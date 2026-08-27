import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { SettingsPanel } from "./SettingsPanel";
import type { AppSettings } from "../types";

function baseSettings(): AppSettings {
  return {
    launchOnStartup: true,
    pauseOnFullscreen: true,
    pauseOnBattery: false,
    syncLockScreen: false,
    syncMonitors: false,
    themeOverride: "system",
    languageOverride: "system",
  };
}

describe("SettingsPanel", () => {
  it("marks the theme matching themeOverride as the checked radio", () => {
    render(<SettingsPanel settings={{ ...baseSettings(), themeOverride: "dark" }} onChange={vi.fn()} />);

    expect(screen.getByRole("radio", { name: "Dark" })).toHaveAttribute("aria-checked", "true");
    expect(screen.getByRole("radio", { name: "System" })).toHaveAttribute("aria-checked", "false");
    expect(screen.getByRole("radio", { name: "Light" })).toHaveAttribute("aria-checked", "false");
  });

  it("calls onChange with the picked theme override", async () => {
    const onChange = vi.fn();
    render(<SettingsPanel settings={baseSettings()} onChange={onChange} />);

    await userEvent.click(screen.getByRole("radio", { name: "Light" }));

    expect(onChange).toHaveBeenCalledWith({ themeOverride: "light" });
  });

  it("still wires the existing behavior toggles through onChange", async () => {
    const onChange = vi.fn();
    render(<SettingsPanel settings={baseSettings()} onChange={onChange} />);

    await userEvent.click(screen.getByRole("switch", { name: /pause on battery/i }));

    expect(onChange).toHaveBeenCalledWith({ pauseOnBattery: true });
  });

  it("shows the language matching languageOverride as selected", () => {
    render(
      <SettingsPanel settings={{ ...baseSettings(), languageOverride: "pt-BR" }} onChange={vi.fn()} />,
    );

    expect(screen.getByLabelText("Language")).toHaveValue("pt-BR");
  });

  it("calls onChange with the picked language override", async () => {
    const onChange = vi.fn();
    render(<SettingsPanel settings={baseSettings()} onChange={onChange} />);

    await userEvent.selectOptions(screen.getByLabelText("Language"), "fr");

    expect(onChange).toHaveBeenCalledWith({ languageOverride: "fr" });
  });
});
