// Native iOS controller events, with the real shared pages and a mock command bridge.
import { chromium, type Browser, type Page } from "playwright-core";
import { createServer, type ViteDevServer } from "vite";
import { afterAll, afterEach, beforeAll, beforeEach, expect, it } from "vitest";

let server: ViteDevServer;
let browser: Browser;
let page: Page;
let base = "";
let errors: string[];

beforeAll(async () => {
  server = await createServer({ configFile: "vite.config.ts", logLevel: "error", server: { port: 0 } });
  await server.listen();
  base = server.resolvedUrls?.local[0] ?? "";
  const executablePath = import.meta.env.OPENRAC_E2E_CHROMIUM as string | undefined;
  browser = await chromium.launch(executablePath ? { executablePath } : {});
}, 60_000);

afterAll(async () => {
  await browser.close();
  await server.close();
});

beforeEach(async () => {
  errors = [];
  page = await browser.newPage({ viewport: { width: 1024, height: 768 } });
  page.on("pageerror", (error) => errors.push(error.message));
  await page.addInitScript({
    content: `
    window.__OPENRAC_IOS__ = true;
    navigator.getGamepads = () => {
      throw new Error("The iOS launcher must not poll the browser Gamepad API");
    };
    window.__nativeSaveRequests = [];
    window.webkit = { messageHandlers: { openrac: { postMessage(request) {
      if (["export_saves", "pick_saves_file", "preview_save_import", "import_saves"].includes(request.command))
        window.__nativeSaveRequests.push({ command: request.command, args: request.args });
      import("/src/lib/mock.ts").then(async (mock) => {
        try {
          let value = request.command === "pick_saves_file" ? "/private/imports/saves.zip" : await mock.mockCall(request.command, request.args);
          if (request.command === "app_info") value = { ...value, platform: "ios", standalone: true,
            capabilities: { mods: false, saveEditing: false, saveTransfer: true, openFolders: false, folderScan: false, updates: false } };
          if (["library", "sync_progress_from_web"].includes(request.command)) for (const game of value.games) {
            if (game.id !== "rac1") for (const version of game.versions)
              for (const action of version.actions) action.thisPlatform = false;
          }
          if (request.command === "get_config") value = { ...value, checkUpdates: false, checkModUpdates: false, autoUpdateMods: false };
          window.__openracReply(request.id, true, value);
        } catch (error) {
          window.__openracReply(request.id, false, String(error));
        }
      });
    } } } };
  `,
  });
  await page.goto(`${base}?built&latest`);
  await page.locator(".list .entry").first().waitFor();
});

afterEach(async () => {
  await page.close();
  expect(errors).toEqual([]);
});

async function press(...buttons: string[]) {
  for (const button of buttons) {
    await page.evaluate((name) => {
      const native = window as unknown as { __openracPad: (event: string) => void };
      native.__openracPad(`down:${name}`);
      native.__openracPad(`up:${name}`);
    }, button);
    await page.waitForTimeout(300);
  }
}

it("selects games and changes tabs through native controller events", async () => {
  await press("down");
  expect(await page.locator(":focus").getAttribute("data-key")).toBe("rac1");
  await press("down", "accept");
  expect(await page.locator(".list .entry[aria-current=page]").getAttribute("data-key")).toBe("rac2");
  await press("next");
  expect(await page.locator(".tab[aria-current=page]").getAttribute("data-page")).toBe("settings");
  await press("back");
  expect(await page.locator(".tab[aria-current=page]").getAttribute("data-page")).toBe("library");
});

it("cycles select fields without opening a native picker", async () => {
  await page.evaluate(() => {
    document.body.innerHTML = '<select id="choice"><option>One</option><option>Two</option></select>';
    const select = document.querySelector("select");
    if (!select) throw new Error("Missing fixture");
    select.showPicker = () => {
      throw new Error("iOS cannot navigate a system select picker");
    };
    select.focus();
  });
  await press("accept");
  expect(await page.locator("#choice").inputValue()).toBe("Two");
  await press("left");
  expect(await page.locator("#choice").inputValue()).toBe("One");
});

it("keeps a visible, actionable target when WebKit declines DOM focus", async () => {
  await press("down");
  await page.evaluate(() => {
    HTMLElement.prototype.focus = () => undefined;
  });
  const target = page.locator("[data-pad-focus]");
  expect(await target.getAttribute("data-key")).toBe("rac1");
  expect(await target.evaluate((el) => getComputedStyle(el).outlineColor)).toBe("rgb(255, 255, 255)");
  expect(await target.evaluate((el) => getComputedStyle(el).outlineWidth)).toBe("4px");
  await press("down");
  expect(await target.getAttribute("data-key")).toBe("rac2");
  await press("accept");
  expect(await page.locator(".list .entry[aria-current=page]").getAttribute("data-key")).toBe("rac2");
  await page.locator(".tab[data-page=settings]").click();
  expect(await page.locator("[data-pad-focus]").count()).toBe(0);
});

it("exports a chosen slot through the native share command", async () => {
  await page.getByRole("button", { name: "Export", exact: true }).click();
  const dialog = page.getByRole("dialog", { name: "Export saves" });
  await dialog.getByRole("radio", { name: /Slot 2/ }).check();
  await dialog.getByRole("button", { name: "Share…", exact: true }).click();
  await dialog.waitFor({ state: "hidden" });
  const requests = await page.evaluate(
    () =>
      (
        window as unknown as {
          __nativeSaveRequests: { command: string; args: { slot?: number } }[];
        }
      ).__nativeSaveRequests,
  );
  expect(requests).toContainEqual({ command: "export_saves", args: { serial: "SCES_509.16", slot: 1 } });
});

it("opens Files and previews a save before an explicit import confirmation", async () => {
  await page.getByRole("button", { name: "Import", exact: true }).click();
  const dialog = page.getByRole("dialog", { name: "Import saves" });
  await dialog.getByText(/Replaces Kerwan/).waitFor();
  const commands = () =>
    page.evaluate(() =>
      (
        window as unknown as {
          __nativeSaveRequests: { command: string }[];
        }
      ).__nativeSaveRequests.map((r) => r.command),
    );
  expect(await commands()).toEqual(["pick_saves_file", "preview_save_import"]);
  await dialog.getByRole("button", { name: "Import", exact: true }).click();
  await dialog.waitFor({ state: "hidden" });
  expect(await commands()).toEqual(["pick_saves_file", "preview_save_import", "import_saves"]);
});

it("keeps future games visible without offering an unsupported iOS disc import", async () => {
  await page.locator('.list .entry[data-key="rac2"]').click();
  const action = page.locator(".go .big");
  await action.waitFor();
  expect(await action.textContent()).toContain("Not available on iOS");
  expect(await action.isDisabled()).toBe(true);
  expect(await page.getByRole("button", { name: "Set up from your disc", exact: true }).count()).toBe(0);
  expect(await page.locator(".list .entry").count()).toBe(6);
});

it("renders the Store catalogue without requesting desktop game pictures", async () => {
  await page.addInitScript("window.__OPENRAC_STORE_ARTWORK__ = true");
  const pictures: string[] = [];
  page.on("request", (request) => {
    if (request.url().includes("/img/")) pictures.push(request.url());
  });
  await page.reload();
  await page.locator('.list .entry img[src="/img/store/rac1.svg"]').waitFor();
  expect(await page.locator('.list .entry img[src^="/img/store/"]').count()).toBe(6);
  expect(pictures.some((url) => url.endsWith(".webp"))).toBe(false);
});
