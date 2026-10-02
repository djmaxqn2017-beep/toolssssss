const state = {
  images: [],
  selected: -1,
  view: 'after',
  copiedSettings: null,
  referenceIndex: -1,
  settings: defaultSettings()
};

const controls = [
  ['exposure','Exposure',-2,2,0.01,0],
  ['contrast','Contrast',-100,100,1,0],
  ['highlights','Highlights',-100,100,1,0],
  ['shadows','Shadows',-100,100,1,0],
  ['temperature','Temperature',-100,100,1,0],
  ['tint','Tint',-100,100,1,0],
  ['saturation','Saturation',-100,100,1,0]
];

const $ = s => document.querySelector(s);
const $$ = s => [...document.querySelectorAll(s)];
const canvas = $('#canvas');
const ctx = canvas.getContext('2d', { willReadFrequently: true });
let sourceImage = null;
let sourcePixels = null;

function defaultSettings(){
  return Object.fromEntries(controls.map(([k,,,,v]) => [k,v]));
}

function buildSliders(){
  const host = $('#sliders');
  host.innerHTML = '';
  for(const [key,label,min,max,step] of controls){
    const wrap = document.createElement('div');
    wrap.className='control';
    wrap.innerHTML=`<div class="control-head"><span>${label}</span><span id="v-${key}">0</span></div><input id="s-${key}" type="range" min="${min}" max="${max}" step="${step}" value="0">`;
    host.appendChild(wrap);
    wrap.querySelector('input').addEventListener('input', e => {
      state.settings[key]=Number(e.target.value);
      wrap.querySelector(`#v-${key}`).textContent=e.target.value;
      persistCurrentSettings();
      renderImage();
    });
  }
}

function syncSliders(){
  for(const [key] of controls){
    const el=$(`#s-${key}`); if(!el) continue;
    el.value=state.settings[key] ?? 0;
    $(`#v-${key}`).textContent=Number(state.settings[key] ?? 0).toFixed(key==='exposure'?2:0);
  }
}

async function importImages(){
  const files=await window.tb.openImages();
  for(const f of files){
    state.images.push({...f, settings:defaultSettings()});
  }
  rebuildLibrary();
  if(state.selected<0 && state.images.length) selectImage(0);
}

function rebuildLibrary(){
  const lib=$('#library'); const strip=$('#filmstrip');
  if(lib) lib.innerHTML='';
  if(strip) strip.innerHTML='';
  state.images.forEach((img,i)=>{
    if(lib){
      const row=document.createElement('div'); row.className='library-item'+(i===state.selected?' active':'');
      row.innerHTML=`<img class="thumb" src="${img.url}"><div class="lib-meta"><b>${escapeHtml(img.name)}</b><span>${i+1}/${state.images.length}</span></div>`;
      row.onclick=()=>selectImage(i); lib.appendChild(row);
    }
    if(strip){
      const th=document.createElement('img'); th.src=img.url; th.className=i===state.selected?'active':''; th.title=img.name; th.onclick=()=>selectImage(i); strip.appendChild(th);
    }
  });
}

async function selectImage(index){
  if(index<0||index>=state.images.length)return;
  state.selected=index; state.settings={...defaultSettings(),...(state.images[index].settings||{})}; syncSliders(); rebuildLibrary();
  $('#imageInfo').textContent=state.images[index].name;
  const img=new Image(); img.onload=()=>{
    sourceImage=img;
    const maxSide=2200; const scale=Math.min(1,maxSide/Math.max(img.naturalWidth,img.naturalHeight));
    canvas.width=Math.max(1,Math.round(img.naturalWidth*scale)); canvas.height=Math.max(1,Math.round(img.naturalHeight*scale));
    ctx.drawImage(img,0,0,canvas.width,canvas.height); sourcePixels=ctx.getImageData(0,0,canvas.width,canvas.height);
    canvas.style.display='block'; $('#emptyState').style.display='none'; renderImage();
  }; img.src=state.images[index].url;
}

function persistCurrentSettings(){ if(state.selected>=0) state.images[state.selected].settings={...state.settings}; }
function clamp(v){return Math.max(0,Math.min(255,v));}
function renderImage(){
  if(!sourcePixels)return;
  if(state.view==='before') { ctx.putImageData(sourcePixels,0,0); return; }
  const src=sourcePixels.data; const out=new Uint8ClampedArray(src.length); const s=state.settings;
  const exp=Math.pow(2,s.exposure||0); const contrast=(s.contrast||0)/100; const sat=1+(s.saturation||0)/100;
  const temp=(s.temperature||0)*0.45; const tint=(s.tint||0)*0.28;
  const shadow=(s.shadows||0)/100; const hi=(s.highlights||0)/100;
  for(let i=0;i<src.length;i+=4){
    let r=src[i]*exp,g=src[i+1]*exp,b=src[i+2]*exp;
    const lum=(r+g+b)/3; const shW=Math.max(0,1-lum/150); const hiW=Math.max(0,(lum-105)/150);
    const shLift=shadow*70*shW; const hiLift=hi*70*hiW;
    r+=shLift+hiLift+temp; g+=shLift+hiLift+tint; b+=shLift+hiLift-temp;
    r=(r-128)*(1+contrast)+128; g=(g-128)*(1+contrast)+128; b=(b-128)*(1+contrast)+128;
    const gray=.299*r+.587*g+.114*b; r=gray+(r-gray)*sat; g=gray+(g-gray)*sat; b=gray+(b-gray)*sat;
    out[i]=clamp(r); out[i+1]=clamp(g); out[i+2]=clamp(b); out[i+3]=src[i+3];
  }
  ctx.putImageData(new ImageData(out,sourcePixels.width,sourcePixels.height),0,0);
  if(state.view==='split'){
    ctx.save(); ctx.beginPath(); ctx.rect(0,0,canvas.width/2,canvas.height); ctx.clip(); ctx.putImageData(sourcePixels,0,0); ctx.restore();
    ctx.strokeStyle='#b66cff'; ctx.lineWidth=3; ctx.beginPath(); ctx.moveTo(canvas.width/2,0); ctx.lineTo(canvas.width/2,canvas.height); ctx.stroke();
  }
}

