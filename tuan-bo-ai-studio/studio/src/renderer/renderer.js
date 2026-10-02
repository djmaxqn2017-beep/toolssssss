const state = {
  images: [],
  selected: -1,
  view: 'after',
  copiedSettings: null,
  referenceIndex: -1,
  settings: defaultSettings()
};

const colorControls = [
  ['exposure','Exposure',-2,2,0.01,0],
  ['contrast','Contrast',-100,100,1,0],
  ['highlights','Highlights',-100,100,1,0],
  ['shadows','Shadows',-100,100,1,0],
  ['temperature','Temperature',-100,100,1,0],
  ['tint','Tint',-100,100,1,0],
  ['saturation','Saturation',-100,100,1,0]
];
const portraitControls = [
  ['skinSmooth','Làm mịn giữ texture',0,100,1,0],
  ['skinEven','Đều màu da',0,100,1,0],
  ['blemish','Giảm mụn / khuyết điểm',0,100,1,0],
  ['skinBright','Sáng da',-30,60,1,0]
];
const backgroundControls = [
  ['backgroundBlur','Làm mờ nền',0,100,1,0],
  ['backgroundLight','Sáng / tối nền',-100,100,1,0],
  ['backgroundSat','Bão hòa nền',-100,100,1,0]
];
const lightingControls = [
  ['subjectLight','Sáng chủ thể',-50,100,1,0],
  ['lightWarm','Độ ấm ánh sáng',-100,100,1,0],
  ['rimLight','Viền sáng / glow',0,100,1,0],
  ['backgroundDepth','Chiều sâu hậu cảnh',0,100,1,0]
];
const allControls = [...colorControls,...portraitControls,...backgroundControls,...lightingControls];

const $ = s => document.querySelector(s);
const $$ = s => [...document.querySelectorAll(s)];
const canvas = $('#canvas');
const ctx = canvas.getContext('2d', { willReadFrequently: true });
let sourceImage = null;
let sourcePixels = null;
let subjectMaskPixels = null;
let subjectMaskCanvas = document.createElement('canvas');
let subjectAlphaCanvas = document.createElement('canvas');
let aiBusy = false;

function defaultSettings(){
  return Object.fromEntries(allControls.map(([k,,,,v]) => [k,v]));
}

function buildControlSet(hostSelector, controls){
  const host=$(hostSelector); if(!host) return;
  host.innerHTML='';
  for(const [key,label,min,max,step] of controls){
    const wrap=document.createElement('div');
    wrap.className='control';
    wrap.innerHTML=`<div class="control-head"><span>${label}</span><span id="v-${key}">0</span></div><input id="s-${key}" type="range" min="${min}" max="${max}" step="${step}" value="0">`;
    host.appendChild(wrap);
    wrap.querySelector('input').addEventListener('input',e=>{
      state.settings[key]=Number(e.target.value);
      const valueEl=$(`#v-${key}`); if(valueEl) valueEl.textContent=e.target.value;
      persistCurrentSettings();
      renderImage();
    });
  }
}

function buildSliders(){
  buildControlSet('#sliders',colorControls);
  buildControlSet('#portraitSliders',portraitControls);
  buildControlSet('#backgroundSliders',backgroundControls);
  buildControlSet('#lightingSliders',lightingControls);
}

function syncSliders(){
  for(const [key] of allControls){
    const el=$(`#s-${key}`); if(!el) continue;
    el.value=state.settings[key] ?? 0;
    const val=$(`#v-${key}`); if(val) val.textContent=Number(state.settings[key] ?? 0).toFixed(key==='exposure'?2:0);
  }
}

function setAiStatus(text,good=false){
  ['#portraitAiStatus','#backgroundAiStatus','#lightingAiStatus'].forEach(sel=>{const el=$(sel);if(el)el.textContent=text;});
  const top=$('#aiTopStatus');
  if(top){top.textContent=good?'AI Offline • Sẵn sàng':text;top.style.color=good?'#d9b8ff':'#e8b9ff';}
}

