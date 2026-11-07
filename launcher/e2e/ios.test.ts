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
    window.webkit = { messageHandlers: { openrac: { postMessage(request) {
      import("/src/lib/mock.ts").then(async (mock) => {
        try {
          let value = await mock.mockCall(request.command, request.args);
          if (request.command === "app_info") value = { ...value, platform: "ios", standalone: true,
            capabilities: { mods: false, saveEditing: false, saveTransfer: false, openFolders: false, folderScan: false, updates: false } };
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
