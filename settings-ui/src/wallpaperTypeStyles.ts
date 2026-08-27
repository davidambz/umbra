// Shared per-type visuals for any wallpaper preview that has no thumbnail
// yet (MonitorCard, WallpaperCard) — a single source so the two card faces
// can't drift out of sync with each other (previously duplicated verbatim
// in both files). Distinct brightness bands, not just distinct hues, so
// the three read apart at a glance even reduced to a thumbnail-sized
// swatch — the panel's neutral steel tones, not the violet accent, which
// stays reserved for lit/active state.
export const TYPE_GRADIENT: Record<string, string> = {
  image: "linear-gradient(155deg, #55525f 0%, #2b2932 100%)",
  video: "linear-gradient(155deg, #35323c 0%, #17151c 100%)",
  web: "linear-gradient(155deg, #211f28 0%, #0d0c11 100%)",
};

export const TYPE_LABEL: Record<string, string> = { video: "Video", image: "Image", web: "Web" };
