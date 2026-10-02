const { fork } = require('child_process');
const path = require('path');
const crypto = require('crypto');

class ProcessingWorkerManager {
  constructor({ resourcePath, userDataPath }){
    this.resourcePath = resourcePath;
    this.userDataPath = userDataPath;
    this.child = null;
    this.pending = new Map();
    this.closed = false;
    this.lastError = null;
    this.start();
  }

  start(){
    if(this.closed || this.child) return;
    const workerPath = path.join(__dirname,'processingWorker.js');
    const env = {
      ...process.env,
      ELECTRON_RUN_AS_NODE:'1',
      TBRETOCH_PROCESS_ROLE:'processing-worker',
      TBRETOCH_RESOURCE_PATH:this.resourcePath || '',
      TBRETOCH_USER_DATA:this.userDataPath || ''
    };
    const child = fork(workerPath,[],{
      execPath:process.execPath,
      env,
      stdio:['ignore','pipe','pipe','ipc'],
      windowsHide:true
    });
    this.child = child;
    child.stdout?.on('data', d => process.stdout.write(`[TB worker] ${d}`));
    child.stderr?.on('data', d => process.stderr.write(`[TB worker] ${d}`));
    child.on('message', msg => this.onMessage(msg));
    child.on('error', err => { this.lastError=err; });
    child.on('exit',(code,signal)=>{
      this.child=null;
      const err=new Error(`TB processing worker stopped (code=${code}, signal=${signal||''})`);
      this.lastError=err;
      for(const {reject,timer} of this.pending.values()){
        clearTimeout(timer);reject(err);
      }
      this.pending.clear();
      if(!this.closed) setTimeout(()=>this.start(),600);
    });
  }

  onMessage(msg){
    if(!msg?.id || msg.id==='__fatal__'){
      if(msg?.error) this.lastError=new Error(msg.error);
      return;
    }
    const p=this.pending.get(msg.id);
    if(!p) return;
    this.pending.delete(msg.id);
    clearTimeout(p.timer);
    if(msg.ok) p.resolve(msg.result);
    else p.reject(new Error(msg.error||'Processing worker error'));
  }

  request(type,payload={},timeoutMs=180000){
    if(this.closed) return Promise.reject(new Error('Processing worker đã đóng.'));
    if(!this.child) this.start();
    const id=crypto.randomUUID();
    return new Promise((resolve,reject)=>{
      const timer=setTimeout(()=>{
        this.pending.delete(id);
        reject(new Error(`Processing job timeout: ${type}`));
      },timeoutMs);
      this.pending.set(id,{resolve,reject,timer,type});
      try{ this.child.send({id,type,payload}); }
      catch(e){ clearTimeout(timer);this.pending.delete(id);reject(e); }
    });
  }

  async diagnostics(){
    try{
      const ping=await this.request('ping',{},5000);
      const ai=await this.request('ai:status',{},5000);
      return { alive:true,pid:ping.pid,ai,lastError:this.lastError?.message||null,pending:this.pending.size };
    }catch(e){
      return { alive:false,error:e.message,lastError:this.lastError?.message||null,pending:this.pending.size };
    }
  }

  close(){
    this.closed=true;
    if(this.child){ try{this.child.kill();}catch{} this.child=null; }
    for(const {reject,timer} of this.pending.values()){
      clearTimeout(timer);reject(new Error('TBRetoch is closing'));
    }
    this.pending.clear();
  }
}

module.exports={ProcessingWorkerManager};
