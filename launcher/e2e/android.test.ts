// The Android launcher's pages, as the browser preview draws them (?android, src/lib/mock.ts), at
// the Ayn Odin 3's screen shape: each page opens without an error, and what Android has, or must
// not have, is there. A few seconds in all: one Vite server and one browser for every test.
// `yarn test:android`, with Playwright's headless Chromium (`yarn playwright-core install
// chromium-headless-shell`, once) or the browser OPENRAC_E2E_CHROMIUM names.
import { chromium, type Browser, type Page } from "playwright-core";
import { createServer, type ViteDevServer } from "vite";
import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it } from "vitest";

let server: ViteDevServer;
let browser: Browser;
let page: Page;
let errors: string[] = [];
let base = "";

beforeAll(async () => {
  server = await createServer({
    configFile: "vite.config.ts",
    logLevel: "error",
    server: { port: 0, strictPort: false },
  });
  await server.listen();
  base = server.resolvedUrls?.local[0] ?? "";
  // Vitest gives the environment's variables in import.meta.env.
  const executablePath = import.meta.env.OPENRAC_E2E_CHROMIUM as string | undefined;
  browser = await chromium.launch(executablePath ? { executablePath } : {});
}, 60_000);

afterAll(async () => {
  await browser.close();
  await server.close();
});

beforeEach(async () => {
  errors = [];
  page = await browser.newPage({ viewport: { width: 960, height: 540 } });
  page.on("pageerror", (e) => errors.push(e.message));
  page.on("console", (m) => {
    if (m.type() === "error") errors.push(m.text());
  });
});

afterEach(async () => {
  await page.close();
  // A page that threw is a regression whatever it shows.
  expect(errors).toEqual([]);
});

/** Opens the preview's Android layout with QUERY. No real look for a launcher update unless asked
 *  (?update): ?latest says there is none. */
async function open(query: string) {
  const q = new URLSearchParams(query);
  q.set("android", "");
  if (!q.has("update")) q.set("latest", "");
  await page.goto(`${base}?${q.toString()}`);
  await page.locator("nav").first().waitFor();
}

