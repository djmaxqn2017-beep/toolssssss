const { app } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');

let segmenter = null;
let loadingPromise = null;
let lastError = null;

function modelRoot() {
  return app.isPackaged
    ? path.join(process.resourcesPath, 'models')
    : path.join(__dirname, '..', 'resources', 'models');
}

function modelDir() {
  return path.join(modelRoot(), 'onnx-community', 'BiRefNet_lite-ONNX');
}

function hasModel() {
  return fs.existsSync(path.join(modelDir(), 'onnx', 'model.onnx'));
}

async function ensureSegmenter() {
  if (segmenter) return segmenter;
  if (loadingPromise) return loadingPromise;
  loadingPromise = (async () => {
    try {
      if (!hasModel()) throw new Error('Không tìm thấy model BiRefNet offline trong bộ cài.');
      const mod = await import('@huggingface/transformers');
      const { env, pipeline } = mod;
      env.allowRemoteModels = false;
      env.allowLocalModels = true;
      env.localModelPath = modelRoot();
      env.cacheDir = path.join(app.getPath('userData'), 'ai-cache');
      segmenter = await pipeline('image-segmentation', 'onnx-community/BiRefNet_lite-ONNX', {
        device: 'cpu'
      });
      lastError = null;
      return segmenter;
    } catch (e) {
      lastError = e;
      loadingPromise = null;
      throw e;
    }
  })();
  return loadingPromise;
}

async function segmentSubject(imagePath) {
  const pipe = await ensureSegmenter();
  const result = await pipe(imagePath, { mask_threshold: 0.22 });
  const items = Array.isArray(result) ? result : [result];
  const item = items.find(x => x && x.mask) || items[0];
  if (!item || !item.mask) throw new Error('AI không tạo được mask chủ thể.');

  const tmp = path.join(os.tmpdir(), `tuanbo-mask-${Date.now()}-${Math.random().toString(16).slice(2)}.png`);
  await item.mask.save(tmp);
  const data = fs.readFileSync(tmp);
  try { fs.unlinkSync(tmp); } catch {}
  return {
    ok: true,
    width: item.mask.width,
    height: item.mask.height,
    dataUrl: `data:image/png;base64,${data.toString('base64')}`,
    label: item.label || 'subject'
  };
}

function status() {
  return {
    modelInstalled: hasModel(),
    model: 'BiRefNet_lite-ONNX',
    runtime: '@huggingface/transformers',
    ready: !!segmenter,
    loading: !!loadingPromise && !segmenter,
    error: lastError ? lastError.message : null
  };
}

module.exports = { segmentSubject, status };
