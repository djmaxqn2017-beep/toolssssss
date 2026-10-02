const { app, BrowserWindow, dialog, ipcMain } = require('electron');
const fs = require('fs');
const path = require('path');
const license = require('./licenseService');
const ai = require('./aiService');

let win;

function createWindow() {
  win = new BrowserWindow({
    width: 1500,
    height: 920,
    minWidth: 1100,
    minHeight: 720,
    backgroundColor: '#120b19',
    title: 'Tuấn Bồ AI Studio',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  });
  win.loadFile(path.join(__dirname, 'renderer', 'index.html'));
}

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => BrowserWindow.getAllWindows().length === 0 && createWindow());
});
app.on('window-all-closed', () => process.platform !== 'darwin' && app.quit());

ipcMain.handle('files:open-images', async () => {
  const r = await dialog.showOpenDialog(win, {
    properties: ['openFile', 'multiSelections'],
    filters: [{ name: 'Images', extensions: ['jpg','jpeg','png','webp','bmp'] }]
  });
  if (r.canceled) return [];
  return r.filePaths.map(p => ({
    path: p,
    name: path.basename(p),
    url: `file://${p.replace(/\\/g,'/')}`
  }));
});

ipcMain.handle('project:save', async (_, project) => {
  const r = await dialog.showSaveDialog(win, { defaultPath: 'TuanBoProject.tbproj', filters: [{ name: 'Tuấn Bồ Project', extensions: ['tbproj'] }] });
  if (r.canceled || !r.filePath) return false;
  fs.writeFileSync(r.filePath, JSON.stringify(project, null, 2), 'utf8');
  return true;
});

ipcMain.handle('project:load', async () => {
  const r = await dialog.showOpenDialog(win, { properties: ['openFile'], filters: [{ name: 'Tuấn Bồ Project', extensions: ['tbproj'] }] });
  if (r.canceled || !r.filePaths[0]) return null;
  return JSON.parse(fs.readFileSync(r.filePaths[0], 'utf8'));
});

ipcMain.handle('export:choose-folder', async () => {
  const r = await dialog.showOpenDialog(win, { properties: ['openDirectory', 'createDirectory'] });
  return r.canceled ? null : r.filePaths[0];
});

ipcMain.handle('export:write', async (_, { folder, filename, dataUrl }) => {
  const status = license.consumeExports(1);
  if (!status.valid) return { ok: false, error: status.reason || 'License không hợp lệ', license: status };
  const match = /^data:image\/(png|jpeg);base64,(.+)$/.exec(dataUrl || '');
  if (!match) return { ok: false, error: 'Dữ liệu ảnh không hợp lệ', license: status };
  fs.mkdirSync(folder, { recursive: true });
  fs.writeFileSync(path.join(folder, filename), Buffer.from(match[2], 'base64'));
  return { ok: true, license: license.validateLicense() };
});

ipcMain.handle('ai:status', () => ai.status());
ipcMain.handle('ai:segment-subject', async (_, imagePath) => {
  try {
    return await ai.segmentSubject(imagePath);
  } catch (e) {
    return { ok: false, error: e.message || String(e) };
  }
});

ipcMain.handle('license:status', () => license.validateLicense());
ipcMain.handle('license:machine-id', () => license.stableMachineId());
ipcMain.handle('license:import-key', async () => {
  const r = await dialog.showOpenDialog(win, { properties: ['openFile'], filters: [{ name: 'Public key', extensions: ['pem'] }] });
  return r.canceled ? license.validateLicense() : license.importPublicKey(r.filePaths[0]);
});
ipcMain.handle('license:import-license', async () => {
  const r = await dialog.showOpenDialog(win, { properties: ['openFile'], filters: [{ name: 'Tuấn Bồ License', extensions: ['tblic'] }] });
  return r.canceled ? license.validateLicense() : license.importLicense(r.filePaths[0]);
});
