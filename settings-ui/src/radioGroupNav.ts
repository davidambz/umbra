import type { KeyboardEvent } from "react";

// WAI-ARIA APG radiogroup keyboard behavior: arrow keys move both focus
// and selection among the group's role="radio" buttons (selection follows
// focus, unlike a plain tablist). Shared by AddWallpaperDialog's type
// picker and AssignDialog's mode/FPS-cap pickers rather than duplicating
// this in each — same reasoning as extracting wallpaperTypeStyles.ts.
export function handleRadioGroupKeyDown(event: KeyboardEvent<HTMLElement>) {
  if (
    event.key !== "ArrowRight" &&
    event.key !== "ArrowDown" &&
    event.key !== "ArrowLeft" &&
    event.key !== "ArrowUp"
  ) {
    return;
  }
  const radios = Array.from(
    event.currentTarget.querySelectorAll<HTMLButtonElement>('[role="radio"]:not([disabled])'),
  );
  const currentIndex = radios.indexOf(document.activeElement as HTMLButtonElement);
  if (currentIndex === -1) return;
  event.preventDefault();
  const delta = event.key === "ArrowRight" || event.key === "ArrowDown" ? 1 : -1;
  const next = radios[(currentIndex + delta + radios.length) % radios.length];
  next?.focus();
  next?.click();
}
