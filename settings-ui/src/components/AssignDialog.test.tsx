import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { AssignDialog } from "./AssignDialog";
import type { LibraryItem, MonitorInfo } from "../types";

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
        assignment={{ kind: "none" }}
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
        assignment={{ kind: "none" }}
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
        assignment={{ kind: "none" }}
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

  it("pre-selects the chosen wallpaper for the quick-assign flow", () => {
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignment={{ kind: "none" }}
        library={library}
        monitorSelectable
        modeSelectable={false}
        initialMode="single"
        initialWallpaperId="b"
        onClose={vi.fn()}
        onSave={vi.fn()}
      />,
    );

    expect(screen.getByRole("radio", { name: "Tidal Glass" })).toBeChecked();
  });

  it("saves to the monitor picked in the selector, not just initialMonitorId", async () => {
    const onSave = vi.fn().mockResolvedValue(true);
    render(
      <AssignDialog
        monitors={twoMonitors}
        initialMonitorId="m1"
        assignment={{ kind: "none" }}
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
});
