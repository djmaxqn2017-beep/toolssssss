const { app } = require('electron');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const os = require('os');

function userFile(name) {
  return path.join(app.getPath('userData'), name);
}

function stableMachineId() {
  const nets = os.networkInterfaces();
  const macs = [];
  Object.values(nets).flat().filter(Boolean).forEach(n => {
    if (!n.internal && n.mac && n.mac !== '00:00:00:00:00:00') macs.push(n.mac);
  });
  macs.sort();
  const raw = [os.hostname(), os.platform(), os.arch(), ...macs].join('|');
  return crypto.createHash('sha256').update(raw).digest('hex').slice(0, 24).toUpperCase();
}

function canonicalPayload(payload) {
  return JSON.stringify(payload);
}

function loadJson(file) {
  try { return JSON.parse(fs.readFileSync(file, 'utf8')); } catch { return null; }
}

function validateLicense() {
  const publicKeyPath = userFile('license-public.pem');
  const licensePath = userFile('license.tblic');
  if (!fs.existsSync(publicKeyPath)) return { valid: false, reason: 'Chưa nhập public key', machineId: stableMachineId() };
  if (!fs.existsSync(licensePath)) return { valid: false, reason: 'Chưa nhập license', machineId: stableMachineId() };

  try {
    const doc = loadJson(licensePath);
    if (!doc || !doc.payload || !doc.signature) throw new Error('License không hợp lệ');
    const publicKey = fs.readFileSync(publicKeyPath, 'utf8');
    const ok = crypto.verify(null, Buffer.from(canonicalPayload(doc.payload)), publicKey, Buffer.from(doc.signature, 'base64'));
    if (!ok) throw new Error('Chữ ký license không hợp lệ');
    if (doc.payload.machineId && doc.payload.machineId !== stableMachineId()) throw new Error('License không khớp máy này');

    const statePath = userFile('license-state.json');
    let state = loadJson(statePath) || { licenseId: doc.payload.licenseId, used: 0 };
    if (state.licenseId !== doc.payload.licenseId) state = { licenseId: doc.payload.licenseId, used: 0 };

    if (doc.payload.kind === 'timed') {
      const exp = new Date(doc.payload.expiresAt).getTime();
      if (!Number.isFinite(exp) || Date.now() > exp) throw new Error('License đã hết hạn');
      return { valid: true, kind: 'timed', expiresAt: doc.payload.expiresAt, customer: doc.payload.customer, used: state.used, machineId: stableMachineId(), licenseId: doc.payload.licenseId };
    }

    const total = Number(doc.payload.total || 0);
    const remaining = Math.max(0, total - Number(state.used || 0));
    if (remaining <= 0) throw new Error('Đã hết lượt xuất ảnh');
    return { valid: true, kind: 'quota', total, used: state.used || 0, remaining, customer: doc.payload.customer, machineId: stableMachineId(), licenseId: doc.payload.licenseId };
  } catch (e) {
    return { valid: false, reason: e.message, machineId: stableMachineId() };
  }
}

function consumeExports(count) {
  const status = validateLicense();
  if (!status.valid) return status;
  if (status.kind === 'timed') return status;
  if (count > status.remaining) return { ...status, valid: false, reason: `Không đủ lượt xuất. Còn ${status.remaining} ảnh.` };
  const statePath = userFile('license-state.json');
  const next = { licenseId: status.licenseId, used: status.used + count, updatedAt: new Date().toISOString() };
  fs.writeFileSync(statePath, JSON.stringify(next, null, 2), 'utf8');
  return validateLicense();
}

function importPublicKey(source) {
  fs.copyFileSync(source, userFile('license-public.pem'));
  return validateLicense();
}

function importLicense(source) {
  fs.copyFileSync(source, userFile('license.tblic'));
  return validateLicense();
}

module.exports = { stableMachineId, validateLicense, consumeExports, importPublicKey, importLicense };
