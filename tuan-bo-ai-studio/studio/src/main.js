const { app, BrowserWindow, dialog, ipcMain, clipboard } = require('electron');
const fs = require('fs');
const path = require('path');
const license = require('./licenseService');
const ai = require('./aiService');

let win;

function createWindow(){
  win=new BrowserWindow({
    width:1500,height:920,minWidth:1100,minHeight:720,backgroundColor:'#120b19',title:'Tuấn Bồ AI Studio',
    webPreferences:{preload:path.join(__dirname,'preload.js'),contextIsolation:true,nodeIntegration:false}
  });
  win.loadFile(path.join(__dirname,'renderer','index.html'));
}

app.whenReady().then(()=>{createWindow();app.on('activate',()=>BrowserWindow.getAllWindows().length===0&&createWindow());});
app.on('window-all-closed',()=>process.platform!=='darwin'&&app.quit());

ipcMain.handle('files:open-images',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openFile','multiSelections'],filters:[{name:'Images',extensions:['jpg','jpeg','png','webp','bmp']}]});
  if(r.canceled)return[];
  return r.filePaths.map(p=>({path:p,name:path.basename(p),url:`file://${p.replace(/\\/g,'/')}`}));
});

ipcMain.handle('project:save',async(_,project)=>{
  const r=await dialog.showSaveDialog(win,{defaultPath:'TuanBoProject.tbproj',filters:[{name:'Tuấn Bồ Project',extensions:['tbproj']}]});
  if(r.canceled||!r.filePath)return false;
  fs.writeFileSync(r.filePath,JSON.stringify(project,null,2),'utf8');return true;
});
ipcMain.handle('project:load',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openFile'],filters:[{name:'Tuấn Bồ Project',extensions:['tbproj']}]});
  if(r.canceled||!r.filePaths[0])return null;
  try{return JSON.parse(fs.readFileSync(r.filePaths[0],'utf8'));}catch(e){return{__error:e.message};}
});

ipcMain.handle('export:choose-folder',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openDirectory','createDirectory']});return r.canceled?null:r.filePaths[0];
});
ipcMain.handle('export:write',async(_,{folder,filename,dataUrl})=>{
  const status=license.validateLicense();
  if(!status.valid)return{ok:false,error:status.reason||'License không hợp lệ',license:status};
  const match=/^data:image\/(png|jpeg);base64,(.+)$/.exec(dataUrl||'');
  if(!match)return{ok:false,error:'Dữ liệu ảnh không hợp lệ',license:status};
  try{
    fs.mkdirSync(folder,{recursive:true});
    const safeName=String(filename||'TuanBo.jpg').replace(/[<>:"/\\|?*]+/g,'_');
    fs.writeFileSync(path.join(folder,safeName),Buffer.from(match[2],'base64'));
    const after=license.consumeExports(1);
    if(!after.valid&&status.kind!=='timed')return{ok:false,error:after.reason||'Không thể trừ lượt xuất',license:after};
    return{ok:true,filePath:path.join(folder,safeName),license:license.validateLicense()};
  }catch(e){return{ok:false,error:e.message||String(e),license:license.validateLicense()};}
});

ipcMain.handle('ai:status',()=>ai.status());
ipcMain.handle('ai:warmup',async()=>{try{await ai.warmup();return ai.status();}catch(e){return{...ai.status(),error:e.message||String(e)};}});
ipcMain.handle('ai:segment-subject',async(_,imagePath)=>{try{return await ai.segmentSubject(imagePath);}catch(e){return{ok:false,error:e.message||String(e)};}});

ipcMain.handle('license:status',()=>license.validateLicense());
ipcMain.handle('license:machine-id',()=>license.stableMachineId());
ipcMain.handle('license:copy-machine-id',()=>{const id=license.stableMachineId();clipboard.writeText(id);return id;});
ipcMain.handle('license:reset-trust',()=>license.resetTrust());
ipcMain.handle('license:import-key',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openFile'],filters:[{name:'Public key',extensions:['pem']}]});
  if(r.canceled)return license.validateLicense();
  try{return license.importPublicKey(r.filePaths[0]);}catch(e){return{valid:false,reason:e.message||String(e),machineId:license.stableMachineId()};}
});
ipcMain.handle('license:import-license',async()=>{
  const r=await dialog.showOpenDialog(win,{properties:['openFile'],filters:[{name:'Tuấn Bồ License',extensions:['tblic']}]});
  if(r.canceled)return license.validateLicense();
  return license.importLicense(r.filePaths[0]);
});