async function refreshAiStatus(){
  try{
    const s=await window.tb.aiStatus();
    if(s.modelInstalled) setAiStatus(s.ready?'AI đã tải model':'Model offline đã cài',true);
    else setAiStatus('Thiếu model AI');
  }catch{setAiStatus('AI chưa sẵn sàng');}
}

async function importImages(){
  const files=await window.tb.openImages();
  for(const f of files){state.images.push({...f,settings:defaultSettings()});}
  rebuildLibrary();
  if(state.selected<0&&state.images.length)selectImage(0);
}

function rebuildLibrary(){
  const lib=$('#library');const strip=$('#filmstrip');
  if(lib)lib.innerHTML='';if(strip)strip.innerHTML='';
  state.images.forEach((img,i)=>{
    if(strip){
      const th=document.createElement('img');th.src=img.url;th.className=i===state.selected?'active':'';th.title=img.name;th.onclick=()=>selectImage(i);strip.appendChild(th);
    }
  });
}

async function selectImage(index){
  if(index<0||index>=state.images.length)return;
  state.selected=index;
  state.settings={...defaultSettings(),...(state.images[index].settings||{})};
  subjectMaskPixels=null;
  syncSliders();rebuildLibrary();
  $('#imageInfo').textContent=state.images[index].name;
  const img=new Image();
  img.onload=()=>{
    sourceImage=img;
    const maxSide=2200;const scale=Math.min(1,maxSide/Math.max(img.naturalWidth,img.naturalHeight));
    canvas.width=Math.max(1,Math.round(img.naturalWidth*scale));canvas.height=Math.max(1,Math.round(img.naturalHeight*scale));
    ctx.drawImage(img,0,0,canvas.width,canvas.height);sourcePixels=ctx.getImageData(0,0,canvas.width,canvas.height);
    canvas.style.display='block';$('#emptyState').style.display='none';
    renderImage();
    if(state.images[index].aiMaskDataUrl) loadMaskDataUrl(state.images[index].aiMaskDataUrl);
    else setTimeout(()=>ensureAIMask(false),120);
  };
  img.src=state.images[index].url;
}

function persistCurrentSettings(){if(state.selected>=0)state.images[state.selected].settings={...state.settings};}
function clamp(v){return Math.max(0,Math.min(255,v));}
function isSkin(r,g,b){
  const cb=128-0.168736*r-0.331264*g+0.5*b;
  const cr=128+0.5*r-0.418688*g-0.081312*b;
  return r>45&&g>30&&b>20&&cb>72&&cb<142&&cr>128&&cr<190&&Math.max(r,g,b)-Math.min(r,g,b)>8;
}

function makeCanvas(w,h){const c=document.createElement('canvas');c.width=w;c.height=h;return c;}

async function loadMaskDataUrl(dataUrl){
  return new Promise((resolve,reject)=>{
    const img=new Image();
    img.onload=()=>{
      subjectMaskCanvas.width=canvas.width;subjectMaskCanvas.height=canvas.height;
      const mctx=subjectMaskCanvas.getContext('2d',{willReadFrequently:true});
      mctx.clearRect(0,0,canvas.width,canvas.height);mctx.drawImage(img,0,0,canvas.width,canvas.height);
      subjectMaskPixels=mctx.getImageData(0,0,canvas.width,canvas.height);
      subjectAlphaCanvas.width=canvas.width;subjectAlphaCanvas.height=canvas.height;
      const actx=subjectAlphaCanvas.getContext('2d');const alpha=actx.createImageData(canvas.width,canvas.height);
      const md=subjectMaskPixels.data,ad=alpha.data;
      for(let i=0;i<md.length;i+=4){const a=md[i];ad[i]=255;ad[i+1]=255;ad[i+2]=255;ad[i+3]=a;}
      actx.putImageData(alpha,0,0);setAiStatus('AI mask sẵn sàng',true);renderImage();resolve();
    };
    img.onerror=reject;img.src=dataUrl;
  });
}

