const CACHE_NAME = "taixiu-ai-cloud-v1";
const FILES_TO_CACHE = [
  "./",
  "index.html",
  "manifest.json",
  "icon.svg"
];

self.addEventListener("install", event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(cache => cache.addAll(FILES_TO_CACHE)).catch(() => null)
  );
  self.skipWaiting();
});

self.addEventListener("activate", event => {
  event.waitUntil(
    caches.keys().then(keys =>
      Promise.all(keys.map(key => key !== CACHE_NAME ? caches.delete(key) : null))
    )
  );
  self.clients.claim();
});

self.addEventListener("fetch", event => {
  const url = new URL(event.request.url);

  // Không cache config.json và API server, để luôn lấy link Cloudflare mới nhất
  if (url.pathname.endsWith("/config.json") || url.href.includes("/api/")) {
    event.respondWith(fetch(event.request));
    return;
  }

  event.respondWith(
    fetch(event.request).catch(() => caches.match(event.request).then(res => res || caches.match("./")))
  );
});
