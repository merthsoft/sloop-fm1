// SPDX-License-Identifier: GPL-3.0-only
// SLOOP 2.6: the site without internet (web/sw.js, made by web/make_site.py), in a real Chromium (Playwright):
// opened once with the server up, the installer (and its firmware, checked against its SHA-256) and the editor
// load again with the server stopped; online again, a changed page is served as published, never the old copy;
// the manifest is one Chrome can install. Run from the repo root after a site is made:
//   node tests/pwa_test.mjs SITE_DIR      (exit 77 when Playwright is missing)
import { createServer } from "node:http";
import { readFileSync, writeFileSync, mkdtempSync } from "node:fs";
import { createRequire } from "node:module";
import { join, resolve, sep, delimiter, extname, dirname } from "node:path";
import { pathToFileURL } from "node:url";

let chromium;
try { ({ chromium } = await import("playwright")); }
catch {
  try {                                              // (a global install: NODE_PATH, as require finds it)
    const req = createRequire(join(process.cwd(), "x.js"));
    ({ chromium } = req(req.resolve("playwright", { paths: (process.env.NODE_PATH || "").split(delimiter).filter(Boolean) })));
  } catch { console.log("pwa: SKIP (Playwright not installed)"); process.exit(77); }
}

const site = process.argv[2];
if (!site) { console.log("usage: node tests/pwa_test.mjs SITE_DIR"); process.exit(2); }
const PORT = 8790 + Math.floor(Math.random() * 100);
const base = `http://127.0.0.1:${PORT}/webapp/`;
const installerSource = readFileSync(join(site, 'webapp/installer/index.html'), 'utf8');
const meta = JSON.parse(installerSource.match(/const meta = (\{[^\n]+\});/)[1]);
const status = `MerthSloop ${meta.version}`;
const editorTitle = readFileSync(join(site, 'webapp/editor/index.html'), 'utf8').match(/<title>(.*?)<\/title>/)[1];
let failed = 0;
const ok = (c, what) => { console.log(`pwa: ${what.padEnd(70)} ${c ? "ok" : "FAIL"}`); if (!c) failed++; };
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let server = null;
let cacheFirmware = false;
const originals = new Map();
function change(path, data) {
  if (!originals.has(path)) originals.set(path, readFileSync(path));
  writeFileSync(path, data);
}
async function serve() {
  const root = resolve(site);
  server = createServer((req, res) => {
    const name = new URL(req.url, base).pathname;
    const path = resolve(root, '.' + name + (name.endsWith('/') ? 'index.html' : ''));
    if (!path.startsWith(root + sep)) { res.writeHead(403).end(); return; }
    try {
      const data = readFileSync(path);
      res.setHeader('Content-Type', ({'.html':'text/html; charset=utf-8','.js':'text/javascript',
        '.wasm':'application/wasm','.webmanifest':'application/manifest+json','.png':'image/png','.ttf':'font/ttf'})[extname(path)] || 'application/octet-stream');
      if (cacheFirmware && path.endsWith('.fwsc')) res.setHeader('Cache-Control', 'max-age=3600');
      res.end(data);
    } catch { res.writeHead(404).end(); }
  });
  await new Promise((r, reject) => { server.once('error', reject); server.listen(PORT, '127.0.0.1', r); });
}
async function stop() { if (server) { const s=server; server=null; await new Promise(r=>{s.close(r);s.closeAllConnections();}); } }

