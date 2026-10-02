const { app, BrowserWindow, dialog, ipcMain } = require('electron');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { ProcessingWorkerManager } = require('./workerManager');

app.commandLine.appendSwitch('enable-gpu-rasterization');
app.commandLine.appendSwitch('enable-zero-copy');
app.commandLine.appendSwitch('ignore-gpu-blocklist');
app.commandLine.appendSwitch('enable-native-gpu-memory-buffers');

let win;
let processing;

function createWindow(){
  win=new BrowserWindow({
    width:1600,height:960,minWidth:1180,minHeight:760,
    backgroundColor:'#100d14',title:'TBRetoch',
    webPreferences:{
      preload:path.join(__dirname,'preload.js'),
      contextIsolation:true,
      nodeIntegration:false,
      backgroundThrottling:false
    }
  });
  win.loadFile(path.join(__dirname,'renderer','index.html'));
}

app.whenReady().then(()=>{
  const resourcePath=app.isPackaged?process.resourcesPath:path.join(__dirname,'..','resources');
  processing=new ProcessingWorkerManager({resourcePath,userDataPath:app.getPath('userData')});
  createWindow();
  app.on('activate',()=>BrowserWindow.getAllWindows().length===0&&createWindow());
});
app.on('before-quit',()=>processing?.close());
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
  try{
    fs.mkdirSync(folder,{recursive:true});
    const out=path.join(folder,safeName(filename));
    const buffer=Buffer.isBuffer(bytes)?bytes:Buffer.from(bytes instanceof ArrayBuffer?new Uint8Array(bytes):bytes);
    fs.writeFileSync(out,buffer);
    return{ok:true,filePath:out,size:buffer.length};
  }catch(e){return{ok:false,error:e.message||String(e)};}
});
ipcMain.handle('export:write',async(_,{folder,filename,dataUrl})=>{
  try{
    const match=/^data:image\/(png|jpeg|webp);base64,(.+)$/.exec(dataUrl||'');
    if(!match)return{ok:false,error:'Dữ liệu ảnh không hợp lệ'};
    fs.mkdirSync(folder,{recursive:true});
    const out=path.join(folder,safeName(filename));
    const buffer=Buffer.from(match[2],'base64');
    fs.writeFileSync(out,buffer);
    return{ok:true,filePath:out,size:buffer.length};
  }catch(e){return{ok:false,error:e.message||String(e)};}
});
ipcMain.handle('export:full',async(_,payload)=>{
  try{return await processing.request('export:full',payload,10*60*1000);}
  catch(e){return{ok:false,error:e.message||String(e)};}
});

ipcMain.handle('system:performance',async()=>{
  let gpuInfo={};try{gpuInfo=await app.getGPUInfo('basic');}catch{}
  const worker=await processing?.diagnostics();
  return{
    cpus:os.cpus()?.length||0,
    cpuModel:os.cpus()?.[0]?.model||'CPU',
    memoryGB:Math.round(os.totalmem()/1073741824),
    gpuStatus:app.getGPUFeatureStatus(),
    gpuInfo,
    processing:worker||null,
    ai:worker?.ai||null,
    teamMode:true,
    architecture:'ui-process + dedicated-processing-worker'
  };
});
ipcMain.handle('processing:diagnostics',()=>processing?.diagnostics());

ipcMain.handle('ai:status',async()=>{
  try{return await processing.request('ai:status',{},5000);}
  catch(e){return{error:e.message,ready:false};}
});
ipcMain.handle('ai:warmup',async()=>{
  try{return await processing.request('ai:warmup',{},180000);}
  catch(e){return{error:e.message,ready:false};}
});
ipcMain.handle('ai:segment-subject',async(_,imagePath)=>{
  try{return await processing.request('ai:segment-subject',{imagePath},180000);}
  catch(e){return{ok:false,error:e.message||String(e)};}
});
