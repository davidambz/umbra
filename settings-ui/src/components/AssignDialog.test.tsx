import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { AssignDialog } from "./AssignDialog";
import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";

const library: LibraryItem[] = [
  { id: "a", title: "Nebula Drift", type: "video" },
  { id: "b", title: "Tidal Glass", type: "image" },
];

const oneMonitor: MonitorInfo[] = [
  { id: "m1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
];

const twoMonitors: MonitorInfo[] = [
  { id: "m1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
  { id: "m2", x: 1920, y: 0, width: 1920, height: 1080, isPrimary: false },
];

describe("AssignDialog", () => {
  it("does not show a monitor picker for the ordinary MonitorGrid flow", () => {
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={{}}
        library={library}
        onClose={vi.fn()}
        onSave={vi.fn()}
      />,
    );

    expect(screen.queryByRole("radiogroup", { name: "Assign to" })).not.toBeInTheDocument();
  });

  it("does not show a monitor picker for the quick-assign flow when only one monitor exists", () => {
    render(
      <AssignDialog
        monitors={oneMonitor}
        initialMonitorId="m1"
        assignments={{}}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="a"
        onClose={vi.fn()}
        onSave={vi.fn()}
      />,
    );

    expect(screen.queryByRole("radiogroup", { name: "Assign to" })).not.toBeInTheDocument();
  });

  it("hides the None/Single/Playlist mode tabs for the quick-assign flow", () => {
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={{}}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="b"
        onClose={vi.fn()}
        onSave={vi.fn()}
      />,
    );

    expect(screen.queryByRole("radiogroup", { name: "Assignment mode" })).not.toBeInTheDocument();
  });

  it("hides the wallpaper picker list for the quick-assign flow — it's already chosen", () => {
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={{}}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="b"
        onClose={vi.fn()}
        onSave={vi.fn()}
      />,
    );

    expect(screen.queryByRole("radio", { name: "Tidal Glass" })).not.toBeInTheDocument();
    expect(screen.queryByRole("radio", { name: "Nebula Drift" })).not.toBeInTheDocument();
  });

  it("saves the preselected wallpaper even though its picker is hidden", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={{}}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="b"
        onClose={vi.fn()}
        onSave={onSave}
      />,
    );

    await userEvent.click(screen.getByRole("button", { name: "Save" }));

    expect(onSave).toHaveBeenCalledWith("m1", { kind: "single", wallpaperId: "b", fpsCap: 30 });
  });

  it("saves to the monitor picked in the selector, not just initialMonitorId", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={{}}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="a"
        onClose={vi.fn()}
        onSave={onSave}
      />,
    );

    await userEvent.click(screen.getByRole("radio", { name: /Display 2/ }));
    await userEvent.click(screen.getByRole("button", { name: "Save" }));

    expect(onSave).toHaveBeenCalledWith("m2", { kind: "single", wallpaperId: "a", fpsCap: 30 });
  });

  it("defaults the FPS cap from the initial monitor's real current assignment, not a hardcoded 30", () => {
    const assignments: Record<string, MonitorAssignment> = {
      m1: { kind: "single", wallpaperId: "a", fpsCap: 60 },
    };
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={assignments}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="b"
        onClose={vi.fn()}
        onSave={vi.fn()}
      />,
    );

    expect(screen.getByRole("radio", { name: "60" })).toHaveAttribute("aria-checked", "true");
  });

  it("switching the target monitor updates the FPS cap default to match that monitor", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    const assignments: Record<string, MonitorAssignment> = {
      m1: { kind: "single", wallpaperId: "a", fpsCap: 15 },
      m2: { kind: "single", wallpaperId: "a", fpsCap: 60 },
    };
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignments={assignments}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="b"
        onClose={vi.fn()}
        onSave={onSave}
      />,
    );

    await userEvent.click(screen.getByRole("radio", { name: /Display 2/ }));
    await userEvent.click(screen.getByRole("button", { name: "Save" }));

    expect(onSave).toHaveBeenCalledWith("m2", { kind: "single", wallpaperId: "b", fpsCap: 60 });
  });
});