function calcMean(imageData){
  const d=imageData.data; let r=0,g=0,b=0,n=0;
  const step=40;
  for(let i=0;i<d.length;i+=4*step){r+=d[i];g+=d[i+1];b+=d[i+2];n++;}
  return {r:r/n,g:g/n,b:b/n,l:(r+g+b)/(3*n)};
}

async function colorMatch(){
  if(state.referenceIndex<0||state.selected<0)return alert('Hãy chọn ảnh mẫu trước.');
  const ref=state.images[state.referenceIndex];
  const img=new Image(); img.onload=()=>{
    const c=document.createElement('canvas'); const x=c.getContext('2d',{willReadFrequently:true});
    const scale=Math.min(1,800/Math.max(img.naturalWidth,img.naturalHeight)); c.width=Math.round(img.naturalWidth*scale);c.height=Math.round(img.naturalHeight*scale);x.drawImage(img,0,0,c.width,c.height);
    const refMean=calcMean(x.getImageData(0,0,c.width,c.height)); const curMean=calcMean(sourcePixels);
    const tempDelta=(refMean.r-refMean.b)-(curMean.r-curMean.b); const tintDelta=(refMean.g-(refMean.r+refMean.b)/2)-(curMean.g-(curMean.r+curMean.b)/2);
    const expDelta=Math.log2(Math.max(0.15,refMean.l)/Math.max(0.15,curMean.l));
    state.settings.exposure=Math.max(-2,Math.min(2,expDelta));
    state.settings.temperature=Math.max(-100,Math.min(100,tempDelta*1.15));
    state.settings.tint=Math.max(-100,Math.min(100,tintDelta*1.2));
    state.settings.contrast=ref.settings?.contrast||0; state.settings.saturation=ref.settings?.saturation||0;
    persistCurrentSettings();syncSliders();renderImage();
  }; img.src=ref.url;
}

async function exportCurrent(){
  if(state.selected<0)return;
  const folder=await window.tb.chooseExportFolder(); if(!folder)return;
  const old=state.view;state.view='after';renderImage();
  const base=state.images[state.selected].name.replace(/\.[^.]+$/,'');
  const result=await window.tb.writeExport({folder,filename:`${base}_TB.jpg`,dataUrl:canvas.toDataURL('image/jpeg',0.95)});
  state.view=old;renderImage();
  if(!result.ok) alert(result.error); await refreshLicense();
}

async function refreshLicense(){
  const s=await window.tb.licenseStatus();
  $('#licenseText').textContent=s.valid?(s.kind==='quota'?`Còn ${s.remaining}/${s.total} ảnh`:`Unlimited đến ${new Date(s.expiresAt).toLocaleDateString('vi-VN')}`):`${s.reason||'Chưa có license'} • Máy ${s.machineId}`;
}

function projectSnapshot(){return {version:1,images:state.images,selected:state.selected,referenceIndex:state.referenceIndex};}
function escapeHtml(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}

buildSliders();syncSliders();refreshLicense();
$('#btnOpen').onclick=importImages;$('#btnOpenCenter').onclick=importImages;$('#btnExport').onclick=exportCurrent;
$('#btnReset').onclick=()=>{state.settings=defaultSettings();persistCurrentSettings();syncSliders();renderImage();};
$('#btnCopy').onclick=()=>state.copiedSettings={...state.settings};
$('#btnPaste').onclick=()=>{if(state.copiedSettings){state.settings={...state.copiedSettings};persistCurrentSettings();syncSliders();renderImage();}};
$('#btnSetReference').onclick=()=>{if(state.selected>=0){state.referenceIndex=state.selected;$('#referenceName').textContent=state.images[state.selected].name;}};
$('#btnColorMatch').onclick=colorMatch;
$('#btnSaveProject').onclick=()=>window.tb.saveProject(projectSnapshot());
$('#btnLoadProject').onclick=async()=>{const p=await window.tb.loadProject();if(!p)return;state.images=p.images||[];state.selected=-1;state.referenceIndex=p.referenceIndex??-1;rebuildLibrary();if(state.images.length)selectImage(Math.max(0,p.selected||0));};
$('#btnPrev').onclick=()=>selectImage(Math.max(0,state.selected-1));$('#btnNext').onclick=()=>selectImage(Math.min(state.images.length-1,state.selected+1));
$('#btnPreset').onclick=()=>{localStorage.setItem('tb-preset',JSON.stringify(state.settings));alert('Đã lưu preset local.');};
$$('.segmented button').forEach(b=>b.onclick=()=>{$$('.segmented button').forEach(x=>x.classList.remove('active'));b.classList.add('active');state.view=b.dataset.view;renderImage();});
$$('.right-tabs button').forEach(b=>b.onclick=()=>{$$('.right-tabs button').forEach(x=>x.classList.remove('active'));$$('.panel').forEach(x=>x.classList.remove('active'));b.classList.add('active');document.querySelector(`[data-panel-content="${b.dataset.panel}"]`).classList.add('active');});
$('#btnLicense').onclick=async()=>{await window.tb.importLicense();await refreshLicense();};
