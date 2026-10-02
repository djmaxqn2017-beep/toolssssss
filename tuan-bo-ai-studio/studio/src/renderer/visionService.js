let runtime=null;
let faceLandmarker=null;
let poseLandmarker=null;
let segmenter=null;
let backend='not-ready';
let initPromise=null;
let lastError=null;

function bytesOf(value){
  if(value instanceof Uint8Array)return value;
  if(value?.type==='Buffer'&&Array.isArray(value.data))return new Uint8Array(value.data);
  if(value instanceof ArrayBuffer)return new Uint8Array(value);
  if(ArrayBuffer.isView(value))return new Uint8Array(value.buffer,value.byteOffset,value.byteLength);
  return new Uint8Array(value||[]);
}

async function closeTasks(){
  for(const task of [faceLandmarker,poseLandmarker,segmenter]){
    try{task?.close?.();}catch{}
  }
  faceLandmarker=null;poseLandmarker=null;segmenter=null;
}

async function createTasks(delegate){
  const paths=await window.tb.visionRuntimePaths();
  if(!paths?.ok)throw new Error(paths?.error||'Không tìm thấy MediaPipe runtime.');
  if(!runtime)runtime=await import(paths.bundleUrl);
  const vision=await runtime.FilesetResolver.forVisionTasks(paths.wasmUrl);
  const [faceRaw,poseRaw,semanticRaw]=await Promise.all([
    window.tb.visionModelBytes('face'),window.tb.visionModelBytes('pose'),window.tb.visionModelBytes('semantic')
  ]);
  const faceBytes=bytesOf(faceRaw),poseBytes=bytesOf(poseRaw),semanticBytes=bytesOf(semanticRaw);
  if(faceBytes.byteLength<1000000||poseBytes.byteLength<1000000||semanticBytes.byteLength<50000)throw new Error('Model semantic/landmark chưa tải đầy đủ.');

  faceLandmarker=await runtime.FaceLandmarker.createFromOptions(vision,{
    baseOptions:{modelAssetBuffer:faceBytes,delegate},
    runningMode:'IMAGE',numFaces:10,minFaceDetectionConfidence:.35,minFacePresenceConfidence:.35,minTrackingConfidence:.35,
    outputFaceBlendshapes:true,outputFacialTransformationMatrixes:true
  });
  poseLandmarker=await runtime.PoseLandmarker.createFromOptions(vision,{
    baseOptions:{modelAssetBuffer:poseBytes,delegate},
    runningMode:'IMAGE',numPoses:10,minPoseDetectionConfidence:.35,minPosePresenceConfidence:.35,minTrackingConfidence:.35,
    outputSegmentationMasks:false
  });
  const renderCanvas=typeof OffscreenCanvas!=='undefined'?new OffscreenCanvas(1,1):document.createElement('canvas');
  segmenter=await runtime.ImageSegmenter.createFromOptions(vision,{
    baseOptions:{modelAssetBuffer:semanticBytes,delegate},
    runningMode:'IMAGE',outputCategoryMask:true,outputConfidenceMasks:false,canvas:renderCanvas
  });
  backend=`MediaPipe ${delegate}`;
  return true;
}

export async function ensureVisionReady({download=true}={}){
  if(faceLandmarker&&poseLandmarker&&segmenter)return status();
  if(initPromise)return initPromise;
  initPromise=(async()=>{
    try{
      const s=await window.tb.modelStatus();
      if(!s?.ready){
        if(!download)throw new Error('Chưa cài bộ model Face/Pose/Semantic.');
        const prepared=await window.tb.prepareVisionModels();
        if(!prepared?.ok||!prepared.ready)throw new Error(prepared?.error||'Không thể tải model semantic.');
      }
      try{
        await createTasks('GPU');
      }catch(gpuError){
        await closeTasks();
        try{await createTasks('CPU');}
        catch(cpuError){throw new Error(`GPU: ${gpuError?.message||gpuError}; CPU: ${cpuError?.message||cpuError}`);}
      }
      lastError=null;return status();
    }catch(e){lastError=e;await closeTasks();throw e;}
    finally{initPromise=null;}
  })();
  return initPromise;
}

function copyCategoryMask(mask){
  if(!mask)return null;
  const width=mask.width,height=mask.height;
  let data=null;
  try{data=new Uint8Array(mask.getAsUint8Array());}
  catch{
    try{const f=mask.getAsFloat32Array();data=Uint8Array.from(f,v=>Math.max(0,Math.min(255,Math.round(v))));}catch{}
  }
  try{mask.close?.();}catch{}
  return data?{width,height,data}:null;
}

function simplifyFaceResult(result){
  const faces=(result?.faceLandmarks||[]).map((points,index)=>({
    index,
    landmarks:points.map(p=>({x:p.x,y:p.y,z:p.z??0,visibility:p.visibility??1})),
    blendshapes:(result?.faceBlendshapes?.[index]?.categories||[]).map(x=>({name:x.categoryName||x.displayName||'',score:x.score||0})),
    matrix:result?.facialTransformationMatrixes?.[index]?.data?Array.from(result.facialTransformationMatrixes[index].data):null
  }));
  return faces;
}
function simplifyPoseResult(result){
  return (result?.landmarks||[]).map((points,index)=>({
    index,
    landmarks:points.map(p=>({x:p.x,y:p.y,z:p.z??0,visibility:p.visibility??1,presence:p.presence??1})),
    worldLandmarks:(result?.worldLandmarks?.[index]||[]).map(p=>({x:p.x,y:p.y,z:p.z,visibility:p.visibility??1,presence:p.presence??1}))
  }));
}

export async function analyzeImage(imageSource){
  await ensureVisionReady();
  const started=performance.now();
  const [faceResult,poseResult]=await Promise.all([
    Promise.resolve().then(()=>faceLandmarker.detect(imageSource)),
    Promise.resolve().then(()=>poseLandmarker.detect(imageSource))
  ]);
  const segResult=segmenter.segment(imageSource);
  const semantic=copyCategoryMask(segResult?.categoryMask);
  try{segResult?.confidenceMasks?.forEach?.(m=>m.close?.());}catch{}
  let labels=[];try{labels=segmenter.getLabels?.()||[];}catch{}
  return{
    ok:true,backend,elapsedMs:Math.round(performance.now()-started),
    faces:simplifyFaceResult(faceResult),poses:simplifyPoseResult(poseResult),
    semantic:semantic?{...semantic,labels}:null
  };
}

export function status(){
  return{ready:!!(faceLandmarker&&poseLandmarker&&segmenter),backend,error:lastError?.message||null};
}