async function ensureAIMask(showErrors=true){
  if(aiBusy||state.selected<0||!sourcePixels)return;
  const currentIndex=state.selected;const image=state.images[currentIndex];
  if(image.aiMaskDataUrl){await loadMaskDataUrl(image.aiMaskDataUrl);return;}
  aiBusy=true;setAiStatus('AI đang phân tích…');
  try{
    const res=await window.tb.aiSegmentSubject(image.path);
    if(currentIndex!==state.selected)return;
    if(!res||!res.ok)throw new Error(res?.error||'Không thể tạo AI mask');
    image.aiMaskDataUrl=res.dataUrl;await loadMaskDataUrl(res.dataUrl);
  }catch(e){setAiStatus('AI lỗi');if(showErrors)alert(`AI Offline: ${e.message||e}`);}
  finally{aiBusy=false;}
}

function renderImage(){
  if(!sourcePixels)return;
  if(state.view==='before'){ctx.putImageData(sourcePixels,0,0);return;}
  const src=sourcePixels.data;const out=new Uint8ClampedArray(src.length);const s=state.settings;
  const exp=Math.pow(2,s.exposure||0),contrast=(s.contrast||0)/100,sat=1+(s.saturation||0)/100;
  const temp=(s.temperature||0)*0.45,tint=(s.tint||0)*0.28,shadow=(s.shadows||0)/100,hi=(s.highlights||0)/100;
  for(let i=0;i<src.length;i+=4){
    let r=src[i]*exp,g=src[i+1]*exp,b=src[i+2]*exp;
    const lum=(r+g+b)/3,shW=Math.max(0,1-lum/150),hiW=Math.max(0,(lum-105)/150);
    const shLift=shadow*70*shW,hiLift=hi*70*hiW;
    r+=shLift+hiLift+temp;g+=shLift+hiLift+tint;b+=shLift+hiLift-temp;
    r=(r-128)*(1+contrast)+128;g=(g-128)*(1+contrast)+128;b=(b-128)*(1+contrast)+128;
    const gray=.299*r+.587*g+.114*b;r=gray+(r-gray)*sat;g=gray+(g-gray)*sat;b=gray+(b-gray)*sat;
    out[i]=clamp(r);out[i+1]=clamp(g);out[i+2]=clamp(b);out[i+3]=src[i+3];
  }

  const baseCanvas=makeCanvas(canvas.width,canvas.height),bctx=baseCanvas.getContext('2d',{willReadFrequently:true});
  bctx.putImageData(new ImageData(out,sourcePixels.width,sourcePixels.height),0,0);

  if(subjectMaskPixels&&(s.skinSmooth||s.skinEven||s.blemish||s.skinBright)){
    const blurCanvas=makeCanvas(canvas.width,canvas.height),blctx=blurCanvas.getContext('2d',{willReadFrequently:true});
    blctx.filter=`blur(${Math.max(0.5,0.8+(s.skinSmooth||0)/32)}px)`;blctx.drawImage(baseCanvas,0,0);
    const blurred=blctx.getImageData(0,0,canvas.width,canvas.height).data;
    const img=bctx.getImageData(0,0,canvas.width,canvas.height),d=img.data,m=subjectMaskPixels.data;
    const smooth=(s.skinSmooth||0)/100*.62,even=(s.skinEven||0)/100*.28,blem=(s.blemish||0)/100*.38,bright=(s.skinBright||0)/100*.18;
    for(let i=0;i<d.length;i+=4){
      if(m[i]<45||!isSkin(d[i],d[i+1],d[i+2]))continue;
      let mix=smooth;
      if(d[i]>d[i+1]*1.12&&d[i]>d[i+2]*1.16)mix=Math.min(.82,mix+blem);
      d[i]=clamp(d[i]*(1-mix)+blurred[i]*mix);d[i+1]=clamp(d[i+1]*(1-mix)+blurred[i+1]*mix);d[i+2]=clamp(d[i+2]*(1-mix)+blurred[i+2]*mix);
      const l=.299*d[i]+.587*d[i+1]+.114*d[i+2];
      d[i]=clamp(d[i]*(1-even)+l*1.06*even);d[i+1]=clamp(d[i+1]*(1-even)+l*1.00*even);d[i+2]=clamp(d[i+2]*(1-even)+l*.94*even);
      d[i]=clamp(d[i]*(1+bright));d[i+1]=clamp(d[i+1]*(1+bright));d[i+2]=clamp(d[i+2]*(1+bright));
    }
    bctx.putImageData(img,0,0);
  }

  ctx.clearRect(0,0,canvas.width,canvas.height);
  if(subjectMaskPixels){
    const bg=makeCanvas(canvas.width,canvas.height),bgx=bg.getContext('2d');
    const blurPx=Math.max(0,(s.backgroundBlur||0)/7.5),depth=(s.backgroundDepth||0)/100;
    const bgBrightness=Math.max(.2,1+(s.backgroundLight||0)/180-depth*.45),bgSat=Math.max(0,1+(s.backgroundSat||0)/100);
    bgx.filter=`blur(${blurPx}px) brightness(${bgBrightness}) saturate(${bgSat})`;bgx.drawImage(baseCanvas,0,0);
    ctx.drawImage(bg,0,0);

    if((s.rimLight||0)>0){
      const glow=makeCanvas(canvas.width,canvas.height),gx=glow.getContext('2d');
      gx.filter=`blur(${3+(s.rimLight||0)/7}px)`;gx.drawImage(subjectAlphaCanvas,0,0);
      gx.globalCompositeOperation='source-in';gx.fillStyle=`rgba(255,196,134,${Math.min(.72,(s.rimLight||0)/125)})`;gx.fillRect(0,0,canvas.width,canvas.height);
      gx.globalCompositeOperation='destination-out';gx.filter='none';gx.drawImage(subjectAlphaCanvas,0,0);
      ctx.globalCompositeOperation='screen';ctx.drawImage(glow,0,0);ctx.globalCompositeOperation='source-over';
    }

    const subject=makeCanvas(canvas.width,canvas.height),sx=subject.getContext('2d');
    const subjectBrightness=Math.max(.45,1+(s.subjectLight||0)/180),warm=(s.lightWarm||0)/100;
    sx.filter=`brightness(${subjectBrightness}) saturate(${1+Math.max(0,warm)*.12}) sepia(${Math.max(0,warm)*.16})`;
    sx.drawImage(baseCanvas,0,0);sx.filter='none';sx.globalCompositeOperation='destination-in';sx.drawImage(subjectAlphaCanvas,0,0);
    ctx.drawImage(subject,0,0);
  }else{
    ctx.drawImage(baseCanvas,0,0);
  }

  if(state.view==='split'){
    ctx.save();ctx.beginPath();ctx.rect(0,0,canvas.width/2,canvas.height);ctx.clip();ctx.putImageData(sourcePixels,0,0);ctx.restore();
    ctx.strokeStyle='#b66cff';ctx.lineWidth=3;ctx.beginPath();ctx.moveTo(canvas.width/2,0);ctx.lineTo(canvas.width/2,canvas.height);ctx.stroke();
  }
}

