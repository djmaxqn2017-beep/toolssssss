import { fork } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here=path.dirname(fileURLToPath(import.meta.url));
const studio=path.resolve(here,'..');
const worker=path.join(studio,'src','processingWorker.js');
const resourcePath=path.join(studio,'resources');
const child=fork(worker,[],{
  env:{...process.env,TBRETOCH_PROCESS_ROLE:'processing-worker',TBRETOCH_RESOURCE_PATH:resourcePath},
  stdio:['ignore','inherit','inherit','ipc']
});
let seq=0;
const pending=new Map();
child.on('message',m=>{
  const p=pending.get(m?.id);if(!p)return;
  pending.delete(m.id);m.ok?p.resolve(m.result):p.reject(new Error(m.error));
});
function req(type,payload={},timeout=180000){
  const id=`smoke-${++seq}`;
  return new Promise((resolve,reject)=>{
    pending.set(id,{resolve,reject});child.send({id,type,payload});
    setTimeout(()=>{if(pending.delete(id))reject(new Error(`timeout ${type}`));},timeout).unref();
  });
}
try{
  const ping=await req('ping',{},5000);
  if(!ping?.ok) throw new Error('worker ping failed');
  const status0=await req('ai:status',{},5000);
  if(!status0.modelInstalled) throw new Error(`offline model missing: ${status0.modelPath}`);
  const warm=await req('ai:warmup',{},180000);
  if(!warm.ready) throw new Error(`AI did not warm: ${warm.error||'unknown'}`);
  console.log('TBRetoch processing worker smoke OK', {pid:ping.pid,backend:warm.backend,model:warm.model});
} finally {
  child.kill();
}
