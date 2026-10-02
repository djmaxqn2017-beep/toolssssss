const { app, BrowserWindow, dialog, ipcMain, clipboard } = require('electron');
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
function fingerprint(pem){ return crypto.createHash('sha256').update(String(pem).trim()).digest('hex').slice(0,16).toUpperCase(); }
function issueLicense(input){
  ensureKeys();
  const now=new Date();
  const machineId=String(input.machineId||'').trim().toUpperCase();
  const kind=input.kind==='timed'?'timed':'quota';
  const payload={
    licenseId: crypto.randomUUID(),
    customer: String(input.customer||'').trim(),
    machineId,
    kind,
    issuedAt: now.toISOString(),
    product: 'TuanBo-AI-Studio'
  };
  if(kind==='quota'){
    const total=Math.max(1,Math.floor(Number(input.total||0)));
    payload.total=total;
  }else{
    const days=Math.max(1,Math.floor(Number(input.days||0)));
    payload.days=days;
    payload.expiresAt=new Date(now.getTime()+days*86400000).toISOString();
  }
  const priv=fs.readFileSync(privateKeyPath(),'utf8');
  const pub=fs.readFileSync(publicKeyPath(),'utf8');
  const signature=crypto.sign(null,Buffer.from(canonical(payload)),priv).toString('base64');
  return {
    format:'TBLIC2',
    payload,
    signature,
    signerPublicKey:pub,
    signerFingerprint:fingerprint(pub)
  };
}
function createWindow(){
  win=new BrowserWindow({width:980,height:740,minWidth:820,minHeight:620,backgroundColor:'#120b19',title:'Tuấn Bồ License Center',webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false}});
  win.loadFile(path.join(__dirname,'renderer','index.html'));
}
app.whenReady().then(()=>{ensureKeys();createWindow();app.on('activate',()=>BrowserWindow.getAllWindows().length===0&&createWindow());});
app.on('window-all-closed',()=>process.platform!=='darwin'&&app.quit());

ipcMain.handle('license:key-info',()=>{ensureKeys();const publicKey=fs.readFileSync(publicKeyPath(),'utf8');return{publicKey,keyDir:keyDir(),fingerprint:fingerprint(publicKey)};});
ipcMain.handle('license:copy-key-fingerprint',()=>{ensureKeys();const fp=fingerprint(fs.readFileSync(publicKeyPath(),'utf8'));clipboard.writeText(fp);return fp;});
ipcMain.handle('license:export-public',async()=>{
  ensureKeys(); const r=await dialog.showSaveDialog(win,{defaultPath:'TuanBo-AI-Studio-Public-Key.pem',filters:[{name:'PEM',extensions:['pem']}]});
  if(r.canceled||!r.filePath)return false;fs.copyFileSync(publicKeyPath(),r.filePath);return true;
});
ipcMain.handle('license:issue',async(_,input)=>{
  if(!String(input?.customer||'').trim()) return {ok:false,error:'Thiếu tên khách hàng'};
  if(!/^[A-F0-9]{24}$/i.test(String(input?.machineId||'').trim())) return {ok:false,error:'Machine ID phải gồm 24 ký tự A-F/0-9 từ Tuấn Bồ AI Studio'};
  const doc=issueLicense(input);
  const safe=(doc.payload.customer||'customer').replace(/[^a-zA-Z0-9_-]+/g,'_').slice(0,40)||'customer';
  const r=await dialog.showSaveDialog(win,{defaultPath:`TuanBo-${safe}-${doc.payload.licenseId.slice(0,8)}.tblic`,filters:[{name:'Tuấn Bồ License',extensions:['tblic']}]});
  if(r.canceled||!r.filePath)return {ok:false,canceled:true};
  fs.writeFileSync(r.filePath,JSON.stringify(doc,null,2),'utf8');
  return {ok:true,filePath:r.filePath,payload:doc.payload,signerFingerprint:doc.signerFingerprint};
});
