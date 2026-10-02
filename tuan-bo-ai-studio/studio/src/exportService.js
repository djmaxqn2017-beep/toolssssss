const fs = require('fs');
const os = require('os');
const path = require('path');
const sharp = require('sharp');
const ai = require('./aiService');

sharp.concurrency(Math.max(2,Math.min(16,(os.cpus()?.length||4)-1)));
sharp.cache({memory:256,files:20,items:100});

function clamp(v,min,max){return Math.max(min,Math.min(max,v));}
function safeName(name){return String(name||'TBRetoch.jpg').replace(/[<>:"/\\|?*]+/g,'_');}
function hasSubjectFx(s){return !!(s.backgroundBlur||s.backgroundLight||s.backgroundSat||s.backgroundTemp||s.subjectLight||s.lightWarm||s.skinSmooth||s.skinBright||s.skinEven||s.blemish||s.backgroundDepth);}
function meanHslShift(s){
  const names=['red','orange','yellow','green','cyan','blue','purple','magenta'];
  const hue=names.reduce((a,k)=>a+Number(s[`${k}Hue`]||0),0)/names.length;
  const sat=names.reduce((a,k)=>a+Number(s[`${k}Sat`]||0),0)/names.length;
  return {hue,sat};
}

function applyBase(p,s){
  const exposure=Math.pow(2,Number(s.exposure||0));
  const contrast=clamp(1+Number(s.contrast||0)/100,.05,2.5);
  const saturation=clamp(1+(Number(s.saturation||0)+Number(s.vibrance||0)*.55)/100,0,3);
  const {hue,sat}=meanHslShift(s);
  const temp=Number(s.temperature||0)/100;
  const tint=Number(s.tint||0)/100;
  const tone=(Number(s.toneMids||0)+Number(s.toneWhites||0)*.35+Number(s.toneHighlights||0)*.25+Number(s.toneShadows||0)*.25+Number(s.toneBlacks||0)*.2)/100;
  const rGain=exposure*(1+temp*.10+tint*.025);
  const gGain=exposure*(1+tint*.06);
  const bGain=exposure*(1-temp*.10-tint*.025);
  const offset=128*(1-contrast)+tone*12;
  return p
    .rotate()
    .toColourspace('srgb')
    .linear([rGain*contrast,gGain*contrast,bGain*contrast],[offset,offset,offset])
    .modulate({saturation:clamp(saturation*(1+sat/250),0,3),hue:clamp(hue,-180,180)});
}

async function compositeSubject(baseBuffer,maskBuffer,s,width,height){
  const blurPx=clamp(Number(s.backgroundBlur||0)/8,0,18);
  const depth=Number(s.backgroundDepth||0)/100;
  const bgBrightness=clamp(1+Number(s.backgroundLight||0)/180-depth*.40,.2,2.2);
  const bgSat=clamp(1+Number(s.backgroundSat||0)/100,0,2.4);
  const bgHue=Number(s.backgroundTemp||0)*.12;
  let bg=sharp(baseBuffer).modulate({brightness:bgBrightness,saturation:bgSat,hue:bgHue});
  if(blurPx>.2) bg=bg.blur(blurPx);
  const bgBuffer=await bg.png().toBuffer();

  let subject=sharp(baseBuffer).modulate({
    brightness:clamp(1+Number(s.subjectLight||0)/180+Number(s.skinBright||0)/500,.45,2),
    saturation:clamp(1+Number(s.lightWarm||0)/700,0,2)
  });
  const smooth=Number(s.skinSmooth||0);
  if(smooth>0) subject=subject.blur(clamp(.3+smooth/90,.3,1.7)).sharpen({sigma:1,m1:clamp(1.0-smooth/250,.55,1)});
  let subjectBuffer=await subject.removeAlpha().png().toBuffer();

  const alpha=await sharp(maskBuffer).resize(width,height,{fit:'fill'}).greyscale().png().toBuffer();
  subjectBuffer=await sharp(subjectBuffer).removeAlpha().joinChannel(alpha).png().toBuffer();
  return sharp(bgBuffer).composite([{input:subjectBuffer,blend:'over'}]);
}

async function exportImage({sourcePath,folder,filename,settings={},format='jpeg',quality=98}){
  if(!sourcePath||!fs.existsSync(sourcePath)) throw new Error('Không tìm thấy ảnh nguồn.');
  fs.mkdirSync(folder,{recursive:true});
  const outPath=path.join(folder,safeName(filename));
  const meta=await sharp(sourcePath,{failOn:'none'}).metadata();
  const swap=[5,6,7,8].includes(Number(meta.orientation||1));
  const width=swap?meta.height:meta.width;
  const height=swap?meta.width:meta.height;
  let pipeline=applyBase(sharp(sourcePath,{failOn:'none'}),settings);
  const baseBuffer=await pipeline.png().toBuffer();
  if(hasSubjectFx(settings)){
    try{
      const mask=await ai.segmentSubjectBuffer(sourcePath);
      pipeline=await compositeSubject(baseBuffer,mask.buffer,settings,width,height);
    }catch{
      pipeline=sharp(baseBuffer);
    }
  }else pipeline=sharp(baseBuffer);

  pipeline=pipeline.withMetadata();
  const q=clamp(Number(quality||98),70,100);
  if(format==='png') pipeline= pipeline.png({compressionLevel:5,adaptiveFiltering:true});
  else if(format==='webp') pipeline= pipeline.webp({quality:q,smartSubsample:true});
  else pipeline= pipeline.jpeg({quality:q,chromaSubsampling:'4:4:4',mozjpeg:true,trellisQuantisation:true,overshootDeringing:true,optimizeScans:true});
  await pipeline.toFile(outPath);
  const st=fs.statSync(outPath);
  return {ok:true,filePath:outPath,size:st.size,width,height,format};
}

module.exports={exportImage};
