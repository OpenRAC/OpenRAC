/// <reference types="vitest/config" />
import { svelte } from "@sveltejs/vite-plugin-svelte";
import { defineConfig } from "vite";

// Tauri expects a fixed dev port (tauri.conf.json's devUrl) and serves dist/ in release builds.
export default defineConfig({
  plugins: [
    svelte(),
    {
      name: "svelte-virtual-css-fallback",
      enforce: "post",
      configureServer(server) {
        server.middlewares.use((req, _res, next) => {
          const rawReq = req as { url?: string; originalUrl?: string };
          const reqUrl = rawReq.originalUrl ?? rawReq.url;
          const url = reqUrl?.split("?")[0];
          const query = reqUrl?.slice((url?.length ?? 0) + 1);
          if (url?.endsWith(".svelte") && query?.includes("svelte&type=style")) {
            void server.transformRequest(url);
          }
          next();
        });
      },
    },
  ],
  resolve: {
    // The same aliases as tsconfig.json's "paths".
    alias: { $lib: "/src/lib", $components: "/src/components" },
  },
  clearScreen: false,
  server: {
    port: 1430,
    strictPort: true,
    // The browser preview (src/lib/mock.ts) reads games/*/game.json and
    // progress/summary.json from the checkout around the launcher.
    fs: { allow: [".."] },
    // Cargo's folders: Tauri watches its own sources, and target/ changes during every build.
    watch: { ignored: ["**/src-tauri/**", "**/core/**", "**/target/**"] },
  },
  envPrefix: ["VITE_", "TAURI_ENV_"],
  build: { target: "es2023", outDir: "dist", emptyOutDir: true },
  test: { include: ["src/**/*.test.ts"], environment: "node" },
});
