// Small inline icon set for the tab bar (App.tsx) — kept here rather than
// pulling in an icon library, matching the rest of settings-ui/'s
// zero-dependency approach (see uiBridge.ts's own doc comment). Outline
// style, 20x20, stroke=currentColor so each icon inherits the tab
// button's own text color (including its active/hover states) with no
// extra color wiring.

interface IconProps {
  className?: string;
}

export function WallpapersIcon({ className }: IconProps) {
  return (
    <svg
      className={className}
      width="20"
      height="20"
      viewBox="0 0 20 20"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <rect x="2.5" y="3.5" width="15" height="13" rx="2" />
      <circle cx="7" cy="8" r="1.4" />
      <path d="M3 14.5l4.5-4.5a1.5 1.5 0 0 1 2.1 0l2.4 2.4" />
      <path d="M9.5 14.5l3-3a1.5 1.5 0 0 1 2.1 0l2.4 2.4" />
    </svg>
  );
}

export function SettingsIcon({ className }: IconProps) {
  return (
    <svg
      className={className}
      width="20"
      height="20"
      viewBox="0 0 20 20"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.5"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <circle cx="10" cy="10" r="2.5" />
      <path
        d="M10 2.5a1.3 1.3 0 0 1 1.29 1.14l.09.73a6.4 6.4 0 0 1 1.66.68l.6-.42a1.3 1.3 0 0 1 1.72.16l.65.65a1.3 1.3 0 0 1 .16 1.72l-.42.6a6.4 6.4 0 0 1 .68 1.66l.73.09a1.3 1.3 0 0 1 1.14 1.29 1.3 1.3 0 0 1-1.14 1.29l-.73.09a6.4 6.4 0 0 1-.68 1.66l.42.6a1.3 1.3 0 0 1-.16 1.72l-.65.65a1.3 1.3 0 0 1-1.72.16l-.6-.42a6.4 6.4 0 0 1-1.66.68l-.09.73a1.3 1.3 0 0 1-1.29 1.14 1.3 1.3 0 0 1-1.29-1.14l-.09-.73a6.4 6.4 0 0 1-1.66-.68l-.6.42a1.3 1.3 0 0 1-1.72-.16l-.65-.65a1.3 1.3 0 0 1-.16-1.72l.42-.6a6.4 6.4 0 0 1-.68-1.66l-.73-.09A1.3 1.3 0 0 1 2.5 10a1.3 1.3 0 0 1 1.14-1.29l.73-.09a6.4 6.4 0 0 1 .68-1.66l-.42-.6a1.3 1.3 0 0 1 .16-1.72l.65-.65a1.3 1.3 0 0 1 1.72-.16l.6.42a6.4 6.4 0 0 1 1.66-.68l.09-.73A1.3 1.3 0 0 1 10 2.5z"
      />
    </svg>
  );
}
