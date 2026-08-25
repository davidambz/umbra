import type { LibraryItem, MonitorAssignment } from "./types";

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

function assignmentReferencesWallpaper(assignment: MonitorAssignment, wallpaperId: string): boolean {
  if (assignment.kind === "single") return assignment.wallpaperId === wallpaperId;
  if (assignment.kind === "playlist") return assignment.playlist.wallpaperIds.includes(wallpaperId);
  return false;
}

/**
 * Which monitors (by id) currently reference a wallpaper id — in a
 * single assignment, or anywhere in a playlist's rotation. Used to warn
 * before deleting a wallpaper that's still assigned somewhere, per
 * scrubWallpaperFromAssignment's own note that removal silently clears
 * it from whatever referenced it.
 */
export function findMonitorsReferencingWallpaper(
  assignments: Record<string, MonitorAssignment>,
  wallpaperId: string,
): string[] {
  return Object.entries(assignments)
    .filter(([, assignment]) => assignmentReferencesWallpaper(assignment, wallpaperId))
    .map(([monitorId]) => monitorId);
}

/**
 * Drops every wallpaper id an assignment references that library no
 * longer has — e.g. a monitor still assigned a wallpaper (or holding one
 * in a playlist) that was deleted since the assignment was last saved.
 * Built on scrubWallpaperFromAssignment so "how to remove one id from an
 * assignment" stays defined in exactly one place.
 */
export function scrubStaleReferences(
  assignment: MonitorAssignment,
  library: LibraryItem[],
): MonitorAssignment {
  const knownIds = new Set(library.map((item) => item.id));
  const referencedIds =
    assignment.kind === "single"
      ? [assignment.wallpaperId]
      : assignment.kind === "playlist"
        ? assignment.playlist.wallpaperIds
        : [];
  const missingIds = referencedIds.filter((id) => !knownIds.has(id));
  return missingIds.reduce(scrubWallpaperFromAssignment, assignment);
}
