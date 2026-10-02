const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('tb', {
  openImages: () => ipcRenderer.invoke('files:open-images'),
  saveProject: (project) => ipcRenderer.invoke('project:save', project),
  loadProject: () => ipcRenderer.invoke('project:load'),
  chooseExportFolder: () => ipcRenderer.invoke('export:choose-folder'),
  writeExport: (payload) => ipcRenderer.invoke('export:write', payload),
  aiStatus: () => ipcRenderer.invoke('ai:status'),
  aiSegmentSubject: (imagePath) => ipcRenderer.invoke('ai:segment-subject', imagePath),
  licenseStatus: () => ipcRenderer.invoke('license:status'),
  machineId: () => ipcRenderer.invoke('license:machine-id'),
  importPublicKey: () => ipcRenderer.invoke('license:import-key'),
  importLicense: () => ipcRenderer.invoke('license:import-license')
});
