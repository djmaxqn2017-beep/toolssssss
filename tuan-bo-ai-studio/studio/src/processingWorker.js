const ai = require('./aiService');
const exporter = require('./exportService');

process.on('message', async (message) => {
  const { id, type, payload } = message || {};
  if(!id || !type) return;
  try {
    let result;
    switch(type){
      case 'ping':
        result = { ok:true, pid:process.pid, role:'processing-worker' };
        break;
      case 'ai:status':
        result = ai.status();
        break;
      case 'ai:warmup':
        await ai.warmup();
        result = ai.status();
        break;
      case 'ai:segment-subject':
        result = await ai.segmentSubject(payload?.imagePath);
        break;
      case 'export:full':
        result = await exporter.exportImage(payload || {});
        break;
      default:
        throw new Error(`Unknown processing job: ${type}`);
    }
    if(process.send) process.send({ id, ok:true, result });
  } catch (error) {
    if(process.send) process.send({
      id,
      ok:false,
      error:error?.stack || error?.message || String(error)
    });
  }
});

process.on('uncaughtException', error => {
  if(process.send) process.send({ id:'__fatal__', ok:false, error:error?.stack || String(error) });
});
process.on('unhandledRejection', error => {
  if(process.send) process.send({ id:'__fatal__', ok:false, error:error?.stack || String(error) });
});
