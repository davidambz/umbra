import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { AssignDialog } from "./AssignDialog";
import type { ComponentProps } from "react";
import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";

const library: LibraryItem[] = [
  { id: "a", title: "Nebula Drift", type: "video", thumbnailUrl: "nebula.png" },
  { id: "b", title: "Tidal Glass", type: "image", thumbnailUrl: "tidal.png" },
];

const oneMonitor: MonitorInfo[] = [
  { id: "m1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
];

const twoMonitors: MonitorInfo[] = [
  { id: "m1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
  { id: "m2", x: 1920, y: 0, width: 1920, height: 1080, isPrimary: false },
];

type AssignDialogProps = ComponentProps<typeof AssignDialog>;

function renderDialog(overrides: Partial<AssignDialogProps> & Pick<AssignDialogProps, "monitors">) {
  const props: AssignDialogProps = {
    initialMonitorId: "m1",
    assignments: {},
    library,
    syncMonitors: false,
    onSyncMonitorsChange: vi.fn(),
    onClose: vi.fn(),
    onSave: vi.fn(),
    ...overrides,
  };
  render(<AssignDialog {...props} />);
  return props;
}

describe("AssignDialog", () => {
  it("does not show a monitor picker for the ordinary MonitorGrid flow", () => {
    renderDialog({ monitors: twoMonitors });

    expect(screen.queryByRole("radiogroup", { name: "Assign to" })).not.toBeInTheDocument();
  });

  it("does not show a monitor picker for the quick-assign flow when only one monitor exists", () => {
    renderDialog({
      monitors: oneMonitor,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "a",
    });

    expect(screen.queryByRole("radiogroup", { name: "Assign to" })).not.toBeInTheDocument();
  });

  it("hides the None/Single/Playlist mode tabs for the quick-assign flow", () => {
    renderDialog({
      monitors: twoMonitors,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "b",
    });

    expect(screen.queryByRole("radiogroup", { name: "Assignment mode" })).not.toBeInTheDocument();
  });

  it("hides the wallpaper picker list for the quick-assign flow — it's already chosen", () => {
    renderDialog({
      monitors: twoMonitors,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "b",
    });

    expect(screen.queryByRole("radio", { name: "Tidal Glass" })).not.toBeInTheDocument();
    expect(screen.queryByRole("radio", { name: "Nebula Drift" })).not.toBeInTheDocument();
  });

  it("saves the preselected wallpaper even though its picker is hidden", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    renderDialog({
      monitors: twoMonitors,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "b",
      onSave,
    });

    await userEvent.click(screen.getByRole("button", { name: "Save" }));

    expect(onSave).toHaveBeenCalledWith("m1", { kind: "single", wallpaperId: "b", fpsCap: 30 });
  });

  it("saves to the monitor picked in the selector, not just initialMonitorId", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    renderDialog({
      monitors: twoMonitors,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "a",
      onSave,
    });

    await userEvent.click(screen.getByRole("radio", { name: /Display 2/ }));
    await userEvent.click(screen.getByRole("button", { name: "Save" }));

    expect(onSave).toHaveBeenCalledWith("m2", { kind: "single", wallpaperId: "a", fpsCap: 30 });
  });

  it("defaults the FPS cap from the initial monitor's real current assignment, not a hardcoded 30", () => {
    const assignments: Record<string, MonitorAssignment> = {
      m1: { kind: "single", wallpaperId: "a", fpsCap: 60 },
    };
    renderDialog({
      monitors: twoMonitors,
      assignments,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "b",
    });

    expect(screen.getByRole("radio", { name: "60" })).toHaveAttribute("aria-checked", "true");
  });

  it("previews the wallpaper being assigned on every monitor option, not each monitor's stale current one", () => {
    const assignments: Record<string, MonitorAssignment> = {
      m1: { kind: "single", wallpaperId: "a", fpsCap: 30 },
      m2: { kind: "none" },
    };
    renderDialog({
      monitors: twoMonitors,
      assignments,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "b",
    });

    const previews = document.querySelectorAll<HTMLImageElement>('[role="radiogroup"] img');
    expect(previews).toHaveLength(2);
    for (const preview of previews) {
      expect(preview.src).toContain("tidal.png");
    }
  });

  it("switching the target monitor updates the FPS cap default to match that monitor", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    const assignments: Record<string, MonitorAssignment> = {
      m1: { kind: "single", wallpaperId: "a", fpsCap: 15 },
      m2: { kind: "single", wallpaperId: "a", fpsCap: 60 },
    };
    renderDialog({
      monitors: twoMonitors,
      assignments,
      monitorSelectable: true,
      modeSelectable: false,
      initialMode: "single",
      initialWallpaperId: "b",
      onSave,
    });

    await userEvent.click(screen.getByRole("radio", { name: /Display 2/ }));
    await userEvent.click(screen.getByRole("button", { name: "Save" }));

    expect(onSave).toHaveBeenCalledWith("m2", { kind: "single", wallpaperId: "b", fpsCap: 60 });
  });

  it("does not show the Sync monitors toggle when there's only one monitor", () => {
    renderDialog({ monitors: oneMonitor });

    expect(screen.queryByText("Sync monitors")).not.toBeInTheDocument();
  });

  it("shows the Sync monitors toggle reflecting the live setting when there's more than one monitor", () => {
    renderDialog({ monitors: twoMonitors, syncMonitors: true });

    expect(screen.getByRole("switch", { name: /Sync monitors/ })).toBeChecked();
  });

  it("flipping the Sync monitors toggle calls onSyncMonitorsChange, not local state", async () => {
    const onSyncMonitorsChange = vi.fn();
    renderDialog({ monitors: twoMonitors, syncMonitors: false, onSyncMonitorsChange });

    await userEvent.click(screen.getByRole("switch", { name: /Sync monitors/ }));

    expect(onSyncMonitorsChange).toHaveBeenCalledWith(true);
  });
});
