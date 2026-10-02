const { app, BrowserWindow, dialog, ipcMain, net } = require('electron');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { pathToFileURL } = require('url');
const { ProcessingWorkerManager } = require('./workerManager');

app.commandLine.appendSwitch('enable-gpu-rasterization');
app.commandLine.appendSwitch('enable-zero-copy');
app.commandLine.appendSwitch('ignore-gpu-blocklist');
app.commandLine.appendSwitch('enable-native-gpu-memory-buffers');

let win;
let processing;

const VISION_MODELS = {
  face: {
    name: 'face_landmarker.task',
    url: 'https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task',
    minBytes: 2500000
  },
  pose: {
    name: 'pose_landmarker_full.task',
    url: 'https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_full/float16/1/pose_landmarker_full.task',
    minBytes: 5000000
  },
  semantic: {
    name: 'selfie_multiclass_256x256.tflite',
    url: 'https://storage.googleapis.com/mediapipe-models/image_segmenter/selfie_multiclass_256x256/float32/latest/selfie_multiclass_256x256.tflite',
    minBytes: 100000
  }
};

function visionModelDir(){
  return path.join(app.getPath('userData'),'models','mediapipe');
}
function visionModelPath(key){
  const spec=VISION_MODELS[key];
  return spec?path.join(visionModelDir(),spec.name):null;
}
function isValidFile(file,minBytes){
  try{return fs.existsSync(file)&&fs.statSync(file).size>=minBytes;}catch{return false;}
}
function modelStatus(){
  const models={};
  for(const [key,spec] of Object.entries(VISION_MODELS)){
    const file=visionModelPath(key);
    let bytes=0;try{bytes=fs.statSync(file).size;}catch{}
    models[key]={name:spec.name,installed:isValidFile(file,spec.minBytes),bytes,path:file};
  }
  return {ready:Object.values(models).every(x=>x.installed),models,directory:visionModelDir()};
}
async function downloadModel(key){
  const spec=VISION_MODELS[key];
  if(!spec)throw new Error(`Unknown model: ${key}`);
  const target=visionModelPath(key);
  if(isValidFile(target,spec.minBytes))return target;
  fs.mkdirSync(path.dirname(target),{recursive:true});
  const temp=`${target}.download`;
  const response=await net.fetch(spec.url,{redirect:'follow'});
  if(!response.ok)throw new Error(`Model ${key} download failed: HTTP ${response.status}`);
  const bytes=Buffer.from(await response.arrayBuffer());
  if(bytes.length<spec.minBytes)throw new Error(`Model ${key} download incomplete (${bytes.length} bytes)`);
  fs.writeFileSync(temp,bytes);
  fs.renameSync(temp,target);
  return target;
}
async function ensureVisionModels(){
  const installed=[];
  for(const key of Object.keys(VISION_MODELS)){
    const file=await downloadModel(key);
    installed.push({key,file,bytes:fs.statSync(file).size});
  }
  return {...modelStatus(),installed};
}
function mediapipeRuntimePaths(){
  const resolved=require.resolve('@mediapipe/tasks-vision');
  const pkgDir=path.dirname(resolved);
  const unpacked=resolved.includes('app.asar')?resolved.replace('app.asar','app.asar.unpacked'):resolved;
  const root=path.dirname(unpacked);
  const bundleCandidates=[
    path.join(root,'vision_bundle.mjs'),
    path.join(root,'vision_bundle.js'),
    path.join(pkgDir,'vision_bundle.mjs'),
    path.join(pkgDir,'vision_bundle.js')
  ];
  const wasmCandidates=[path.join(root,'wasm'),path.join(pkgDir,'wasm')];
  const bundle=bundleCandidates.find(fs.existsSync);
  const wasm=wasmCandidates.find(fs.existsSync);
  if(!bundle||!wasm)throw new Error('Không tìm thấy MediaPipe Tasks Vision runtime trong bộ cài.');
  const s=modelStatus();
  return {
    bundleUrl:pathToFileURL(bundle).href,
    wasmUrl:pathToFileURL(wasm+path.sep).href,
    faceModelUrl:pathToFileURL(visionModelPath('face')).href,
    poseModelUrl:pathToFileURL(visionModelPath('pose')).href,
    semanticModelUrl:pathToFileURL(visionModelPath('semantic')).href,
    modelStatus:s
  };
}

function createWindow(){
  win=new BrowserWindow({
    width:1640,height:980,minWidth:1180,minHeight:760,
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
  const r=await dialog.showOpenDialog(win,{properties:['openFile','multiSelections'],filters:[{name:'Images',extensions:['jpg','jpeg','png','webp','bmp','tif','tiff','dng','nef','cr2','cr3','arw','raf','orf','rw2']}]});
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
ipcMain.handle('export:full',async(_,payload)=>{
  try{return await processing.request('export:full',payload,10*60*1000);}
  catch(e){return{ok:false,error:e.message||String(e)};}
});

ipcMain.handle('models:status',()=>modelStatus());
ipcMain.handle('models:prepare',async()=>{
  try{return{ok:true,...await ensureVisionModels()};}
  catch(e){return{ok:false,error:e.message||String(e),...modelStatus()};}
});
ipcMain.handle('vision:runtime-paths',()=>{
  try{return{ok:true,...mediapipeRuntimePaths()};}
  catch(e){return{ok:false,error:e.message||String(e),modelStatus:modelStatus()};}
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
    architecture:'ui-process + semantic-vision-runtime + dedicated-processing-worker'
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
