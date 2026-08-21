import { beforeEach, describe, expect, it } from "vitest";
import { createUiBridge } from "./uiBridge";

describe("mock ui bridge", () => {
  beforeEach(() => {
    localStorage.clear();
  });

  it("seeds a non-empty monitor list and library on first use", async () => {
    const bridge = createUiBridge();
    const monitors = await bridge.getMonitors();
    const library = await bridge.getLibrary();
    expect(monitors.length).toBeGreaterThan(0);
    expect(library.length).toBeGreaterThan(0);
  });

  it("round-trips a single-wallpaper assignment", async () => {
    const bridge = createUiBridge();
    const [monitor] = await bridge.getMonitors();
    const [wallpaper] = await bridge.getLibrary();

    await bridge.assignSingle(monitor.id, wallpaper.id, 30);
    const assignment = await bridge.getAssignment(monitor.id);

    expect(assignment).toEqual({ kind: "single", wallpaperId: wallpaper.id, fpsCap: 30 });
  });

  it("clearAssignment resets a monitor back to none", async () => {
    const bridge = createUiBridge();
    const [monitor] = await bridge.getMonitors();
    const [wallpaper] = await bridge.getLibrary();

    await bridge.assignSingle(monitor.id, wallpaper.id, 30);
    await bridge.clearAssignment(monitor.id);

    expect(await bridge.getAssignment(monitor.id)).toEqual({ kind: "none" });
  });

  it("removeWallpaper clears any monitor assignment pointing at it", async () => {
    const bridge = createUiBridge();
    const [monitor] = await bridge.getMonitors();
    const [wallpaper] = await bridge.getLibrary();

    await bridge.assignSingle(monitor.id, wallpaper.id, 30);
    await bridge.removeWallpaper(wallpaper.id);

    expect(await bridge.getAssignment(monitor.id)).toEqual({ kind: "none" });
    expect((await bridge.getLibrary()).find((item) => item.id === wallpaper.id)).toBeUndefined();
  });

  it("removeWallpaper drops the id from a playlist assignment's rotation too", async () => {
    const bridge = createUiBridge();
    const [monitor] = await bridge.getMonitors();
    const [first, second] = await bridge.getLibrary();

    await bridge.assignPlaylist(
      monitor.id,
      { wallpaperIds: [first.id, second.id], intervalSeconds: 300, mode: "sequential" },
      30,
    );
    await bridge.removeWallpaper(first.id);

    expect(await bridge.getAssignment(monitor.id)).toEqual({
      kind: "playlist",
      playlist: { wallpaperIds: [second.id], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    });
  });

  it("importWallpaper adds a new library entry with the given title and type", async () => {
    const bridge = createUiBridge();
    const before = (await bridge.getLibrary()).length;

    const item = await bridge.importWallpaper("Test Wallpaper", "image");

    expect(item?.title).toBe("Test Wallpaper");
    expect(item?.type).toBe("image");
    expect((await bridge.getLibrary()).length).toBe(before + 1);
  });

  it("renameWallpaper updates the title in place", async () => {
    const bridge = createUiBridge();
    const [wallpaper] = await bridge.getLibrary();

    await bridge.renameWallpaper(wallpaper.id, "Renamed");

    const updated = (await bridge.getLibrary()).find((item) => item.id === wallpaper.id);
    expect(updated?.title).toBe("Renamed");
  });

  it("updateSettings merges a partial patch", async () => {
    const bridge = createUiBridge();
    const before = await bridge.getSettings();

    await bridge.updateSettings({ pauseOnBattery: !before.pauseOnBattery });

    const after = await bridge.getSettings();
    expect(after.pauseOnBattery).toBe(!before.pauseOnBattery);
    expect(after.launchOnStartup).toBe(before.launchOnStartup);
  });
});
