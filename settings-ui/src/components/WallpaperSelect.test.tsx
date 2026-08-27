import { describe, expect, it, vi } from "vitest";
import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { WallpaperSelect } from "./WallpaperSelect";
import type { LibraryItem } from "../types";

const library: LibraryItem[] = [
  { id: "a", title: "Nebula Drift", type: "video" },
  { id: "b", title: "Tidal Glass", type: "image" },
];

describe("WallpaperSelect", () => {
  it("shows every library item as an option once opened", async () => {
    render(<WallpaperSelect library={library} value="a" onChange={vi.fn()} label="Wallpaper" />);

    await userEvent.click(screen.getByRole("button", { name: "Wallpaper" }));

    expect(screen.getByRole("option", { name: "Nebula Drift" })).toBeInTheDocument();
    expect(screen.getByRole("option", { name: "Tidal Glass" })).toBeInTheDocument();
  });

  it("calls onChange and closes the listbox when an option is picked", async () => {
    const onChange = vi.fn();
    render(<WallpaperSelect library={library} value="a" onChange={onChange} label="Wallpaper" />);

    await userEvent.click(screen.getByRole("button", { name: "Wallpaper" }));
    await userEvent.click(screen.getByRole("option", { name: "Tidal Glass" }));

    expect(onChange).toHaveBeenCalledWith("b");
    expect(screen.queryByRole("listbox")).not.toBeInTheDocument();
  });

  it("returns focus to the trigger after picking an option", async () => {
    render(<WallpaperSelect library={library} value="a" onChange={vi.fn()} label="Wallpaper" />);

    await userEvent.click(screen.getByRole("button", { name: "Wallpaper" }));
    await userEvent.click(screen.getByRole("option", { name: "Tidal Glass" }));

    expect(screen.getByRole("button", { name: "Wallpaper" })).toHaveFocus();
  });

  it("closes and returns focus to the trigger on Escape, without letting the keypress escape further", async () => {
    const onEscapeFromOutside = vi.fn();
    document.addEventListener("keydown", onEscapeFromOutside);
    render(<WallpaperSelect library={library} value="a" onChange={vi.fn()} label="Wallpaper" />);

    await userEvent.click(screen.getByRole("button", { name: "Wallpaper" }));
    await userEvent.keyboard("{Escape}");

    expect(screen.queryByRole("listbox")).not.toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Wallpaper" })).toHaveFocus();
    expect(onEscapeFromOutside).not.toHaveBeenCalled();

    document.removeEventListener("keydown", onEscapeFromOutside);
  });

  it("closes without stealing focus when clicking outside", async () => {
    render(
      <div>
        <button type="button">Outside</button>
        <WallpaperSelect library={library} value="a" onChange={vi.fn()} label="Wallpaper" />
      </div>,
    );

    await userEvent.click(screen.getByRole("button", { name: "Wallpaper" }));
    await userEvent.click(screen.getByRole("button", { name: "Outside" }));

    expect(screen.queryByRole("listbox")).not.toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Outside" })).toHaveFocus();
  });
});
