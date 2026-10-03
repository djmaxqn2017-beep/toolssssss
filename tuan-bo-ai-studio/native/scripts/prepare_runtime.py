"""Fetch pinned native runtime/models. Python is used only on the build runner."""
import concurrent.futures, hashlib, os, pathlib, shutil, subprocess, sys, urllib.request, zipfile
root=pathlib.Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=True)
files={
 'opencv.exe':('https://github.com/opencv/opencv/releases/download/4.11.0/opencv-4.11.0-windows.exe','7c9d1c0b70db1b1952cc815252fced9a07f51267563cf3eaa1674d734c49b8e4'),
 'ort.nupkg':('https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime.directml/1.23.0/microsoft.ml.onnxruntime.directml.1.23.0.nupkg','a33ec2382b3c440bab74042a135733bb6e5085f293b908d3997688a58fe307e7'),
 'directml.nupkg':('https://api.nuget.org/v3-flatcontainer/microsoft.ai.directml/1.15.4/microsoft.ai.directml.1.15.4.nupkg','4e7cb7ddce8cf837a7a75dc029209b520ca0101470fcdf275c1f49736a3615b9'),
 'exiftool.zip':('https://downloads.sourceforge.net/project/exiftool/exiftool-13.59_64.zip','44b512b25af500724ba579d0a53c8fc5851628b692dd5e5d94ae4a15c2cba9ec'),
 'models/blazeface.onnx':('https://github.com/yakhyo/mediapipe-face-mesh-onnx/releases/download/weights/face_detection_short_range.onnx','2f2689b040becf555706d2cb978d2f0e3296ea82413734fba9a856c66c5f2b17'),
 'models/face_landmarker.onnx':('https://github.com/yakhyo/mediapipe-face-mesh-onnx/releases/download/weights/face_landmarker_Nx3x256x256.onnx','111795f8703cdeb6d0c68a9f3cc966a0f23f8786bb00f4577a11f461fc4276ac'),
 'selfie.tflite':('https://storage.googleapis.com/mediapipe-models/image_segmenter/selfie_multiclass_256x256/float32/1/selfie_multiclass_256x256.tflite','c6748b1253a99067ef71f7e26ca71096cd449baefa8f101900ea23016507e0e0'),
 'models/depth.onnx':('https://huggingface.co/onnx-community/depth-anything-v2-small/resolve/4472b7362082ad9968fee890ca0f1e5aca36b93d/onnx/model_quantized.onnx','fcf51f1b230362b28690bb9d1809bf0431f29cad20534e3f589bd7285547f20d'),
 'models/sky.onnx':('https://huggingface.co/JianyuanWang/skyseg/resolve/3ba8c6df1d9ba9ff26f637c7ba9568ac11a9aa7f/skyseg.onnx','ab9c34c64c3d821220a2886a4a06da4642ffa14d5b30e8d5339056a089aa1d39'),
}
def fetch(item):
 name,(url,checksum)=item;p=root/name;p.parent.mkdir(parents=True,exist_ok=True)
 if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest()!=checksum:
  with urllib.request.urlopen(url,timeout=90) as r,p.open('wb') as f:
   while b:=r.read(1024*1024):f.write(b)
 if hashlib.sha256(p.read_bytes()).hexdigest()!=checksum:raise RuntimeError('Checksum mismatch: '+name)
 print('Verified '+name,flush=True)
with concurrent.futures.ThreadPoolExecutor(4) as pool:list(pool.map(fetch,files.items()))
subprocess.run(['7z','x','-y',str(root/'opencv.exe'),'-o'+str(root)],check=True,stdout=subprocess.DEVNULL)
for name in ['ort','directml','exiftool']:
 with zipfile.ZipFile(root/(name+('.zip' if name=='exiftool' else '.nupkg'))) as z:z.extractall(root/name)
env=dict(os.environ,OMP_NUM_THREADS='2',TF_NUM_INTRAOP_THREADS='2',TF_NUM_INTEROP_THREADS='2')
subprocess.run([sys.executable,'-m','tf2onnx.convert','--tflite',str(root/'selfie.tflite'),'--output',str(root/'models/selfie_multiclass.onnx'),'--opset','17'],env=env,check=True)
# Numerical conversion acceptance, against the original Google TFLite graph.
import numpy as np, tensorflow as tf, onnxruntime as ort
original=tf.lite.Interpreter(model_path=str(root/'selfie.tflite'),num_threads=2);original.allocate_tensors()
session=ort.InferenceSession(str(root/'models/selfie_multiclass.onnx'),providers=['CPUExecutionProvider'])
input=np.random.default_rng(42).uniform(-1,1,(1,256,256,3)).astype('float32')
original.set_tensor(original.get_input_details()[0]['index'],input);original.invoke()
expected=original.get_tensor(original.get_output_details()[0]['index']);actual=session.run(None,{session.get_inputs()[0].name:input})[0]
assert np.max(np.abs(expected-actual))<.001,'Converted segmentation graph differs from TFLite'
manifest={name:{'url':url,'sha256':sha} for name,(url,sha) in files.items() if name.startswith('models/') or name=='selfie.tflite'}
import json
(root/'models/manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf8')