const launch = { headless: true };
if (process.env.PWA_BROWSER) launch.executablePath = process.env.PWA_BROWSER;
const profile = mkdtempSync(join(process.env.PWA_PROFILE_ROOT || dirname(resolve(site)), 'pwa-profile-'));
// Installability cannot be checked in the incognito contexts from newContext().
const ctx = await chromium.launchPersistentContext(profile, launch);
const browser = ctx.browser();
try {
  await serve();
  let page = await ctx.newPage();
  await page.addInitScript(() => {
    window.__pwaOffered = false;
    window.addEventListener('beforeinstallprompt', () => { window.__pwaOffered = true; });
  });
  const perr = [];
  page.on("pageerror", (e) => perr.push(e.message));
  await page.goto(base + "editor/");
  await page.waitForTimeout(500);
  ok(!perr.length, "the editor loads without a script error" + (perr.length ? ": " + perr.join("; ") : ""));
  ok(await page.evaluate(() => { const b = document.getElementById("install"); return !!b && b.hidden === !window.__pwaOffered; }),
     "the editor: Install the app is visible only after Chrome offers to install");
  await page.goto(base + "installer/");
  await page.waitForTimeout(300);
  ok(!perr.length, "the installer loads without a script error" + (perr.length ? ": " + perr.join("; ") : ""));
  const ready = await page.evaluate(async () => {
    const reg = await navigator.serviceWorker.ready;
    return !!reg.active;
  });
  ok(ready, "online: the installer registers sw.js, it is active");
  await page.reload();                                   // (now controlled)
  ok(await page.evaluate(() => !!navigator.serviceWorker.controller), "the installer page is controlled by it");
  const cached = await page.evaluate(async () => {
    const ks = await caches.keys();
    const c = await caches.open(ks.find((k) => k.startsWith("sloop-site-")));
    return (await c.keys()).map((r) => new URL(r.url).pathname);
  });
  ok(cached.includes("/webapp/editor/") && cached.includes("/webapp/installer/") &&
     cached.includes(new URL(meta.pkg, base + 'installer/').pathname) && cached.includes("/webapp/editor/fukiai.ttf"),
     "kept: the installer, the editor, its font, the firmware");

  const cdp = await ctx.newCDPSession(page);
  const errs = (await cdp.send("Page.getInstallabilityErrors")).installabilityErrors || [];
  ok(errs.length === 0, "Chrome can install it (no installability error)" + (errs.length ? ": " + errs.map((e) => e.errorId).join(", ") : ""));

  await stop();                                          // ---- no network at all
  await page.goto(base + "installer/");
  await page.waitForFunction(expected => document.getElementById("status").textContent === expected, status, { timeout: 8000 })
    .catch(() => {});
  const st = await page.textContent("#status");
  ok(st === status, `offline: the installer loads its firmware from the copy (${st.trim().slice(0, 40)})`);
  await page.goto(base + "editor/");
  const ed = await page.evaluate(async () => {
    await document.fonts.ready;
    return { title: document.title, connect: !!document.getElementById("connect"), font: document.fonts.check('16px "Fukiai"', "") };
  });
  ok(ed.title === editorTitle && ed.connect, "offline: the editor loads");
  ok(await page.evaluate(async () => {
    const [js, worklet, wasm] = await Promise.all(['preview.js','preview-worklet.js','sloop-preview.wasm'].map(f => fetch(f)));
    const module = await WebAssembly.compile(await wasm.arrayBuffer());
    return js.ok && worklet.ok && WebAssembly.Module.exports(module).some(e => e.name === 'preview_init');
  }), 'offline: preview modules and a compilable firmware DSP are cached');
  ok(ed.font, "offline: with its icon font");
  await page.goto(base + "editor/index.html?x=1");
  ok((await page.title()) === editorTitle, "offline: index.html and a query find the same copy");
  const miss = await page.goto(base + "nothing.html").then(() => "page").catch(() => "error");
  ok(miss === "error", "offline: what is not kept is not invented");
  await page.close(); // Chromium's pending error-page navigation must finish before the next test.
  page = await ctx.newPage();

  await serve();                                         // ---- online again, the site changed
  const f = join(site, "webapp", "editor", "index.html");
  const orig = readFileSync(f, "utf8");
  change(f, orig.replace(/<title>.*?<\/title>/, '<title>PWA refresh regression</title>'));
  await page.goto(base + "editor/");
  ok((await page.title()) === "PWA refresh regression", "online: the page as published now, not the old copy");
  await stop();
  await page.goto(base + "editor/");
  ok((await page.title()) === "PWA refresh regression", "offline after that: the copy is the newest one seen");
  writeFileSync(f, orig);

  await serve();                                         // ---- a new site: a new sw.js takes over, the old copy goes
  const swf = join(site, "webapp", "sw.js");
  const sw0 = readFileSync(swf, "utf8");
  change(swf, sw0.replace(/const STAMP = "([^"]*)"/, 'const STAMP = "$1-next"'));
  const keys = await page.evaluate(async () => {
    const reg = await navigator.serviceWorker.getRegistration();
    await reg.update();
    for (let i = 0; i < 80 && !(await caches.keys()).some((k) => k.endsWith("-next")); i++)
      await new Promise((r) => setTimeout(r, 100));
    for (let i = 0; i < 40 && (await caches.keys()).length > 1; i++) await new Promise((r) => setTimeout(r, 100));
    return caches.keys();
  });
  ok(keys.length === 1 && keys[0].endsWith("-next"), "a new site: its sw.js replaces the copy (" + keys.join(", ") + ")");
  writeFileSync(swf, sw0);
  await stop();

  /* (2.7) a new build keeps the firmware's name: the copy the browser kept of the build before (a server that lets it
     keep one, no service worker) is not offered as "damaged": the installer asks the server first */
  const fw = resolve(site, 'webapp/installer', meta.pkg), ix = join(site, "webapp", "installer", "index.html");
  const fw0 = readFileSync(fw), ix0 = readFileSync(ix, "utf8");
  cacheFirmware = true;
  await serve();
  const c2 = await browser.newContext({ serviceWorkers: "block" });
  const p2 = await c2.newPage();
  await p2.addInitScript(() => { if (!navigator.requestMIDIAccess) navigator.requestMIDIAccess = () => Promise.reject(new Error("none")); });
  await p2.goto(base + "installer/");
  await p2.waitForTimeout(800);
  const first = await p2.textContent("#status");
  const fw1 = Buffer.from(fw0); fw1[fw1.length - 1] ^= 0x55;
  change(fw, fw1);
  const { createHash } = await import("node:crypto");
  change(ix, ix0.replace(/"sha256": "[0-9a-f]+"/, `"sha256": "${createHash("sha256").update(fw1).digest("hex")}"`));
  await p2.reload();
  await p2.waitForTimeout(800);
  const second = await p2.textContent("#status");
  ok(first === status && second === status, "a new build of the same name: the new firmware, not the browser's old copy");
  writeFileSync(fw, fw0); writeFileSync(ix, ix0);
  await c2.close();
  const disk = await ctx.newPage();
  await disk.goto(pathToFileURL(resolve(site, 'webapp/editor/sloop-editor.html')).href + '?mock=1');
  await disk.locator('#connect').click();
  await disk.waitForFunction(() => document.getElementById('version').textContent.length > 0);
  ok(await disk.locator('#preview-enable').isDisabled() && await disk.locator('#preview-file-help').isVisible(),
     'downloaded file: audio limitation and offline-app link at the disabled control');
  await disk.close();
} finally {
  await stop();
  await browser.close();
  for (const [path, data] of originals) writeFileSync(path, data);
}
console.log("pwa:", failed ? `FAILED (${failed})` : "the site without internet: test passed");
process.exit(failed ? 1 : 0);
