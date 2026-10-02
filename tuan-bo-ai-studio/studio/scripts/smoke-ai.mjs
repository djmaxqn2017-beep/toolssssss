import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { env, pipeline, RawImage } from '@huggingface/transformers';

const here=path.dirname(fileURLToPath(import.meta.url));
const modelRoot=path.resolve(here,'..','resources','models');
env.allowRemoteModels=false;
env.allowLocalModels=true;
env.localModelPath=modelRoot;
env.useBrowserCache=false;
console.log('AI smoke: loading local BiRefNet from',modelRoot);
const pipe=await pipeline('image-segmentation','onnx-community/BiRefNet_lite-ONNX',{device:'cpu',dtype:'fp32'});
const w=64,h=64,data=new Uint8ClampedArray(w*h*3);
for(let y=0;y<h;y++)for(let x=0;x<w;x++){const i=(y*w+x)*3;const subject=(x>18&&x<46&&y>8&&y<58);data[i]=subject?220:30;data[i+1]=subject?170:35;data[i+2]=subject?145:45;}
const image=new RawImage(data,w,h,3);
const out=await pipe(image,{mask_threshold:.2});
const items=Array.isArray(out)?out:[out];
if(!items.length||!items[0]?.mask)throw new Error('BiRefNet smoke test returned no mask');
console.log('AI smoke OK:',items[0].mask.width,'x',items[0].mask.height);
