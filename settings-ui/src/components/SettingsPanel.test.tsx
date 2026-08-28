import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { SettingsPanel } from "./SettingsPanel";
import type { ComponentProps } from "react";
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

type SettingsPanelProps = ComponentProps<typeof SettingsPanel>;

function renderPanel(overrides: Partial<SettingsPanelProps> = {}) {
  const props: SettingsPanelProps = {
    settings: baseSettings(),
    onChange: vi.fn(),
    appVersion: "0.1.0",
    updateCheck: null,
    checkingForUpdate: false,
    applyingUpdate: false,
    onCheckForUpdate: vi.fn(),
    onApplyUpdate: vi.fn(),
    ...overrides,
  };
  render(<SettingsPanel {...props} />);
  return props;
}

describe("SettingsPanel", () => {
  it("marks the theme matching themeOverride as the checked radio", () => {
    renderPanel({ settings: { ...baseSettings(), themeOverride: "dark" } });

    expect(screen.getByRole("radio", { name: "Dark" })).toHaveAttribute("aria-checked", "true");
    expect(screen.getByRole("radio", { name: "System" })).toHaveAttribute("aria-checked", "false");
    expect(screen.getByRole("radio", { name: "Light" })).toHaveAttribute("aria-checked", "false");
  });

  it("calls onChange with the picked theme override", async () => {
    const onChange = vi.fn();
    renderPanel({ onChange });

    await userEvent.click(screen.getByRole("radio", { name: "Light" }));

    expect(onChange).toHaveBeenCalledWith({ themeOverride: "light" });
  });

  it("still wires the existing behavior toggles through onChange", async () => {
    const onChange = vi.fn();
    renderPanel({ onChange });

    await userEvent.click(screen.getByRole("switch", { name: /pause on battery/i }));

    expect(onChange).toHaveBeenCalledWith({ pauseOnBattery: true });
  });

  it("shows the language matching languageOverride as selected", () => {
    renderPanel({ settings: { ...baseSettings(), languageOverride: "pt-BR" } });

    expect(screen.getByLabelText("Language")).toHaveValue("pt-BR");
  });

  it("calls onChange with the picked language override", async () => {
    const onChange = vi.fn();
    renderPanel({ onChange });

    await userEvent.selectOptions(screen.getByLabelText("Language"), "fr");

    expect(onChange).toHaveBeenCalledWith({ languageOverride: "fr" });
  });

  it("shows the current app version", () => {
    renderPanel({ appVersion: "1.2.3" });

    expect(screen.getByText("You're on version 1.2.3")).toBeInTheDocument();
  });

  it("calls onCheckForUpdate when the check button is clicked", async () => {
    const onCheckForUpdate = vi.fn();
    renderPanel({ onCheckForUpdate });

    await userEvent.click(screen.getByRole("button", { name: "Check for updates" }));

    expect(onCheckForUpdate).toHaveBeenCalledTimes(1);
  });

  it("shows an update-available row with an Update button once a check finds one", async () => {
    const onApplyUpdate = vi.fn();
    renderPanel({
      onApplyUpdate,
      updateCheck: {
        checkSucceeded: true,
        updateAvailable: true,
        latestVersion: "0.2.0",
        downloadUrl: "https://example.com/UmbraSetup-0.2.0.exe",
        error: "",
      },
    });

    expect(screen.getByText("Update available: v0.2.0")).toBeInTheDocument();
    await userEvent.click(screen.getByRole("button", { name: "Update and restart" }));
    expect(onApplyUpdate).toHaveBeenCalledTimes(1);
  });

  it("shows an up-to-date message when a check finds nothing", () => {
    renderPanel({
      updateCheck: {
        checkSucceeded: true,
        updateAvailable: false,
        latestVersion: "0.1.0",
        downloadUrl: "",
        error: "",
      },
    });

    expect(screen.getByText("You're up to date.")).toBeInTheDocument();
    expect(screen.queryByRole("button", { name: "Update and restart" })).not.toBeInTheDocument();
  });

  it("shows an error message when the check itself fails", () => {
    renderPanel({
      updateCheck: {
        checkSucceeded: false,
        updateAvailable: false,
        latestVersion: "",
        downloadUrl: "",
        error: "network unreachable",
      },
    });

    expect(
      screen.getByText("Couldn't check for updates — check your connection and try again."),
    ).toBeInTheDocument();
  });
});
