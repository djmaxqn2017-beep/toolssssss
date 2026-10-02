const { app } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { pathToFileURL } = require('url');

let segmenter=null;
let loadingPromise=null;
let lastError=null;
let loadStartedAt=null;

function modelRoot(){return app.isPackaged?path.join(process.resourcesPath,'models'):path.join(__dirname,'..','resources','models');}
function modelDir(){return path.join(modelRoot(),'onnx-community','BiRefNet_lite-ONNX');}
function modelFile(){return path.join(modelDir(),'onnx','model.onnx');}
function hasModel(){try{return fs.existsSync(modelFile())&&fs.statSync(modelFile()).size>100*1024*1024;}catch{return false;}}

async function ensureSegmenter(){
  if(segmenter)return segmenter;
  if(loadingPromise)return loadingPromise;
  loadingPromise=(async()=>{
    loadStartedAt=Date.now();
    try{
      if(!hasModel())throw new Error(`Không tìm thấy model BiRefNet offline: ${modelFile()}`);
      const mod=await import('@huggingface/transformers');
      const {env,pipeline}=mod;
      env.allowRemoteModels=false;
      env.allowLocalModels=true;
      env.localModelPath=modelRoot();
      env.cacheDir=path.join(app.getPath('userData'),'ai-cache');
      env.useBrowserCache=false;
      segmenter=await pipeline('image-segmentation','onnx-community/BiRefNet_lite-ONNX',{device:'cpu',dtype:'fp32'});
      lastError=null;
      return segmenter;
    }catch(e){
      lastError=e;
      loadingPromise=null;
      throw e;
    }
  })();
  return loadingPromise;
}

async function warmup(){await ensureSegmenter();return true;}

async function segmentSubject(imagePath){
  if(!imagePath||!fs.existsSync(imagePath))throw new Error('Không tìm thấy ảnh nguồn để AI phân tích.');
  const pipe=await ensureSegmenter();
  const mod=await import('@huggingface/transformers');
  let input=imagePath;
  try{input=await mod.RawImage.read(pathToFileURL(imagePath).href);}catch{}
  const result=await pipe(input,{mask_threshold:0.20});
  const items=Array.isArray(result)?result:[result];
  const item=items.find(x=>x&&x.mask)||items[0];
  if(!item||!item.mask)throw new Error('BiRefNet không tạo được mask chủ thể.');
  const tmp=path.join(os.tmpdir(),`tuanbo-mask-${Date.now()}-${Math.random().toString(16).slice(2)}.png`);
  await item.mask.save(tmp);
  const data=fs.readFileSync(tmp);try{fs.unlinkSync(tmp);}catch{}
  return{ok:true,width:item.mask.width,height:item.mask.height,dataUrl:`data:image/png;base64,${data.toString('base64')}`,label:item.label||'subject'};
}

function status(){
  let modelBytes=0;try{modelBytes=fs.statSync(modelFile()).size;}catch{}
  return{modelInstalled:hasModel(),model:'BiRefNet_lite-ONNX',modelBytes,runtime:'@huggingface/transformers 3.8.1',ready:!!segmenter,loading:!!loadingPromise&&!segmenter,loadSeconds:loadStartedAt&&!segmenter?Math.round((Date.now()-loadStartedAt)/1000):0,error:lastError?lastError.message:null,modelPath:modelFile()};
}

module.exports={segmentSubject,status,warmup};
