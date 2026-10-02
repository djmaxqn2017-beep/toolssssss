const { app } = require('electron');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const os = require('os');

function userFile(name){ return path.join(app.getPath('userData'),name); }
function canonicalPayload(payload){ return JSON.stringify(payload); }
function loadJson(file){ try{return JSON.parse(fs.readFileSync(file,'utf8'));}catch{return null;} }
function normalizePem(pem){ return String(pem||'').trim()+'\n'; }
function fingerprintPem(pem){ return crypto.createHash('sha256').update(normalizePem(pem)).digest('hex').slice(0,16).toUpperCase(); }

function stableMachineId(){
  const nets=os.networkInterfaces();const macs=[];
  Object.values(nets).flat().filter(Boolean).forEach(n=>{if(!n.internal&&n.mac&&n.mac!=='00:00:00:00:00:00')macs.push(n.mac.toUpperCase());});
  macs.sort();
  const raw=[os.hostname().toUpperCase(),os.platform(),os.arch(),...macs].join('|');
  return crypto.createHash('sha256').update(raw).digest('hex').slice(0,24).toUpperCase();
}

function trustedKeyPath(){ return userFile('trusted-license-signer.pem'); }
function legacyKeyPath(){ return userFile('license-public.pem'); }
function licensePath(){ return userFile('license.tblic'); }
function statePath(){ return userFile('license-state.json'); }

function getVerificationKey(doc){
  if(fs.existsSync(trustedKeyPath())) return fs.readFileSync(trustedKeyPath(),'utf8');
  if(fs.existsSync(legacyKeyPath())) return fs.readFileSync(legacyKeyPath(),'utf8');
  if(doc&&doc.signerPublicKey) return doc.signerPublicKey;
  return null;
}

function verifyDoc(doc,key){
  if(!doc||!doc.payload||!doc.signature) throw new Error('License không hợp lệ');
  if(!key) throw new Error('License thiếu khóa ký');
  const ok=crypto.verify(null,Buffer.from(canonicalPayload(doc.payload)),key,Buffer.from(doc.signature,'base64'));
  if(!ok) throw new Error('Chữ ký license không hợp lệ');
  return true;
}

function validateLicense(){
  const machineId=stableMachineId();
  if(!fs.existsSync(licensePath())) return {valid:false,reason:'Chưa nhập license',machineId};
  try{
    const doc=loadJson(licensePath());
    const key=getVerificationKey(doc);
    verifyDoc(doc,key);
    if(fs.existsSync(trustedKeyPath())&&doc.signerPublicKey&&fingerprintPem(doc.signerPublicKey)!==fingerprintPem(fs.readFileSync(trustedKeyPath(),'utf8'))){
      throw new Error('License được ký bởi License Center khác');
    }
    const licMachine=String(doc.payload.machineId||'').trim().toUpperCase();
    if(licMachine&&licMachine!==machineId) throw new Error(`License không khớp máy này (${machineId})`);
    if(doc.payload.product&&doc.payload.product!=='TuanBo-AI-Studio') throw new Error('License không đúng sản phẩm');

    let state=loadJson(statePath())||{licenseId:doc.payload.licenseId,used:0};
    if(state.licenseId!==doc.payload.licenseId) state={licenseId:doc.payload.licenseId,used:0};
    const used=Math.max(0,Math.floor(Number(state.used||0)));

    if(doc.payload.kind==='timed'){
      const exp=new Date(doc.payload.expiresAt).getTime();
      if(!Number.isFinite(exp)||Date.now()>exp) throw new Error('License đã hết hạn');
      return {valid:true,kind:'timed',expiresAt:doc.payload.expiresAt,customer:doc.payload.customer,used,machineId,licenseId:doc.payload.licenseId,signerFingerprint:fingerprintPem(key)};
    }
    const total=Math.max(0,Math.floor(Number(doc.payload.total||0)));
    const remaining=Math.max(0,total-used);
    if(remaining<=0) throw new Error('Đã hết lượt xuất ảnh');
    return {valid:true,kind:'quota',total,used,remaining,customer:doc.payload.customer,machineId,licenseId:doc.payload.licenseId,signerFingerprint:fingerprintPem(key)};
  }catch(e){return {valid:false,reason:e.message||String(e),machineId};}
}

function consumeExports(count){
  const n=Math.max(0,Math.floor(Number(count||0)));const status=validateLicense();
  if(!status.valid)return status;if(status.kind==='timed')return status;
  if(n>status.remaining)return{...status,valid:false,reason:`Không đủ lượt xuất. Còn ${status.remaining} ảnh.`};
  const next={licenseId:status.licenseId,used:status.used+n,updatedAt:new Date().toISOString()};
  fs.writeFileSync(statePath(),JSON.stringify(next,null,2),'utf8');return validateLicense();
}

function importPublicKey(source){
  const pem=fs.readFileSync(source,'utf8');
  if(!pem.includes('PUBLIC KEY')) throw new Error('Public Key không hợp lệ');
  fs.writeFileSync(trustedKeyPath(),normalizePem(pem),'utf8');
  fs.writeFileSync(legacyKeyPath(),normalizePem(pem),'utf8');
  return validateLicense();
}

function importLicense(source){
  try{
    const doc=JSON.parse(fs.readFileSync(source,'utf8'));
    if(!doc||!doc.payload||!doc.signature) return {valid:false,reason:'File .tblic không hợp lệ',machineId:stableMachineId()};
    let key=null;
    if(fs.existsSync(trustedKeyPath())){
      key=fs.readFileSync(trustedKeyPath(),'utf8');
      if(doc.signerPublicKey&&fingerprintPem(doc.signerPublicKey)!==fingerprintPem(key)) return {valid:false,reason:'License được ký bởi License Center khác',machineId:stableMachineId()};
    }else if(doc.signerPublicKey){
      key=normalizePem(doc.signerPublicKey);
      verifyDoc(doc,key);
      fs.writeFileSync(trustedKeyPath(),key,'utf8');
    }else if(fs.existsSync(legacyKeyPath())) key=fs.readFileSync(legacyKeyPath(),'utf8');
    else return {valid:false,reason:'License cũ cần Public Key. Hãy tạo license mới bằng License Center V0.3.',machineId:stableMachineId()};
    verifyDoc(doc,key);
    fs.copyFileSync(source,licensePath());
    const st=validateLicense();
    if(!st.valid){try{fs.unlinkSync(licensePath());}catch{};return st;}
    return st;
  }catch(e){return {valid:false,reason:e.message||String(e),machineId:stableMachineId()};}
}

function resetTrust(){
  try{fs.unlinkSync(trustedKeyPath());}catch{}
  try{fs.unlinkSync(legacyKeyPath());}catch{}
  try{fs.unlinkSync(licensePath());}catch{}
  return {ok:true,machineId:stableMachineId()};
}

module.exports={stableMachineId,validateLicense,consumeExports,importPublicKey,importLicense,resetTrust};
