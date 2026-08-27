import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { WallpaperLibrary } from "./WallpaperLibrary";
import { I18nProvider } from "../i18n/I18nProvider";
import type { LibraryItem, MonitorAssignment, MonitorInfo } from "../types";

const library: LibraryItem[] = [{ id: "a", title: "Nebula Drift", type: "video" }];

const monitors: MonitorInfo[] = [
  { id: "m1", x: 0, y: 0, width: 1920, height: 1080, isPrimary: true },
  { id: "m2", x: 1920, y: 0, width: 1920, height: 1080, isPrimary: false },
];

function renderLibrary(assignments: Record<string, MonitorAssignment>, onQuickAssign = vi.fn()) {
  render(
    <WallpaperLibrary
      library={library}
      monitors={monitors}
      assignments={assignments}
      onAdd={vi.fn()}
      onRename={vi.fn()}
      onRemove={vi.fn()}
      onQuickAssign={onQuickAssign}
    />,
  );
}

describe("WallpaperLibrary", () => {
  it("warns with the first display's 1-based index label", async () => {
    renderLibrary({ m1: { kind: "single", wallpaperId: "a", fpsCap: 30 } });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.getByText(/Currently assigned to Display 1/)).toBeInTheDocument();
  });

  it("warns with a secondary display's 1-based index label", async () => {
    renderLibrary({ m2: { kind: "single", wallpaperId: "a", fpsCap: 30 } });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.getByText(/Currently assigned to Display 2/)).toBeInTheDocument();
  });

  it("orders multiple assigned displays by display order, not by assignment insertion order", async () => {
    // m2 listed before m1 here — the label order must still come out
    // Display 1, then Display 2, matching monitors' own order.
    renderLibrary({
      m2: { kind: "single", wallpaperId: "a", fpsCap: 30 },
      m1: { kind: "single", wallpaperId: "a", fpsCap: 30 },
    });

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(
      screen.getByText("Currently assigned to Display 1 and Display 2 — deleting it will clear that assignment."),
    ).toBeInTheDocument();
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

  it("double-clicking a wallpaper's thumbnail quick-assigns that item", async () => {
    const onQuickAssign = vi.fn();
    renderLibrary({}, onQuickAssign);

    await userEvent.dblClick(screen.getByTitle("Double-click to assign this wallpaper"));

    expect(onQuickAssign).toHaveBeenCalledWith(library[0]);
  });

  it("renders through I18nProvider in the active locale", () => {
    render(
      <I18nProvider locale="pt-BR">
        <WallpaperLibrary
          library={library}
          monitors={monitors}
          assignments={{}}
          onAdd={vi.fn()}
          onRename={vi.fn()}
          onRemove={vi.fn()}
          onQuickAssign={vi.fn()}
        />
      </I18nProvider>,
    );

    expect(screen.getByRole("heading", { name: "Biblioteca" })).toBeInTheDocument();
    expect(screen.getByRole("button", { name: "+ Adicionar papel de parede" })).toBeInTheDocument();
  });
});
