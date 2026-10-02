const { app, BrowserWindow, dialog, ipcMain } = require('electron');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

let win;
function keyDir(){ const p=path.join(app.getPath('userData'),'keys'); fs.mkdirSync(p,{recursive:true}); return p; }
function privateKeyPath(){ return path.join(keyDir(),'ed25519-private.pem'); }
function publicKeyPath(){ return path.join(keyDir(),'ed25519-public.pem'); }
function ensureKeys(){
  if(!fs.existsSync(privateKeyPath()) || !fs.existsSync(publicKeyPath())){
    const { publicKey, privateKey } = crypto.generateKeyPairSync('ed25519');
    fs.writeFileSync(privateKeyPath(), privateKey.export({type:'pkcs8',format:'pem'}));
    fs.writeFileSync(publicKeyPath(), publicKey.export({type:'spki',format:'pem'}));
  }
}
function canonical(payload){ return JSON.stringify(payload); }
function issueLicense(input){
  ensureKeys();
  const now=new Date();
  const payload={
    licenseId: crypto.randomUUID(),
    customer: String(input.customer||'').trim(),
    machineId: String(input.machineId||'').trim().toUpperCase(),
    kind: input.kind,
    issuedAt: now.toISOString()
  };
  if(payload.kind==='quota') payload.total=Number(input.total||0);
  if(payload.kind==='timed'){
    const days=Number(input.days||0); const exp=new Date(now.getTime()+days*86400000);
    payload.expiresAt=exp.toISOString();
  }
  const priv=fs.readFileSync(privateKeyPath(),'utf8');
  const signature=crypto.sign(null,Buffer.from(canonical(payload)),priv).toString('base64');
  return {payload,signature};
}
function createWindow(){
  win=new BrowserWindow({width:960,height:720,minWidth:820,minHeight:620,backgroundColor:'#120b19',title:'Tuấn Bồ License Center',webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false}});
  win.loadFile(path.join(__dirname,'renderer','index.html'));
}
app.whenReady().then(()=>{ensureKeys();createWindow();app.on('activate',()=>BrowserWindow.getAllWindows().length===0&&createWindow());});
app.on('window-all-closed',()=>process.platform!=='darwin'&&app.quit());

ipcMain.handle('license:key-info',()=>({publicKey:fs.readFileSync(publicKeyPath(),'utf8'),keyDir:keyDir()}));
ipcMain.handle('license:export-public',async()=>{
  ensureKeys(); const r=await dialog.showSaveDialog(win,{defaultPath:'TuanBo-AI-Studio-Public-Key.pem',filters:[{name:'PEM',extensions:['pem']}]});
  if(r.canceled||!r.filePath)return false;fs.copyFileSync(publicKeyPath(),r.filePath);return true;
});
ipcMain.handle('license:issue',async(_,input)=>{
  const doc=issueLicense(input);
  const safe=(doc.payload.customer||'customer').replace(/[^a-zA-Z0-9_-]+/g,'_').slice(0,40)||'customer';
  const r=await dialog.showSaveDialog(win,{defaultPath:`TuanBo-${safe}-${doc.payload.licenseId.slice(0,8)}.tblic`,filters:[{name:'Tuấn Bồ License',extensions:['tblic']}]});
  if(r.canceled||!r.filePath)return {ok:false,canceled:true};
  fs.writeFileSync(r.filePath,JSON.stringify(doc,null,2),'utf8');
  return {ok:true,filePath:r.filePath,payload:doc.payload};
});