function calcMean(imageData){
  const d=imageData.data;let r=0,g=0,b=0,n=0;const step=40;
  for(let i=0;i<d.length;i+=4*step){r+=d[i];g+=d[i+1];b+=d[i+2];n++;}
  return{r:r/n,g:g/n,b:b/n,l:(r+g+b)/(3*n)};
}

async function colorMatch(){
  if(state.referenceIndex<0||state.selected<0)return alert('Hãy chọn ảnh mẫu trước.');
  const ref=state.images[state.referenceIndex],img=new Image();
  img.onload=()=>{
    const c=document.createElement('canvas'),x=c.getContext('2d',{willReadFrequently:true});
    const scale=Math.min(1,800/Math.max(img.naturalWidth,img.naturalHeight));c.width=Math.round(img.naturalWidth*scale);c.height=Math.round(img.naturalHeight*scale);x.drawImage(img,0,0,c.width,c.height);
    const refMean=calcMean(x.getImageData(0,0,c.width,c.height)),curMean=calcMean(sourcePixels);
    const tempDelta=(refMean.r-refMean.b)-(curMean.r-curMean.b),tintDelta=(refMean.g-(refMean.r+refMean.b)/2)-(curMean.g-(curMean.r+curMean.b)/2),expDelta=Math.log2(Math.max(.15,refMean.l)/Math.max(.15,curMean.l));
    state.settings.exposure=Math.max(-2,Math.min(2,expDelta));state.settings.temperature=Math.max(-100,Math.min(100,tempDelta*1.15));state.settings.tint=Math.max(-100,Math.min(100,tintDelta*1.2));
    state.settings.contrast=ref.settings?.contrast||0;state.settings.saturation=ref.settings?.saturation||0;persistCurrentSettings();syncSliders();renderImage();
  };img.src=ref.url;
}

