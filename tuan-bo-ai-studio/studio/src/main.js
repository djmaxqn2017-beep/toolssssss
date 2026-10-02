const { app, BrowserWindow, dialog, ipcMain, clipboard } = require('electron');
const fs = require('fs');
const path = require('path');
const os = require('os');
const license = require('./licenseService');
const ai = require('./aiService');
const exporter = require('./exportService');

app.commandLine.appendSwitch('enable-gpu-rasterization');
app.commandLine.appendSwitch('enable-zero-copy');
app.commandLine.appendSwitch('ignore-gpu-blocklist');

let win;

function createWindow(){
  win=new BrowserWindow({
    width:1600,height:960,minWidth:1180,minHeight:760,
    backgroundColor:'#100d14',title:'TBRetoch',
    webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false,backgroundThrottling:false}
  });
  win.loadFile(path.join(__dirname,'renderer','index.html'));
}

app.whenReady().then(()=>{createWindow();app.on('activate',()=>BrowserWindow.getAllWindows().length===0&&createWindow());});
app.on('window-all-closed',()=>process.platform!=='darwin'&&app.quit());

ipcMain.handle('files:open-images',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openFile','multiSelections'],filters:[{name:'Images',extensions:['jpg','jpeg','png','webp','bmp','tif','tiff']}]});
  if(r.canceled)return[];
  return r.filePaths.map(p=>({path:p,name:path.basename(p),url:`file://${p.replace(/\\/g,'/')}`}));
});

ipcMain.handle('project:save',async(_,project)=>{
  const r=await dialog.showSaveDialog(win,{defaultPath:'TBRetoch-Project.tbproj',filters:[{name:'TBRetoch Project',extensions:['tbproj']}]});
  if(r.canceled||!r.filePath)return false;
  fs.writeFileSync(r.filePath,JSON.stringify(project,null,2),'utf8');return true;
});
ipcMain.handle('project:load',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openFile'],filters:[{name:'TBRetoch Project',extensions:['tbproj']}]});
  if(r.canceled||!r.filePaths[0])return null;
  try{return JSON.parse(fs.readFileSync(r.filePaths[0],'utf8'));}catch(e){return{__error:e.message};}
});

ipcMain.handle('export:choose-folder',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openDirectory','createDirectory']});return r.canceled?null:r.filePaths[0];
});
function safeName(name){return String(name||'TBRetoch.jpg').replace(/[<>:"/\\|?*]+/g,'_');}
ipcMain.handle('export:write-buffer',async(_,{folder,filename,bytes})=>{
  try{fs.mkdirSync(folder,{recursive:true});const out=path.join(folder,safeName(filename));const buffer=Buffer.isBuffer(bytes)?bytes:Buffer.from(bytes instanceof ArrayBuffer?new Uint8Array(bytes):bytes);fs.writeFileSync(out,buffer);return{ok:true,filePath:out,size:buffer.length};}
  catch(e){return{ok:false,error:e.message||String(e)};}
});
ipcMain.handle('export:write',async(_,{folder,filename,dataUrl})=>{
  try{const match=/^data:image\/(png|jpeg|webp);base64,(.+)$/.exec(dataUrl||'');if(!match)return{ok:false,error:'Dữ liệu ảnh không hợp lệ'};fs.mkdirSync(folder,{recursive:true});const out=path.join(folder,safeName(filename));const buffer=Buffer.from(match[2],'base64');fs.writeFileSync(out,buffer);return{ok:true,filePath:out,size:buffer.length};}
  catch(e){return{ok:false,error:e.message||String(e)};}
});
ipcMain.handle('export:full',async(_,payload)=>{try{return await exporter.exportImage(payload);}catch(e){return{ok:false,error:e.message||String(e)};}});

ipcMain.handle('system:performance',async()=>{
  let gpuInfo={};try{gpuInfo=await app.getGPUInfo('basic');}catch{}
  return{cpus:os.cpus()?.length||0,cpuModel:os.cpus()?.[0]?.model||'CPU',memoryGB:Math.round(os.totalmem()/1073741824),gpuStatus:app.getGPUFeatureStatus(),gpuInfo,ai:ai.status(),teamMode:true};
});

ipcMain.handle('ai:status',()=>ai.status());
ipcMain.handle('ai:warmup',async()=>{try{await ai.warmup();return ai.status();}catch(e){return{...ai.status(),error:e.message||String(e)};}});
ipcMain.handle('ai:segment-subject',async(_,imagePath)=>{try{return await ai.segmentSubject(imagePath);}catch(e){return{ok:false,error:e.message||String(e)};}});

// Team build: license no longer blocks editing or export.
ipcMain.handle('license:status',()=>({valid:true,kind:'team',customer:'TB Team',machineId:license.stableMachineId()}));
ipcMain.handle('license:machine-id',()=>license.stableMachineId());
ipcMain.handle('license:copy-machine-id',()=>{const id=license.stableMachineId();clipboard.writeText(id);return id;});
ipcMain.handle('license:reset-trust',()=>license.resetTrust());
ipcMain.handle('license:import-key',async()=>({valid:true,kind:'team',customer:'TB Team',machineId:license.stableMachineId()}));
ipcMain.handle('license:import-license',async()=>({valid:true,kind:'team',customer:'TB Team',machineId:license.stableMachineId()}));
