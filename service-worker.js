const CACHE_NAME = "taixiu-ai-cloud-v2-1";

self.addEventListener("install", event => {
  self.skipWaiting();
});

self.addEventListener("activate", event => {
  event.waitUntil(
    caches.keys().then(keys => Promise.all(keys.map(key => key !== CACHE_NAME ? caches.delete(key) : null)))
  );
  self.clients.claim();
});

self.addEventListener("fetch", event => {
  const url = new URL(event.request.url);

  // Không cache HTML, config và API để tránh kẹt bản cũ.
  if (url.pathname.endsWith("/") || url.pathname.endsWith("index.html") || url.pathname.endsWith("config.json") || url.href.includes("/api/")) {
    event.respondWith(fetch(event.request, { cache: "no-store" }));
    return;
  }

  event.respondWith(
    fetch(event.request).catch(() => caches.match(event.request))
  );
});
