import { describe, expect, it, vi } from "vitest";
import { fireEvent, render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { PlaylistEditor } from "./PlaylistEditor";
import type { LibraryItem, Playlist } from "../types";

const library: LibraryItem[] = [
  { id: "a", title: "Alpha", type: "video" },
  { id: "b", title: "Beta", type: "image" },
];

function basePlaylist(): Playlist {
  return { wallpaperIds: [], intervalSeconds: 300, mode: "sequential" };
}

describe("PlaylistEditor", () => {
  it("adds a wallpaper to the rotation when its checkbox is checked", async () => {
    const onChange = vi.fn();
    render(<PlaylistEditor playlist={basePlaylist()} library={library} onChange={onChange} />);

    await userEvent.click(screen.getByRole("checkbox", { name: "Alpha" }));

    expect(onChange).toHaveBeenCalledWith({
      wallpaperIds: ["a"],
      intervalSeconds: 300,
      mode: "sequential",
    });
  });

  it("removes a wallpaper from the rotation when unchecked", async () => {
    const onChange = vi.fn();
    const playlist: Playlist = { wallpaperIds: ["a", "b"], intervalSeconds: 300, mode: "sequential" };
    render(<PlaylistEditor playlist={playlist} library={library} onChange={onChange} />);

    await userEvent.click(screen.getByRole("checkbox", { name: "Alpha" }));

    expect(onChange).toHaveBeenCalledWith({ ...playlist, wallpaperIds: ["b"] });
  });

  it("moving the first item later swaps its position with the next one", async () => {
    const onChange = vi.fn();
    const playlist: Playlist = { wallpaperIds: ["a", "b"], intervalSeconds: 300, mode: "sequential" };
    render(<PlaylistEditor playlist={playlist} library={library} onChange={onChange} />);

    const [moveFirstItemLater] = screen.getAllByRole("button", { name: "Move later" });
    await userEvent.click(moveFirstItemLater);

    expect(onChange).toHaveBeenCalledWith({ ...playlist, wallpaperIds: ["b", "a"] });
  });

  it("accepts an interval as low as 1 minute", () => {
    const onChange = vi.fn();
    const playlist: Playlist = { wallpaperIds: ["a"], intervalSeconds: 300, mode: "sequential" };
    render(<PlaylistEditor playlist={playlist} library={library} onChange={onChange} />);

    fireEvent.change(screen.getByRole("spinbutton"), { target: { value: "1" } });

    expect(onChange).toHaveBeenCalledWith({ ...playlist, intervalSeconds: 60 });
  });

  it("clamps an empty or sub-1-minute interval up to 1 minute, not down to 0", () => {
    const onChange = vi.fn();
    const playlist: Playlist = { wallpaperIds: ["a"], intervalSeconds: 300, mode: "sequential" };
    render(<PlaylistEditor playlist={playlist} library={library} onChange={onChange} />);

    fireEvent.change(screen.getByRole("spinbutton"), { target: { value: "" } });

    expect(onChange).toHaveBeenCalledWith({ ...playlist, intervalSeconds: 60 });
  });

  it("disables moving the first item earlier and the last item later", () => {
    const playlist: Playlist = { wallpaperIds: ["a", "b"], intervalSeconds: 300, mode: "sequential" };
    render(<PlaylistEditor playlist={playlist} library={library} onChange={vi.fn()} />);

    const earlierButtons = screen.getAllByRole("button", { name: "Move earlier" });
    const laterButtons = screen.getAllByRole("button", { name: "Move later" });

    expect(earlierButtons[0]).toBeDisabled();
    expect(laterButtons[laterButtons.length - 1]).toBeDisabled();
  });
});
