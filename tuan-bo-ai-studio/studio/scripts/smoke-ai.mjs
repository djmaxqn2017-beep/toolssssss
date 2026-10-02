import path from 'node:path';
import { fileURLToPath } from 'node:url';
import ort from 'onnxruntime-node';

const here=path.dirname(fileURLToPath(import.meta.url));
const model=path.resolve(here,'..','resources','models','onnx-community','BiRefNet_lite-ONNX','onnx','model.onnx');
console.log('AI smoke: loading',model);
const session=await ort.InferenceSession.create(model,{executionProviders:['cpu'],graphOptimizationLevel:'all',intraOpNumThreads:4});
const pixels=1024*1024;
const input=new Float32Array(3*pixels);
for(let i=0;i<pixels;i++){
  const x=i%1024,y=Math.floor(i/1024),subject=x>310&&x<714&&y>130&&y<930;
  const rgb=subject?[220,170,145]:[30,35,45];
  input[i]=(rgb[0]/255-.485)/.229;
  input[pixels+i]=(rgb[1]/255-.456)/.224;
  input[pixels*2+i]=(rgb[2]/255-.406)/.225;
}
const tensor=new ort.Tensor('float32',input,[1,3,1024,1024]);
const outputs=await session.run({[session.inputNames[0]]:tensor});
const out=outputs[session.outputNames[0]];
if(!out?.data||out.data.length<1024*1024)throw new Error('BiRefNet smoke test returned invalid tensor');
console.log('AI smoke OK:',session.inputNames[0],'->',session.outputNames[0],out.dims.join('x'));
