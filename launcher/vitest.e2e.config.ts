// The launcher's pages in a browser (e2e/): Android's, and the desktop's tabs with a list:
// `yarn test:android`. Apart from `yarn test`, which needs no browser.
import { defineConfig } from "vitest/config";

export default defineConfig({
  test: {
    include: ["e2e/**/*.test.ts"],
    environment: "node",
    testTimeout: 20_000,
    // One file at a time: each starts a Vite server, and two pre-bundling into the same cache at
    // once fail with ENOTEMPTY renaming node_modules/.vite/deps_ssr.
    fileParallelism: false,
  },
});
