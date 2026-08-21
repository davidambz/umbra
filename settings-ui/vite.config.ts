/// <reference types="vitest/config" />
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  // The built bundle is loaded via file:// inside a WebView2 control (see
  // src/ui/settings_window.* in #9), not served from a domain root — every
  // asset reference needs to resolve relative to index.html.
  base: "./",
  test: {
    environment: "jsdom",
    globals: true,
    setupFiles: ["./src/test/setup.ts"],
  },
});
