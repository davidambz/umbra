# settings-ui

The React front-end rendered inside Umbra's settings window (a WebView2
control — see `ui_bridge` in `src/ui/`, not yet implemented). Standalone
static site: no backend, no network calls, built and shipped as part of
the installer per `ARCHITECTURE.md`.

## Commands

```
npm install
npm run dev      # local dev server, using an in-memory mock of the native bridge
npm run test     # vitest
npm run build    # typecheck + production build into dist/
```

## Native bridge

`src/bridge/uiBridge.ts` defines the `UiBridge` contract this UI needs
from the native side. Until `ui_bridge` exists, `createUiBridge()` falls
back to an in-memory mock (persisted to `localStorage`) so every screen
here is buildable and demoable in a plain browser. Once `ui_bridge` is
implemented, it injects the real implementation as `window.umbra` before
this page loads, and `createUiBridge()` picks it up automatically — no
code here needs to change.

## Design tokens

`src/styles/tokens.css` is the single source of truth for color, type,
spacing, and radius — every component reads these CSS variables rather
than hardcoding values. Theme resolution (dark/light, following the
Windows system theme) is handled by `src/bridge/useSystemTheme.ts`.
