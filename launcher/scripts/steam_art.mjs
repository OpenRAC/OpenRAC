#!/usr/bin/env node
// The pictures Steam's library shows for the launcher (core/src/steam.rs), in src-tauri/steam/:
// the site's backdrop cut to each size Steam asks for, with the logo on it, and the logo alone.
// Run from launcher/ after the picture or the logo changes. Needs Chromium (OPENRAC_E2E_CHROMIUM
// names one that playwright-core does not find itself).
import { mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import { chromium } from "playwright-core";

const OUT = "src-tauri/steam";
const BACKDROP = pathToFileURL("public/img/rac1-bg.webp").href;
const INK = "#e8f0ff"; // --text (src/app.css)
const SVG = readFileSync("src/assets/openrac-logo.svg");
const LOGO = `data:image/svg+xml;base64,${SVG.toString("base64")}`;

const logoBox = (width, height) => `
  <div style="width:${width}px;height:${height}px;background:${INK};
    -webkit-mask:url('${LOGO}') center/contain no-repeat;mask:url('${LOGO}') center/contain no-repeat;
    filter:drop-shadow(0 4px 18px rgb(0 0 0 / 0.7))"></div>`;

const page = (body, background) =>
  `<!doctype html><html><body style="margin:0;background:${background};overflow:hidden">${body}</body></html>`;

const backdrop = (focus, shade) => `
  <div style="position:fixed;inset:0;background:url('${BACKDROP}') ${focus}/cover no-repeat"></div>
  <div style="position:fixed;inset:0;background:${shade}"></div>`;

// A hero carries no logo of its own: Steam draws the logo picture over it.
const PICTURES = [
  {
    file: "cover.jpg",
    width: 600,
    height: 900,
    html: page(
      `${backdrop("center", "linear-gradient(transparent 45%, rgb(8 10 30 / 0.92))")}
       <div style="position:fixed;left:50px;right:50px;bottom:60px">${logoBox(500, 280)}</div>`,
      "#0b0e22",
    ),
  },
  {
    file: "wide.jpg",
    width: 920,
    height: 430,
    html: page(
      `${backdrop("center", "linear-gradient(90deg, rgb(8 10 30 / 0.85), transparent 70%)")}
       <div style="position:fixed;left:50px;top:95px">${logoBox(430, 240)}</div>`,
      "#0b0e22",
    ),
  },
  {
    file: "hero.jpg",
    width: 1920,
    height: 620,
    html: page(backdrop("center", "linear-gradient(transparent 60%, rgb(8 10 30 / 0.6))"), "#0b0e22"),
  },
  {
    file: "logo.png",
    width: 800,
    height: 447,
    transparent: true,
    // The logo's file has wide empty margins: Steam places the picture, so it is cut to the logo.
    clip: { x: 30, y: 95, width: 745, height: 205 },
    html: page(logoBox(800, 447), "transparent"),
  },
];

mkdirSync(OUT, { recursive: true });
const executablePath = process.env.OPENRAC_E2E_CHROMIUM;
const browser = await chromium.launch(executablePath ? { executablePath } : {});
try {
  for (const picture of PICTURES) {
    const tab = await browser.newPage({ viewport: { width: picture.width, height: picture.height } });
    // file:// so that the page may read the backdrop.
    const holder = join(OUT, ".page.html");
    writeFileSync(holder, picture.html);
    await tab.goto(pathToFileURL(holder).href, { waitUntil: "networkidle" });
    rmSync(holder);
    const path = join(OUT, picture.file);
    if (picture.transparent) await tab.screenshot({ path, type: "png", omitBackground: true, clip: picture.clip });
    else await tab.screenshot({ path, type: "jpeg", quality: 88 });
    await tab.close();
    console.log(path);
  }
} finally {
  await browser.close();
}
