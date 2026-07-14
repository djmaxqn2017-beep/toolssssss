const CACHE_VERSION='taixiu-v7250';

self.addEventListener('install',event=>self.skipWaiting());

self.addEventListener('activate',event=>{
  event.waitUntil(Promise.all([
    self.clients.claim(),
    caches.keys().then(keys=>Promise.all(keys.map(key=>caches.delete(key))))
  ]));
});

self.addEventListener('fetch',event=>{
  if(event.request.method!=='GET')return;

  const url=new URL(event.request.url);
  const neverCache=
    url.pathname.endsWith('/config.json') ||
    url.pathname.endsWith('/index.html') ||
    url.pathname.includes('/api/');

  if(neverCache){
    event.respondWith(fetch(event.request,{cache:'no-store'}));
    return;
  }

  event.respondWith(
    fetch(event.request,{cache:'no-store'})
      .catch(()=>caches.match(event.request))
  );
});
