import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { UpdatesPanel } from "./UpdatesPanel";
import type { ComponentProps } from "react";

type UpdatesPanelProps = ComponentProps<typeof UpdatesPanel>;

function renderPanel(overrides: Partial<UpdatesPanelProps> = {}) {
  const props: UpdatesPanelProps = {
    appVersion: "0.1.0",
    updateCheck: null,
    checking: false,
    applying: false,
    onCheckForUpdate: vi.fn(),
    onApplyUpdate: vi.fn(),
    ...overrides,
  };
  render(<UpdatesPanel {...props} />);
  return props;
}

describe("UpdatesPanel", () => {
  it("shows nothing but the version and check button before any check runs", () => {
    renderPanel();

    expect(screen.getByText("You're on version 0.1.0")).toBeInTheDocument();
    expect(screen.queryByText(/up to date|Update available/)).not.toBeInTheDocument();
  });

  it("disables the check button and shows a checking label while a check is in flight", () => {
    renderPanel({ checking: true });

    const button = screen.getByRole("button", { name: "Checking for updates…" });
    expect(button).toBeDisabled();
  });

  it("disables Update and shows an installing label while applying", () => {
    renderPanel({
      applying: true,
      updateCheck: {
        checkSucceeded: true,
        updateAvailable: true,
        latestVersion: "0.2.0",
        downloadUrl: "https://example.com/UmbraSetup-0.2.0.exe",
        error: "",
      },
    });

    expect(
      screen.getByText("Downloading and installing the update — Umbra will restart automatically."),
    ).toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Update and restart" })).toBeDisabled();
  });

  it("calls onApplyUpdate when Update and restart is clicked", async () => {
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

    await userEvent.click(screen.getByRole("button", { name: "Update and restart" }));

    expect(onApplyUpdate).toHaveBeenCalledTimes(1);
  });
});