async function exportCurrent(){
  if(state.selected<0)return;
  const folder=await window.tb.chooseExportFolder();if(!folder)return;
  const old=state.view;state.view='after';renderImage();const base=state.images[state.selected].name.replace(/\.[^.]+$/,'');
  const result=await window.tb.writeExport({folder,filename:`${base}_TB.jpg`,dataUrl:canvas.toDataURL('image/jpeg',.95)});state.view=old;renderImage();
  if(!result.ok)alert(result.error);await refreshLicense();
}

async function refreshLicense(){
  const s=await window.tb.licenseStatus();
  $('#licenseText').textContent=s.valid?(s.kind==='quota'?`Còn ${s.remaining}/${s.total} ảnh`:`Unlimited đến ${new Date(s.expiresAt).toLocaleDateString('vi-VN')}`):`${s.reason||'Chưa có license'} • Máy ${s.machineId}`;
}

function projectSnapshot(){
  return{version:2,images:state.images.map(({aiMaskDataUrl,...rest})=>rest),selected:state.selected,referenceIndex:state.referenceIndex};
}
function escapeHtml(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}

buildSliders();syncSliders();refreshLicense();refreshAiStatus();
$('#btnOpen').onclick=importImages;$('#btnOpenCenter').onclick=importImages;$('#btnExport').onclick=exportCurrent;
$('#btnReset').onclick=()=>{state.settings=defaultSettings();persistCurrentSettings();syncSliders();renderImage();};
$('#btnCopy').onclick=()=>state.copiedSettings={...state.settings};
$('#btnPaste').onclick=()=>{if(state.copiedSettings){state.settings={...state.copiedSettings};persistCurrentSettings();syncSliders();renderImage();}};
$('#btnSetReference').onclick=()=>{if(state.selected>=0){state.referenceIndex=state.selected;$('#referenceName').textContent=state.images[state.selected].name;}};
$('#btnColorMatch').onclick=colorMatch;
$('#btnSaveProject').onclick=()=>window.tb.saveProject(projectSnapshot());
$('#btnLoadProject').onclick=async()=>{const p=await window.tb.loadProject();if(!p)return;state.images=(p.images||[]).map(x=>({...x,settings:{...defaultSettings(),...(x.settings||{})}}));state.selected=-1;state.referenceIndex=p.referenceIndex??-1;rebuildLibrary();if(state.images.length)selectImage(Math.max(0,p.selected||0));};
$('#btnPrev').onclick=()=>selectImage(Math.max(0,state.selected-1));$('#btnNext').onclick=()=>selectImage(Math.min(state.images.length-1,state.selected+1));
$('#btnPreset').onclick=()=>{localStorage.setItem('tb-preset',JSON.stringify(state.settings));alert('Đã lưu preset local.');};
$$('.segmented button').forEach(b=>b.onclick=()=>{$$('.segmented button').forEach(x=>x.classList.remove('active'));b.classList.add('active');state.view=b.dataset.view;renderImage();});
$$('.right-tabs button').forEach(b=>b.onclick=()=>{$$('.right-tabs button').forEach(x=>x.classList.remove('active'));$$('.panel').forEach(x=>x.classList.remove('active'));b.classList.add('active');document.querySelector(`[data-panel-content="${b.dataset.panel}"]`).classList.add('active');});
['#btnAnalyzePortrait','#btnAnalyzeBackground','#btnAnalyzeLighting'].forEach(sel=>{const b=$(sel);if(b)b.onclick=()=>ensureAIMask(true);});
$('#btnLicense').onclick=async()=>{await window.tb.importLicense();await refreshLicense();};
