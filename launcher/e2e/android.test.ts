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
    // Measure the pages themselves, without Svelte entrance transitions replacing their nodes.
    await page.emulateMedia({ reducedMotion: "reduce" });
    const lefts: number[] = [];
    // Games is a shelf of discs across the whole window (DiscShelf), not a page with margins.
    for (const query of ["version=rac1/pal", "page=mods", "page=tasks", "page=settings"]) {
      await open(`version=rac1/pal&${query}`);
      await page.locator(".page").first().waitFor({ state: "visible" });
      lefts.push(
        await page
          .locator(".page")
          .first()
          .evaluate((el) => Math.round(el.getBoundingClientRect().left)),
      );
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

  it("offers to prepare missing levels beside the disc, and keeps Rebuild in its menu", async () => {
    await open("version=rac1/pal&page=library");
    await expect(page.locator(".acts .go").textContent()).resolves.toContain("Get levels and play");
    await open("built&version=rac1/pal&page=library");
    await expect(page.locator(".acts .go").textContent()).resolves.toMatch(/Play|Continue/);
    await page.locator(".acts").getByRole("button", { name: "More actions" }).click();
    const rebuild = page.getByRole("menuitem", { name: "Rebuild the game" });
    await rebuild.waitFor({ state: "visible" });
    await expect(rebuild.evaluate((e) => document.activeElement === e)).resolves.toBe(true);
  });

  it("turns the Mods switch beside the disc, as Play's own", async () => {
    await open("built&version=rac1/pal&page=library");
    const mods = page.locator(".with-mods").getByRole("switch");
    await mods.waitFor();
    const before = await mods.isChecked();
    await mods.click();
    await expect(mods.isChecked()).resolves.toBe(!before);
  });

  it("opens the game's logs from the menu beside the disc", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".acts").getByRole("button", { name: "More actions" }).click();
    await page.getByRole("menuitem", { name: "Logs and crash reports" }).click();
    await page.locator("#logs").waitFor({ state: "visible" });
  });

  it("keeps Play off the game's page, its band leading back to Games", async () => {
    await open("built&version=rac1/pal");
    await page.locator(".hero .back").waitFor();
    expect(await page.locator(".hero .big").count()).toBe(0);
    expect(await page.getByRole("button", { name: "More actions" }).count()).toBe(0);
    await page.locator(".hero .back").click();
    await page.locator(".slot.on").waitFor();
  });

  it("draws a running step's strip over the band's picture, not under its shade", async () => {
    await open("version=rac1/pal&page=library");
    await page.locator(".acts .go").click();
    await page.locator(".acts").getByRole("button", { name: "Game page" }).click();
    const strip = page.locator(".hero > .job");
    await strip.waitFor();
    expect(await strip.evaluate((e) => getComputedStyle(e).zIndex)).toBe("1");
  });

  it("shows no empty menu beside Set up", async () => {
    await open("fresh&version=rac1/pal&page=library");
    await page.locator(".acts .go").waitFor();
    expect(await page.locator(".acts").getByRole("button", { name: "More actions" }).count()).toBe(0);
  });

  it("spins the disc while a game starts", async () => {
    await open("launch&version=rac1/pal");
    await page.locator(".launch .disc").waitFor();
    await expect(page.locator(".launch h1").textContent()).resolves.toBe("Ratchet & Clank");
  });

  it("covers the launcher while the game runs, and steps aside when asked", async () => {
    await open("playing&built&version=rac1/pal&page=library");
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

  it("shows the disc and its buttons whole on a phone's low screen, and what changed", async () => {
    await page.setViewportSize({ width: 915, height: 412 });
    await open("built&crash&version=rac1/pal&page=library");
    await page.locator(".slot.on").waitFor();
    await page.waitForTimeout(900);
    const disc = await page.locator(".slot.on").boundingBox();
    const play = await page.locator(".acts .go").boundingBox();
    expect(disc && disc.y >= 0 && disc.y + disc.height <= 412).toBe(true);
    expect(play && play.y + play.height).toBeLessThanOrEqual(412 - 30);
    await page.getByRole("group", { name: "What's new" }).waitFor();
  });

  it("keeps the launcher gold on Games", async () => {
    await open("version=rac2/ntsc&page=library");
    await page.locator(".slot.on").waitFor();
    await page.waitForTimeout(400);
    // No game's accent on the page itself: the launcher's own gold.
    expect(await page.evaluate(() => document.documentElement.style.length)).toBe(0);
  });

  it("puts a theme on, with the launcher's gold and the game's light", async () => {
    // Game colours off: the launcher keeps its gold, the game only its light.
    await open("theme=oled&gold&version=rac3/ntsc");
    // The light eases from one game's to the next (app.css).
    await page.waitForTimeout(700);
    const [ink, amber, glow] = await page.evaluate(() => {
      const s = getComputedStyle(document.documentElement);
      return ["--ink", "--amber", "--glow"].map((name) => s.getPropertyValue(name).trim());
    });
    expect(ink).toBe("#000000");
    expect(amber).toBe("rgb(245, 192, 64)");
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

  /** The name beside the disc, once the last game's has gone (the two cross for a moment). */
  async function shownName() {
    await page.waitForFunction(() => document.querySelectorAll(".info").length === 1);
    return page.locator(".info h1").textContent();
  }

  /** What has the focus: its name as a screen reader says it, or its text. */
  const focused = () =>
    page.evaluate(() => {
      const el = document.activeElement;
      if (!el || el === document.body) return "";
      return (el.getAttribute("aria-label") ?? el.textContent).trim();
    });

  it("turns the wheel with the controller, and opens the game picked by Y or a tap", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".slot.on").waitFor();
    // The first press lands on a control; up and down on the wheel pick the disc it lands on.
    await page.locator(".slot.on").focus();
    await page.evaluate(() => (document.documentElement.dataset.input = "pad"));
    await press("down");
    expect(await shownName()).toContain("Going Commando");
    expect(await focused()).toMatch(/^Ratchet & Clank: Going Commando:/);
    await press("up");
    expect(await shownName()).not.toContain("Going Commando");
    // Y opens its page.
    await press("more");
    await page.locator(".hero .back").waitFor();
    await press("back");
    await page.waitForTimeout(500);
    // A tap on another disc picks it.
    await page
      .locator(".slot")
      .nth(1)
      .click({ position: { x: 150, y: 40 } });
    expect(await shownName()).toContain("Going Commando");
  });

  it("starts a game's next step from beside its disc by a tap", async () => {
    await open("");
    await page.locator(".acts .go").click();
    await page.getByRole("button", { name: /^Ratchet & Clank: Getting ready/ }).waitFor();
    expect(await page.locator(".slot").count()).toBeGreaterThan(0);
  });

  it("puts the disc into the console on Play, then starts the game", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".acts .go").click();
    await page.getByRole("dialog", { name: /^Starting Ratchet & Clank/ }).waitFor();
    await page.locator(".launch .led.on").waitFor({ timeout: 4000 });
    await page.getByRole("dialog", { name: /^Starting/ }).waitFor({ state: "detached", timeout: 6000 });
  });

  it("puts the disc back when B comes before the click", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".acts .go").click();
    await page.getByRole("dialog", { name: /^Starting/ }).waitFor();
    await press("back");
    await page.getByRole("dialog", { name: /^Starting/ }).waitFor({ state: "detached" });
    expect(await page.locator(".slot.on").count()).toBe(1);
  });

  it("opens the game's mods by X and its page by Y", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".slot.on").waitFor();
    await press("extra");
    await page.waitForFunction(() => !document.querySelector(".slot"));
    await press("back");
    await page.locator(".slot.on").waitFor();
    await press("more");
    await page.locator(".hero .back").waitFor();
  });

  it("does nothing on X while a dialog is open over the tab", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".slot.on").waitFor();
    await page.evaluate(() => {
      const d = document.createElement("div");
      d.setAttribute("role", "dialog");
      document.body.append(d);
    });
    await press("extra");
    expect(await page.locator(".slot").count()).toBeGreaterThan(0);
  });

  it("keeps the controller in the words beside the disc, and goes left to the disc and the tab open", async () => {
    await open("built&crash&version=rac1/pal&page=library");
    await page.locator(".slot.on").focus();
    await page.evaluate(() => (document.documentElement.dataset.input = "pad"));
    await press("right");
    expect(await focused()).toMatch(/Continue|Play/);
    // Down at the foot of the words stays there: it never turns the wheel to another game.
    await press("down", "down", "down");
    expect(await shownName()).toBe("Ratchet & Clank");
    expect(await page.evaluate(() => !!document.activeElement?.closest(".info"))).toBe(true);
    // Right at the end of the buttons stays too; up goes to what changed, down back to the buttons.
    await press("up", "right", "right", "right");
    expect(await page.evaluate(() => !!document.activeElement?.closest(".info"))).toBe(true);
    await press("up");
    expect(await page.evaluate(() => !!document.activeElement?.closest(".news"))).toBe(true);
    await press("down");
    expect(await page.evaluate(() => !!document.activeElement?.closest(".info .acts"))).toBe(true);
    // Left: the disc picked, then the Games tab, not the tab nearest to it.
    await press("left");
    expect(await focused()).toMatch(/^Ratchet & Clank:/);
    await press("left");
    expect(await focused()).toBe("Games");
  });

  it("keeps only the game the focus stops on, once", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".slot.on").focus();
    await page.evaluate(() => (document.documentElement.dataset.input = "pad"));
    for (const b of ["down", "down", "down", "up"]) {
      await page.evaluate((name) => {
        const pad = (window as unknown as { __openracPad: (e: string) => void }).__openracPad;
        pad(`down:${name}`);
        pad(`up:${name}`);
      }, b);
      await page.waitForTimeout(60);
    }
    await page.waitForTimeout(1200);
    const writes = await page.evaluate(
      () =>
        (window as unknown as { __openracConfigWrites?: { count: number; last: { lastVersion: string } } })
          .__openracConfigWrites,
    );
    const shown = await shownName();
    expect(writes?.count).toBe(1);
    expect(writes?.last.lastVersion.split("/")[0]).toBe("rac3");
    expect(shown).toContain("Up Your Arsenal");
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
    expect(await focused()).toBe("OLED black");
    await press("accept");
    await expect(page.getByRole("radio", { name: "OLED black" }).getAttribute("aria-checked")).resolves.toBe("true");
  });

  it("moves through a menu's items", async () => {
    await open("built&version=rac1/pal&page=library");
    await page.locator(".acts").getByRole("button", { name: "More actions" }).focus();
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
    // The page's slide in ends first: the rows are where they show.
    await page.locator(".plugins ul:not(.built-in) li").first().waitFor();
    await page.waitForTimeout(1500);
    const before = await names();
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
