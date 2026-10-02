import fs from 'node:fs';
import path from 'node:path';
import { chromium } from 'playwright-core';

const outDir = path.resolve(process.argv[2] || 'evoto-audit');
fs.mkdirSync(outDir, { recursive: true });
const pageUrl = 'https://www.evoto.ai/download';
const edgeCandidates = [
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Google/Chrome/Application/chrome.exe'
];
const executablePath = edgeCandidates.find(fs.existsSync);
if (!executablePath) throw new Error('No Edge/Chrome executable found on Windows runner');

const browser = await chromium.launch({
  headless: true,
  executablePath,
  args: ['--disable-features=Translate', '--no-first-run']
});
const context = await browser.newContext({
  acceptDownloads: true,
  userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/131 Safari/537.36'
});
const page = await context.newPage();
const network = [];
const possibleUrls = new Set();
page.on('response', async (response) => {
  const url = response.url();
  const ct = (response.headers()['content-type'] || '').toLowerCase();
  network.push({ status: response.status(), url, contentType: ct });
  if (/\.exe(?:$|\?)/i.test(url) || /download|installer|setup/i.test(url)) possibleUrls.add(url);
  if (/json|javascript|text/.test(ct) && /evoto\.ai/i.test(url)) {
    try {
      const len = Number(response.headers()['content-length'] || '0');
      if (!len || len < 4_000_000) {
        const text = await response.text();
        const matches = text.match(/https?:\\?\/\\?\/[^"'\s<>]+?(?:\.exe|\.msi)(?:\?[^"'\s<>]*)?/gi) || [];
        for (const m of matches) possibleUrls.add(m.replaceAll('\\/', '/'));
      }
    } catch {}
  }
});

await page.goto(pageUrl, { waitUntil: 'networkidle', timeout: 120000 });
await page.screenshot({ path: path.join(outDir, 'official-download-page.png'), fullPage: true }).catch(() => {});
const bodyText = await page.locator('body').innerText();
const html = await page.content();
fs.writeFileSync(path.join(outDir, 'official-download-page.txt'), bodyText, 'utf8');

for (const text of [html, bodyText]) {
  const matches = text.match(/https?:\\?\/\\?\/[^"'\s<>]+?(?:\.exe|\.msi)(?:\?[^"'\s<>]*)?/gi) || [];
  for (const m of matches) possibleUrls.add(m.replaceAll('\\/', '/'));
}

const candidates = await page.locator('a,button').evaluateAll((els) => els.map((el, index) => {
  const text = (el.innerText || el.textContent || '').trim().replace(/\s+/g, ' ');
  const href = el.href || el.getAttribute('href') || '';
  const aria = el.getAttribute('aria-label') || '';
  let p = el;
  let context = '';
  for (let i = 0; i < 5 && p; i++, p = p.parentElement) {
    const t = (p.innerText || '').trim().replace(/\s+/g, ' ');
    if (t.length > context.length && t.length < 1200) context = t;
  }
  return { index, text, href, aria, context, tag: el.tagName };
}));
fs.writeFileSync(path.join(outDir, 'download-controls.json'), JSON.stringify(candidates, null, 2), 'utf8');

const ranked = candidates
  .filter(x => /download|tải xuống/i.test(`${x.text} ${x.aria}`))
  .map(x => ({ ...x, score: (/windows|\bwin\b/i.test(x.context) ? 100 : 0) + (/\.exe|\.msi/i.test(x.href) ? 50 : 0) }))
  .sort((a, b) => b.score - a.score);

let savedInstaller = null;
let finalDownloadUrl = null;
for (const c of ranked) {
  if (savedInstaller) break;
  const loc = page.locator('a,button').nth(c.index);
  try {
    const downloadPromise = page.waitForEvent('download', { timeout: 12000 }).catch(() => null);
    const navPromise = page.waitForEvent('request', req => /\.exe(?:$|\?)/i.test(req.url()), { timeout: 12000 }).catch(() => null);
    await loc.click({ timeout: 8000, force: true });
    const download = await downloadPromise;
    if (download) {
      const suggested = download.suggestedFilename() || 'Evoto-Official-Setup.exe';
      const ext = path.extname(suggested) || '.exe';
      savedInstaller = path.join(outDir, `Evoto-Official-Setup${ext}`);
      await download.saveAs(savedInstaller);
      finalDownloadUrl = download.url();
      break;
    }
    const req = await navPromise;
    if (req) {
      finalDownloadUrl = req.url();
      possibleUrls.add(finalDownloadUrl);
    }
    if (page.url() !== pageUrl) await page.goto(pageUrl, { waitUntil: 'networkidle', timeout: 120000 }).catch(() => {});
  } catch {}
}

if (!savedInstaller) {
  const urls = [...possibleUrls].filter(u => /\.exe(?:$|\?)/i.test(u));
  for (const u of urls) {
    try {
      const response = await context.request.get(u, { timeout: 120000 });
      if (response.ok()) {
        const buf = await response.body();
        if (buf.length > 5_000_000) {
          savedInstaller = path.join(outDir, 'Evoto-Official-Setup.exe');
          fs.writeFileSync(savedInstaller, buf);
          finalDownloadUrl = u;
          break;
        }
      }
    } catch {}
  }
}

const versions = [...new Set((bodyText.match(/\b\d+\.\d+\.\d+(?:-\d+)?\b/g) || []))];
const summary = {
  auditedAt: new Date().toISOString(),
  pageUrl,
  browser: executablePath,
  versionsFound: versions,
  installerDownloaded: !!savedInstaller,
  installerPath: savedInstaller,
  finalDownloadUrl,
  possibleDownloadUrls: [...possibleUrls].slice(0, 200),
  rankedControls: ranked.slice(0, 30),
  networkCount: network.length
};
fs.writeFileSync(path.join(outDir, 'download-audit-summary.json'), JSON.stringify(summary, null, 2), 'utf8');
fs.writeFileSync(path.join(outDir, 'network-urls.json'), JSON.stringify(network, null, 2), 'utf8');
console.log(JSON.stringify(summary, null, 2));
await browser.close();
if (!savedInstaller) process.exitCode = 2;
