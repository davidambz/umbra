import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { WallpaperCard } from "./WallpaperCard";
import type { LibraryItem } from "../types";

const item: LibraryItem = { id: "a", title: "Nebula Drift", type: "video" };

describe("WallpaperCard", () => {
  it("does not remove the wallpaper on the first click — it opens a confirmation dialog", async () => {
    const onRemove = vi.fn();
    render(
      <WallpaperCard
        item={item}
        assignedDisplayLabels={[]}
        onRename={vi.fn()}
        onRemove={onRemove}
        onQuickAssign={vi.fn()}
      />,
    );

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(onRemove).not.toHaveBeenCalled();
    expect(screen.getByRole("dialog", { name: 'Delete "Nebula Drift"?' })).toBeInTheDocument();
  });

  it("removes the wallpaper only after confirming", async () => {
    const onRemove = vi.fn();
    render(
      <WallpaperCard
        item={item}
        assignedDisplayLabels={[]}
        onRename={vi.fn()}
        onRemove={onRemove}
        onQuickAssign={vi.fn()}
      />,
    );

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));
    await userEvent.click(screen.getByRole("button", { name: "Delete" }));

    expect(onRemove).toHaveBeenCalledTimes(1);
    expect(screen.queryByRole("dialog")).not.toBeInTheDocument();
  });

  it("cancelling the confirmation leaves the wallpaper in place", async () => {
    const onRemove = vi.fn();
    render(
      <WallpaperCard
        item={item}
        assignedDisplayLabels={[]}
        onRename={vi.fn()}
        onRemove={onRemove}
        onQuickAssign={vi.fn()}
      />,
    );

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));
    await userEvent.click(screen.getByRole("button", { name: "Cancel" }));

    expect(onRemove).not.toHaveBeenCalled();
    expect(screen.queryByRole("dialog")).not.toBeInTheDocument();
  });

  it("warns which displays reference the wallpaper when it's currently assigned", async () => {
    render(
      <WallpaperCard
        item={item}
        assignedDisplayLabels={["Display 1", "Display 2"]}
        onRename={vi.fn()}
        onRemove={vi.fn()}
        onQuickAssign={vi.fn()}
      />,
    );

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(
      screen.getByText("Currently assigned to Display 1 and Display 2 — deleting it will clear that assignment."),
    ).toBeInTheDocument();
  });

  it("shows no assignment warning when the wallpaper isn't assigned anywhere", async () => {
    render(
      <WallpaperCard
        item={item}
        assignedDisplayLabels={[]}
        onRename={vi.fn()}
        onRemove={vi.fn()}
        onQuickAssign={vi.fn()}
      />,
    );

    await userEvent.click(screen.getByRole("button", { name: "Delete Nebula Drift" }));

    expect(screen.queryByText(/Currently assigned to/)).not.toBeInTheDocument();
  });

  it("double-clicking the thumbnail triggers quick-assign", async () => {
    const onQuickAssign = vi.fn();
    render(
      <WallpaperCard
        item={item}
        assignedDisplayLabels={[]}
        onRename={vi.fn()}
        onRemove={vi.fn()}
        onQuickAssign={onQuickAssign}
      />,
    );

    await userEvent.dblClick(screen.getByTitle("Double-click to assign this wallpaper"));

    expect(onQuickAssign).toHaveBeenCalledTimes(1);
  });
});
