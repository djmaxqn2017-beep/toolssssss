const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('tbLicense', {
  keyInfo: () => ipcRenderer.invoke('license:key-info'),
  copyKeyFingerprint: () => ipcRenderer.invoke('license:copy-key-fingerprint'),
  exportPublic: () => ipcRenderer.invoke('license:export-public'),
  issue: (payload) => ipcRenderer.invoke('license:issue', payload)
});