describe("Android launcher", () => {
  it("has the rail, and no desktop header", async () => {
    await open("");
    expect(await page.evaluate(() => document.documentElement.dataset.platform)).toBe("android");
    for (const name of ["Games", "Mods", "Settings"]) {
      await expect(page.getByRole("button", { name, exact: true }).count()).resolves.toBeGreaterThan(0);
    }
    expect(await page.locator(".discord").count()).toBe(0);
  });

  it("opens every page at one width", async () => {
    const lefts: number[] = [];
    for (const query of ["page=library", "version=rac1/pal", "page=mods", "page=tasks", "page=settings"]) {
      await open(`version=rac1/pal&${query}`);
      const box = await page.locator(".page").first().boundingBox();
      expect(box, query).not.toBeNull();
      lefts.push(Math.round(box?.x ?? -1));
    }
    expect(new Set(lefts).size).toBe(1);
  });

  it("shows Develop for the picked game, with its versions and only what Android runs", async () => {
    await open("developer&page=develop&version=rac1/pal");
    await expect(page.locator(".game .name").textContent()).resolves.toBe("Ratchet & Clank");
    const tabs = await page.getByRole("tab").allTextContents();
    expect(tabs.some((t) => t.startsWith("PAL"))).toBe(true);
    expect(tabs.some((t) => t.startsWith("NTSC"))).toBe(true);
    expect(tabs.some((t) => t.includes("Console"))).toBe(true);
    const text = (await page.locator("main").textContent()) ?? "";
    expect(text).not.toContain("This checkout");
    expect(text).not.toContain("more for other platforms");
    expect(text).not.toContain("Build the PS2 program");
    await page.getByRole("tab", { name: /NTSC/ }).click();
    await expect(page.getByRole("tab", { name: /NTSC/ }).getAttribute("aria-selected")).resolves.toBe("true");
  });

  it("shows Mods for the picked game, without the level editor", async () => {
    await open("page=mods&version=rac1/pal");
    await expect(page.locator(".game .name").textContent()).resolves.toBe("Ratchet & Clank");
    expect(await page.getByRole("tab", { name: /Level editor/ }).count()).toBe(0);
  });

  it("brings the game page in whole, its band not popping in after it", async () => {
    await open("version=rac1/pal");
    const hero = page.locator(".hero");
    await hero.waitFor();
    // At once, not after a fly-in of its own: the page's slide carries it.
    expect(await hero.evaluate((e) => getComputedStyle(e).opacity)).toBe("1");
    expect(await page.locator(".hero-bg").evaluate((e) => getComputedStyle(e).animationName)).toBe("none");
  });

  it("offers to prepare missing levels and keeps Rebuild in the ready game's menu", async () => {
    await open("version=rac1/pal");
    await expect(page.locator(".go .big").textContent()).resolves.toContain("Get levels and play");
    await open("built&version=rac1/pal");
    await expect(page.locator(".go .big").textContent()).resolves.toContain("Play");
    const more = page.locator(".go-row").getByRole("button", { name: "More actions" });
    await more.scrollIntoViewIfNeeded();
    // The click may scroll its target into view. Measure when the menu is opened,
    // after that legitimate movement and before its focus effect runs.
    await more.evaluate((button) => {
      button.addEventListener(
        "click",
        () => {
          button.setAttribute("data-scroll-at-click", String(document.querySelector("main")?.scrollTop));
        },
        { capture: true, once: true },
      );
    });
    await more.click();
    const scrollTop = Number(await more.getAttribute("data-scroll-at-click"));
    const rebuild = page.getByRole("menuitem", { name: "Rebuild the game" });
    await rebuild.waitFor({ state: "visible" });
    // Allow focus-triggered scrolling to arrive before checking that the menu stays open.
    await page.evaluate(
      () =>
        new Promise<void>((resolve) =>
          requestAnimationFrame(() =>
            requestAnimationFrame(() => {
              resolve();
            }),
          ),
        ),
    );
    await expect(rebuild.count()).resolves.toBe(1);
    await expect(page.locator("main").evaluate((e) => e.scrollTop)).resolves.toBe(scrollTop);
    await expect(rebuild.evaluate((e) => document.activeElement === e)).resolves.toBe(true);
    await page.locator("main").evaluate((e) => {
      e.scrollTop = e.scrollTop > 0 ? 0 : e.scrollHeight;
    });
    await rebuild.waitFor({ state: "detached" });
  });

  it("draws a running step's strip over the band's picture, not under its shade", async () => {
    await open("version=rac1/pal");
    await page.locator(".go .big").click();
    const strip = page.locator(".hero > .job");
    await strip.waitFor();
    expect(await strip.evaluate((e) => getComputedStyle(e).zIndex)).toBe("1");
  });

  it("shows no empty menu beside Set up, and keeps the main button in its place after it", async () => {
    const centre = async () => {
      const box = await page.locator(".go .big").boundingBox();
      return Math.round((box?.x ?? 0) + (box?.width ?? 0) / 2);
    };
    await open("fresh&version=rac1/pal");
    await page.locator(".go .big").waitFor();
    expect(await page.locator(".go-row").getByRole("button", { name: "More actions" }).count()).toBe(0);
    const setUp = await centre();
    await open("built&version=rac1/pal");
    await page.locator(".go .big").waitFor();
    expect(Math.abs((await centre()) - setUp)).toBeLessThanOrEqual(1);
  });

  it("spins the disc while a game starts", async () => {
    await open("launch&version=rac1/pal");
    await page.locator(".launch .disc").waitFor();
    await expect(page.locator(".launch h1").textContent()).resolves.toBe("Ratchet & Clank");
  });

  it("covers the launcher while the game runs, and steps aside when asked", async () => {
    await open("playing&built&version=rac1/pal");
    const running = page.getByRole("dialog", { name: "Ratchet & Clank" });
    await running.waitFor();
    await expect(running.getByRole("button", { name: /Return to game/ }).count()).resolves.toBe(1);
    await expect(running.getByRole("button", { name: /Quit game/ }).count()).resolves.toBe(1);
    await running.getByRole("button", { name: /Use the launcher meanwhile/ }).click();
    await running.waitFor({ state: "detached" });
    await expect(page.getByRole("button", { name: /Return to game/ }).count()).resolves.toBe(1);
  });

  it("offers a new launcher by itself, once until put off", async () => {
    await open("update");
    const offer = page.getByRole("dialog", { name: "A new launcher is ready" });
    await offer.waitFor();
    await offer.getByRole("button", { name: "Later" }).click();
    await offer.waitFor({ state: "detached" });
    await open("update");
    await page.waitForTimeout(500);
    expect(await offer.count()).toBe(0);
  });

  it("puts a theme on, with the launcher's gold and the game's light", async () => {
    await open("theme=oled&version=rac3/ntsc");
    // The light eases from one game's to the next (app.css).
    await page.waitForTimeout(700);
    const [ink, amber, glow] = await page.evaluate(() => {
      const s = getComputedStyle(document.documentElement);
      return ["--ink", "--amber", "--glow"].map((name) => s.getPropertyValue(name).trim());
    });
    expect(ink).toBe("#000000");
    expect(amber).toBe("#f5c040");
    expect(glow).toBe("rgb(255, 210, 63)");
  });
});

