import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { WallpaperLibrary } from "./WallpaperLibrary";
import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";

const library: LibraryItem[] = [{ id: "a", title: "Nebula Drift", type: "video" }];

const monitors: MonitorInfo[] = [
  { id: "m1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
  { id: "m2", x: 1920, y: 0, width: 1920, height: 1080, isPrimary: false },
];

function renderLibrary(assignments: Record<string, MonitorAssignment>) {
  render(
    <WallpaperLibrary
      library={library}
      monitors={monitors}
      assignments={assignments}
      onAdd={vi.fn()}
      onRename={vi.fn()}
      onRemove={vi.fn()}
    />,
  );
}

describe("WallpaperLibrary", () => {
  it("warns with the primary display's label when it's assigned there", async () => {
    renderLibrary({ m1: { kind: "single", wallpaperId: "a", fpsCap: 30 } });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.getByText(/Currently assigned to Primary display/)).toBeInTheDocument();
  });

  it("warns with a secondary display's 1-based index label", async () => {
    renderLibrary({ m2: { kind: "single", wallpaperId: "a", fpsCap: 30 } });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.getByText(/Currently assigned to Display 2/)).toBeInTheDocument();
  });

  it("falls back to a generic label for a monitor id no longer in the current monitor list", async () => {
    renderLibrary({ unplugged: { kind: "single", wallpaperId: "a", fpsCap: 30 } });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.getByText(/Currently assigned to a disconnected display/)).toBeInTheDocument();
  });

  it("shows no warning when nothing references the wallpaper", async () => {
    renderLibrary({ m1: { kind: "none" } });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.queryByText(/Currently assigned to/)).not.toBeInTheDocument();
  });
});
