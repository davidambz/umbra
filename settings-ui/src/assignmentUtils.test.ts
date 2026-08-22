import { describe, expect, it } from "vitest";
import { scrubStaleReferences, scrubWallpaperFromAssignment } from "./assignmentUtils";
import type { LibraryItem, MonitorAssignment } from "./types";

describe("scrubWallpaperFromAssignment", () => {
  it("leaves a none assignment untouched", () => {
    const assignment: MonitorAssignment = { kind: "none" };
    expect(scrubWallpaperFromAssignment(assignment, "a")).toEqual(assignment);
  });

  it("clears a single assignment pointing at the removed id", () => {
    const assignment: MonitorAssignment = { kind: "single", wallpaperId: "a", fpsCap: 30 };
    expect(scrubWallpaperFromAssignment(assignment, "a")).toEqual({ kind: "none" });
  });

  it("leaves a single assignment pointing elsewhere untouched", () => {
    const assignment: MonitorAssignment = { kind: "single", wallpaperId: "a", fpsCap: 30 };
    expect(scrubWallpaperFromAssignment(assignment, "b")).toEqual(assignment);
  });

  it("drops the removed id from a playlist's rotation", () => {
    const assignment: MonitorAssignment = {
      kind: "playlist",
      playlist: { wallpaperIds: ["a", "b"], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    };
    expect(scrubWallpaperFromAssignment(assignment, "a")).toEqual({
      kind: "playlist",
      playlist: { wallpaperIds: ["b"], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    });
  });

  it("falls back to none when removing the id empties the playlist", () => {
    const assignment: MonitorAssignment = {
      kind: "playlist",
      playlist: { wallpaperIds: ["a"], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    };
    expect(scrubWallpaperFromAssignment(assignment, "a")).toEqual({ kind: "none" });
  });

  it("leaves a playlist untouched when it doesn't contain the removed id", () => {
    const assignment: MonitorAssignment = {
      kind: "playlist",
      playlist: { wallpaperIds: ["a", "b"], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    };
    expect(scrubWallpaperFromAssignment(assignment, "z")).toEqual(assignment);
  });
});

describe("scrubStaleReferences", () => {
  const library: LibraryItem[] = [{ id: "a", title: "Alpha", type: "video" }];

  it("leaves a single assignment untouched when its id is still in the library", () => {
    const assignment: MonitorAssignment = { kind: "single", wallpaperId: "a", fpsCap: 30 };
    expect(scrubStaleReferences(assignment, library)).toEqual(assignment);
  });

  it("clears a single assignment whose id is no longer in the library", () => {
    const assignment: MonitorAssignment = { kind: "single", wallpaperId: "gone", fpsCap: 30 };
    expect(scrubStaleReferences(assignment, library)).toEqual({ kind: "none" });
  });

  it("drops only the missing ids from a playlist, keeping the rest", () => {
    const assignment: MonitorAssignment = {
      kind: "playlist",
      playlist: { wallpaperIds: ["a", "gone"], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    };
    expect(scrubStaleReferences(assignment, library)).toEqual({
      kind: "playlist",
      playlist: { wallpaperIds: ["a"], intervalSeconds: 300, mode: "sequential" },
      fpsCap: 30,
    });
  });
});
