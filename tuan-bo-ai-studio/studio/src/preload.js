const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('tb', {
  openImages: () => ipcRenderer.invoke('files:open-images'),
  saveProject: (project) => ipcRenderer.invoke('project:save', project),
  loadProject: () => ipcRenderer.invoke('project:load'),
  chooseExportFolder: () => ipcRenderer.invoke('export:choose-folder'),
  writeExportBuffer: (payload) => ipcRenderer.invoke('export:write-buffer', payload),
  exportFull: (payload) => ipcRenderer.invoke('export:full', payload),
  performanceInfo: () => ipcRenderer.invoke('system:performance'),
  processingDiagnostics: () => ipcRenderer.invoke('processing:diagnostics'),
  aiStatus: () => ipcRenderer.invoke('ai:status'),
  aiWarmup: () => ipcRenderer.invoke('ai:warmup'),
  aiSegmentSubject: (imagePath) => ipcRenderer.invoke('ai:segment-subject', imagePath),
  modelStatus: () => ipcRenderer.invoke('models:status'),
  prepareVisionModels: () => ipcRenderer.invoke('models:prepare'),
  visionRuntimePaths: () => ipcRenderer.invoke('vision:runtime-paths')
});
