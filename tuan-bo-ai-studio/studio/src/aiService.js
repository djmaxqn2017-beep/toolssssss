const { app } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');
const ort = require('onnxruntime-node');
const sharp = require('sharp');

let session = null;
let loadingPromise = null;
let lastError = null;
let backend = 'Chưa khởi tạo';
let loadStartedAt = null;

function modelRoot(){ return app.isPackaged ? path.join(process.resourcesPath,'models') : path.join(__dirname,'..','resources','models'); }
function modelDir(){ return path.join(modelRoot(),'onnx-community','BiRefNet_lite-ONNX'); }
function modelFile(){ return path.join(modelDir(),'onnx','model.onnx'); }
function hasModel(){ try { return fs.existsSync(modelFile()) && fs.statSync(modelFile()).size > 100*1024*1024; } catch { return false; } }

function orientedSize(meta){
  const swap = [5,6,7,8].includes(Number(meta.orientation||1));
  return { width: swap ? meta.height : meta.width, height: swap ? meta.width : meta.height };
}

async function createSession(){
  if(session) return session;
  if(loadingPromise) return loadingPromise;
  loadingPromise = (async()=>{
    loadStartedAt = Date.now();
    if(!hasModel()) throw new Error(`Không tìm thấy model BiRefNet offline: ${modelFile()}`);
    const cpuThreads = Math.max(2, Math.min(12, (os.cpus()?.length || 4) - 1));
    if(process.platform === 'win32'){
      try{
        session = await ort.InferenceSession.create(modelFile(),{
          executionProviders:[{name:'dml',deviceId:0},'cpu'],
          executionMode:'sequential',
          enableMemPattern:false,
          graphOptimizationLevel:'all',
          intraOpNumThreads:cpuThreads
        });
        backend = 'DirectML GPU + CPU fallback';
        lastError = null;
        return session;
      }catch(e){
        lastError = e;
      }
    }
    session = await ort.InferenceSession.create(modelFile(),{
      executionProviders:['cpu'],
      graphOptimizationLevel:'all',
      executionMode:'parallel',
      intraOpNumThreads:cpuThreads,
      interOpNumThreads:2
    });
    backend = 'CPU đa luồng';
    lastError = null;
    return session;
  })().catch(e=>{ loadingPromise=null; lastError=e; throw e; });
  return loadingPromise;
}

async function preprocess(imagePath){
  const meta = await sharp(imagePath,{failOn:'none'}).metadata();
  if(!meta.width || !meta.height) throw new Error('Không đọc được kích thước ảnh.');
  const oriented = orientedSize(meta);
  const {data,info} = await sharp(imagePath,{failOn:'none'})
    .rotate()
    .resize(1024,1024,{fit:'fill',kernel:'lanczos3'})
    .flatten({background:{r:255,g:255,b:255}})
    .toColourspace('srgb')
    .removeAlpha()
    .raw()
    .toBuffer({resolveWithObject:true});
  const channels = info.channels;
  const pixels = 1024*1024;
  const input = new Float32Array(3*pixels);
  const mean=[0.485,0.456,0.406], std=[0.229,0.224,0.225];
  for(let i=0;i<pixels;i++){
    const base=i*channels;
    input[i] = (data[base]/255-mean[0])/std[0];
    input[pixels+i] = (data[base+1]/255-mean[1])/std[1];
    input[pixels*2+i] = (data[base+2]/255-mean[2])/std[2];
  }
  return {input, width:oriented.width, height:oriented.height};
}

function sigmoid(x){ return x>=0 ? 1/(1+Math.exp(-x)) : Math.exp(x)/(1+Math.exp(x)); }

async function segmentSubjectBuffer(imagePath){
  if(!imagePath || !fs.existsSync(imagePath)) throw new Error('Không tìm thấy ảnh nguồn để AI phân tích.');
  const s = await createSession();
  const p = await preprocess(imagePath);
  const inputName = s.inputNames[0];
  const outputName = s.outputNames[0];
  const tensor = new ort.Tensor('float32',p.input,[1,3,1024,1024]);
  const outputs = await s.run({[inputName]:tensor});
  const out = outputs[outputName]?.data;
  if(!out || out.length < 1024*1024) throw new Error('BiRefNet không tạo được mask hợp lệ.');
  const mask = Buffer.allocUnsafe(1024*1024);
  for(let i=0;i<mask.length;i++) mask[i]=Math.max(0,Math.min(255,Math.round(sigmoid(out[i])*255)));
  const png = await sharp(mask,{raw:{width:1024,height:1024,channels:1}})
    .resize(p.width,p.height,{fit:'fill',kernel:'lanczos3'})
    .png({compressionLevel:6})
    .toBuffer();
  return {buffer:png,width:p.width,height:p.height};
}

async function segmentSubject(imagePath){
  const r = await segmentSubjectBuffer(imagePath);
  return {ok:true,width:r.width,height:r.height,dataUrl:`data:image/png;base64,${r.buffer.toString('base64')}`,label:'subject',backend};
}

async function warmup(){ await createSession(); return true; }

function status(){
  let modelBytes=0; try{modelBytes=fs.statSync(modelFile()).size;}catch{}
  return {
    modelInstalled:hasModel(),model:'BiRefNet_lite-ONNX',modelBytes,
    runtime:'ONNX Runtime Node 1.30',backend,ready:!!session,
    loading:!!loadingPromise&&!session,
    loadSeconds:loadStartedAt&&!session?Math.round((Date.now()-loadStartedAt)/1000):0,
    error:lastError?lastError.message:null,modelPath:modelFile(),cpuThreads:os.cpus()?.length||0
  };
}

module.exports={segmentSubject,segmentSubjectBuffer,status,warmup};
