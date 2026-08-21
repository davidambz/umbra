import type { MonitorAssignment } from "./types";

/**
 * Removes a deleted wallpaper id from an assignment: clears a "single"
 * assignment pointing at it, and drops it from a "playlist" assignment's
 * rotation (falling back to "none" if that empties the rotation). Shared
 * by App.tsx's live state update and the mock bridge's own persisted
 * state so deleting a wallpaper can't leave a dangling reference in
 * either place.
 */
export function scrubWallpaperFromAssignment(
  assignment: MonitorAssignment,
  removedId: string,
): MonitorAssignment {
  if (assignment.kind === "single") {
    return assignment.wallpaperId === removedId ? { kind: "none" } : assignment;
  }
  if (assignment.kind === "playlist") {
    const wallpaperIds = assignment.playlist.wallpaperIds.filter((id) => id !== removedId);
    if (wallpaperIds.length === 0) {
      return { kind: "none" };
    }
    if (wallpaperIds.length === assignment.playlist.wallpaperIds.length) {
      return assignment;
    }
    return { ...assignment, playlist: { ...assignment.playlist, wallpaperIds } };
  }
  return assignment;
}
