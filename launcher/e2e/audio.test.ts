// The creator's recipe form sends one structured action input, then refreshes the installed packs.
import { chromium, type Browser, type Page } from "playwright-core";
import { createServer, type ViteDevServer } from "vite";
import { afterAll, beforeAll, expect, it } from "vitest";

let server: ViteDevServer | undefined;
let browser: Browser | undefined;
let page: Page;

beforeAll(async () => {
  server = await createServer({ configFile: "vite.config.ts", logLevel: "error", server: { port: 0 } });
  await server.listen();
  const executablePath = import.meta.env.OPENRAC_E2E_CHROMIUM as string | undefined;
  browser = await chromium.launch(executablePath ? { executablePath } : {});
  page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
  page.setDefaultTimeout(8000);
  await page.goto(`${server.resolvedUrls?.local[0]}?latest&page=mods`);
  await page.locator("#mods-tab-audio").click();
}, 60_000);

afterAll(async () => {
  await browser?.close();
  await server?.close();
});

it("validates adjustments and creates an enabled recipe in My mods", async () => {
  await page.getByLabel("What to export").selectOption("music");
  await page.getByLabel("Pack name", { exact: true }).fill("Fast music");
  expect((await page.locator(".recipe .text").boundingBox())?.width).toBeGreaterThan(400);
  const speed = page.getByLabel("Speed (%)", { exact: true });
  const create = page.getByRole("button", { name: "Create shareable recipe", exact: true });
  await speed.fill("0");
  expect(await create.isDisabled()).toBe(true);
  await speed.fill("104");
  await page.getByLabel("Volume (dB)", { exact: true }).fill("-3");
  expect(await create.isEnabled()).toBe(true);
  await create.click();
  await page
    .getByText("Recipe installed: share its ZIP. The sounds will be prepared when you play with mods.", { exact: true })
    .waitFor();
  const screenshot = import.meta.env.OPENRAC_AUDIO_SCREENSHOT as string | undefined;
  if (screenshot) await page.locator(".audio").screenshot({ path: screenshot });
  await page.locator("#mods-tab-mine").click();
  try {
    const enabled = page.getByRole("switch", { name: "Use Fast music", exact: true });
    await enabled.waitFor();
    expect(await enabled.isChecked()).toBe(true);
  } catch (error) {
    if (screenshot) await page.screenshot({ path: screenshot + ".packs.png" });
    throw error;
  }
});