describe("A controller", () => {
  /** Presses BUTTONS on the controller as Android's activity hands them over (pad.ts), letting the
   *  page settle after each. */
  async function press(...buttons: string[]) {
    for (const b of buttons) {
      await page.evaluate((name) => {
        const pad = (window as unknown as { __openracPad: (e: string) => void }).__openracPad;
        pad(`down:${name}`);
        pad(`up:${name}`);
      }, b);
      await page.waitForTimeout(300);
    }
  }

  /** What has the focus: its name as a screen reader says it, or its text. */
  const focused = () =>
    page.evaluate(() => {
      const el = document.activeElement;
      if (!el || el === document.body) return "";
      return (el.getAttribute("aria-label") ?? el.textContent).trim();
    });

  it("opens a game's page from its tile by A or a tap, and leaves the tile's button to a finger", async () => {
    await open("built");
    // The tile's own Play is there for a finger, but the controller never lands on it.
    const play = page.locator(".tile .act").first();
    await expect(play.textContent()).resolves.toContain("Play");
    await press("down");
    expect(await focused()).toMatch(/^Ratchet & Clank:/);
    // Right goes to the next game, not to the tile's own button.
    await press("right");
    expect(await focused()).toMatch(/^Ratchet & Clank: Going Commando:/);
    await press("left", "accept");
    await page.waitForTimeout(500);
    // A opens the game's page, with the focus on its first control, and starts nothing.
    expect(await page.evaluate(() => document.activeElement?.closest(".page") !== null)).toBe(true);
    expect(await page.locator(".tile").count()).toBe(0);
    await expect(page.locator(".go .big").textContent()).resolves.toContain("Play");
    await press("back");
    await page.waitForTimeout(500);
    expect(await focused()).toMatch(/^Ratchet & Clank:/);
    // A tap on the tile opens the page too.
    await page.locator(".tile .open").first().click();
    await page.locator(".go .big").waitFor();
    expect(await page.locator(".tile").count()).toBe(0);
  });

  it("starts a game's next step from its tile's button by a tap", async () => {
    await open("");
    await page.locator(".tile .act").first().click();
    await page.getByRole("button", { name: /^Ratchet & Clank: Getting ready/ }).waitFor();
    expect(await page.locator(".tile").count()).toBeGreaterThan(0);
  });

  it("rings the game picked last, and only the focus once the controller moves", async () => {
    await open("version=rac1/pal&page=library");
    const tile = page.locator(".tile").first();
    await tile.waitFor();
    await page.waitForTimeout(600);
    // The picked game is ringed, and its picture does not zoom under the pointer.
    const ring = () => tile.evaluate((e) => getComputedStyle(e, "::after").opacity);
    expect(await ring()).toBe("1");
    await tile.hover();
    await page.waitForTimeout(600);
    expect(
      await page
        .locator(".tile .art")
        .first()
        .evaluate((e) => getComputedStyle(e).transform),
    ).toBe("none");
    // With the controller on the next game, only that one is ringed.
    await press("down", "right");
    await page.waitForTimeout(400);
    expect(await ring()).toBe("0");
    const outlines = await page
      .locator(".tile")
      .evaluateAll((tiles) => tiles.map((t) => getComputedStyle(t).outlineStyle !== "none"));
    expect(outlines.filter(Boolean).length).toBe(1);
    expect(outlines[0]).toBe(false);
  });

  it("goes into the rail only sideways", async () => {
    await open("page=settings");
    for (let i = 0; i < 12; i++) {
      await press("up");
      expect(await page.evaluate(() => document.activeElement?.closest("header") === null)).toBe(true);
    }
    await press("left");
    expect(await page.evaluate(() => document.activeElement?.closest("header") !== null)).toBe(true);
  });

  it("reaches every theme, not only the one on", async () => {
    await open("page=settings");
    await page.getByRole("radio", { name: "OpenRAC" }).focus();
    await press("right");
    expect(await focused()).toBe("Gruvbox");
    await press("accept");
    await expect(page.getByRole("radio", { name: "Gruvbox" }).getAttribute("aria-checked")).resolves.toBe("true");
  });

  it("moves through a menu's items", async () => {
    await open("version=rac1/pal");
    await page.getByRole("button", { name: "More actions" }).focus();
    await press("accept");
    const first = await focused();
    await press("down");
    expect(await focused()).not.toBe(first);
    expect(await page.evaluate(() => document.activeElement?.getAttribute("role"))).toBe("menuitem");
  });

  it("keeps the texture pack on Mods as a pack, not on the game's page, and reaches it by pad", async () => {
    await open("version=rac1/pal");
    expect(await page.getByText("Your texture pack").count()).toBe(0);
    await open("page=mods&version=rac1/pal");
    const own = page.getByRole("switch", { name: "Use Your texture pack" });
    await own.waitFor();
    // Its steps are folded under the packs until Make your own opens them.
    const dumping = page.getByRole("switch", { name: "Save the game's textures while you play" });
    expect(await dumping.isVisible()).toBe(false);
    await own.focus();
    await press("right");
    expect(await focused()).toBe("Make your own");
    await press("accept");
    await dumping.waitFor();
    expect(await dumping.isVisible()).toBe(true);
    // Its arrow says it opens, and turns when it is open.
    const arrow = page.locator("#texture-make summary .chevron");
    expect(await arrow.isVisible()).toBe(true);
    expect(await arrow.evaluate((el) => getComputedStyle(el).transform)).not.toBe("none");
  });

  it("says where mods come from at the top, and lists none itself", async () => {
    const said: string[] = [];
    page.on("console", (m) => {
      if (m.text().includes("[preview]"))
        void Promise.all(m.args().map((x) => x.jsonValue())).then((a: unknown[]) => said.push(JSON.stringify(a)));
    });
    await open("page=mods&version=rac1/pal");
    const banner = page.getByRole("region", { name: "Get mods" });
    await banner.waitFor();
    // First on the page, above the lists.
    const top = await banner.boundingBox();
    const list = await page.getByRole("switch", { name: "Use Your texture pack" }).boundingBox();
    expect(top && list && top.y < list.y).toBe(true);
    expect(await page.getByPlaceholder("Search mods").count()).toBe(0);
    // A click anywhere on it opens the hub, not only on its button.
    await banner.click({ position: { x: 30, y: 20 } });
    await expect.poll(() => said.join(" ")).toContain('"https://openrac.dev/mods"');
    // Publish does its own, not the hub.
    said.length = 0;
    await banner.getByRole("button", { name: "Publish your mod" }).click();
    await expect.poll(() => said.join(" ")).toContain("https://openrac.dev/mods/submit");
    expect(said.join(" ")).not.toContain('"https://openrac.dev/mods"');
    expect(said.join(" ")).not.toContain("index_fetch");
  });

  it("finds a mod and shows one kind when there are many", async () => {
    await open("page=mods&version=rac1/pal&manymods");
    const search = page.getByRole("searchbox", { name: "Find a mod" });
    await search.fill("bolt magnet");
    await expect(page.getByRole("switch", { name: /^Use Bolt magnet/ }).count()).resolves.toBe(1);
    await expect(page.getByRole("switch", { name: /^Use Lock-strafe/ }).count()).resolves.toBe(0);
    await search.fill("");
    await page.getByRole("radio", { name: /^Tools/ }).click();
    await expect(page.getByRole("button", { name: "Open Nanotech Upscaler" }).count()).resolves.toBe(1);
    // A tool can't be off: its switch is on and greyed, and says why.
    const always = page.getByRole("switch", { name: /^Nanotech Upscaler: Always on/ });
    expect(await always.isChecked()).toBe(true);
    expect(await always.isDisabled()).toBe(true);
    await expect(page.getByRole("switch", { name: /^Use Bolt magnet/ }).count()).resolves.toBe(0);
    await expect(page.getByRole("switch", { name: "Use Your texture pack" }).count()).resolves.toBe(0);
  });

  it("says once when a search finds nothing, and keeps the search while it hides mods", async () => {
    await open("page=mods&version=rac1/pal&manymods");
    const search = page.getByRole("searchbox", { name: "Find a mod" });
    await search.fill("no such mod");
    await page.getByText("Nothing matches “no such mod”.").waitFor();
    expect(await page.getByText(/^Nothing matches/).count()).toBe(1);
    expect(await page.getByRole("heading", { name: "Gameplay mods" }).count()).toBe(0);
    await page.getByRole("button", { name: "Clear" }).click();
    await page.getByRole("heading", { name: "Gameplay mods" }).waitFor();
  });

  it("moves a mod by dragging its grip, and not while sorted by name", async () => {
    await open("page=mods&version=rac1/pal&manymods");
    const names = () => page.locator(".plugins ul:not(.built-in) .name").allTextContents();
    const before = await names();
    // The page's slide in ends first: the rows are where they show.
    await page.locator(".plugins ul:not(.built-in) li").first().waitFor();
    await page.waitForTimeout(1500);
    await page
      .locator(".plugins ul:not(.built-in) li")
      .first()
      .evaluate((li) => {
        li.scrollIntoView({ block: "start" });
      });
    await page.waitForTimeout(300);
    const grip = page.locator(".plugins ul:not(.built-in) li").nth(2).locator("[data-grip]");
    const g = await grip.boundingBox();
    const top = await page.locator(".plugins ul:not(.built-in) li").first().boundingBox();
    if (!g || !top) throw new Error("no grip");
    await page.mouse.move(g.x + g.width / 2, g.y + g.height / 2);
    await page.mouse.down();
    await page.mouse.move(g.x + g.width / 2, top.y + 4, { steps: 10 });
    await page.waitForTimeout(100);
    await page.mouse.up();
    await expect.poll(names).toEqual([before[2], before[0], before[1], ...before.slice(3)]);
    await page.getByRole("radio", { name: "A to Z" }).click();
    expect(await page.locator(".plugins [data-grip]").count()).toBe(0);
  });

  it("says Play starts without mods above the lists, once", async () => {
    await open("page=mods&version=rac1/pal");
    await page.getByRole("switch", { name: "Use Your texture pack" }).waitFor();
    expect(await page.getByText("Play starts the game without mods.", { exact: false }).count()).toBe(1);
  });

  it("passes over what a closed fold holds", async () => {
    await open("page=settings");
    const advanced = page.locator("details:not([open]) > summary").first();
    if ((await advanced.count()) === 0) return;
    await advanced.focus();
    await press("up");
    // Moved on to a control that shows, rather than staying put on a hidden one.
    expect(await page.evaluate(() => document.activeElement?.tagName)).not.toBe("SUMMARY");
  });
});
