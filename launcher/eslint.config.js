// ESLint flat config: typed TypeScript rules for .ts and .svelte files, Svelte's own rules,
// sorted imports and blank lines between a function's steps. Line layout is Prettier's job;
// the spacing rules here are the ones Prettier leaves alone, and all of them fix themselves
// (yarn lint --fix).
import js from "@eslint/js";
import stylistic from "@stylistic/eslint-plugin";
import { defineConfig } from "eslint/config";
import simpleImportSort from "eslint-plugin-simple-import-sort";
import svelte from "eslint-plugin-svelte";
import globals from "globals";
import ts from "typescript-eslint";

import svelteConfig from "./svelte.config.js";

export default defineConfig(
  {
    ignores: [
      "dist/",
      "scripts/",
      "src/lib/generated/",
      "src-tauri/",
      "core/",
      "target/",
      "node_modules/",
      ".svelte-check/",
      ".pnp.*",
      ".yarn/",
    ],
  },
  js.configs.recommended,
  ts.configs.strictTypeChecked,
  ts.configs.stylisticTypeChecked,
  svelte.configs.recommended,
  {
    languageOptions: {
      globals: { ...globals.browser },
      parserOptions: {
        projectService: { allowDefaultProject: ["*.config.js"] },
        tsconfigRootDir: import.meta.dirname,
        extraFileExtensions: [".svelte"],
      },
    },
    rules: {
      // Errors from Tauri commands arrive as strings; numbers in messages are fine.
      "@typescript-eslint/restrict-template-expressions": ["error", { allowNumber: true }],
    },
  },
  {
    files: ["**/*.svelte", "**/*.svelte.ts"],
    languageOptions: { parserOptions: { parser: ts.parser, svelteConfig } },
  },
  {
    files: ["**/*.svelte"],
    rules: {
      // Typed linting cannot see other components' prop types inside .svelte
      // files and reports their callbacks as `any`; svelte-check checks them.
      "@typescript-eslint/no-unsafe-argument": "off",
      "@typescript-eslint/no-unsafe-assignment": "off",
      "@typescript-eslint/no-unsafe-call": "off",
      "@typescript-eslint/no-unsafe-member-access": "off",
      "@typescript-eslint/no-unsafe-return": "off",
    },
  },
  {
    // Room to read: blank lines around blocks and between a function's steps, sorted imports.
    plugins: { "@stylistic": stylistic, "simple-import-sort": simpleImportSort },
    rules: {
      "simple-import-sort/imports": "error",
      "simple-import-sort/exports": "error",
      "@stylistic/lines-between-class-members": [
        "error",
        "always",
        { exceptAfterSingleLine: true },
      ],
      "@stylistic/padding-line-between-statements": [
        "error",
        // After the imports and the directives.
        { blankLine: "always", prev: ["import", "directive"], next: "*" },
        { blankLine: "any", prev: "import", next: "import" },
        // Around every block: if, loops, switch, try, functions, classes, plain blocks.
        {
          blankLine: "always",
          prev: "*",
          next: ["block-like", "class", "function", "switch", "try"],
        },
        {
          blankLine: "always",
          prev: ["block-like", "class", "function", "switch", "try"],
          next: "*",
        },
        // After a group of declarations, and before the way out of a function.
        { blankLine: "always", prev: ["const", "let", "var"], next: "*" },
        { blankLine: "any", prev: ["const", "let", "var"], next: ["const", "let", "var"] },
        { blankLine: "always", prev: "*", next: ["return", "throw"] },
        // Not between a switch's case labels: an empty case stacked on the next stays tight.
        { blankLine: "any", prev: "*", next: ["case", "default"] },
        // Between exported declarations.
        { blankLine: "always", prev: "export", next: "*" },
        { blankLine: "always", prev: "*", next: "export" },
      ],
    },
  },
  { files: ["*.config.js"], extends: [ts.configs.disableTypeChecked] },
);
