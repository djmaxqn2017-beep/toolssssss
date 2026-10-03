#include <QtTest>
#include <cstdio>
#include <cmath>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QSignalSpy>
#include <QFile>
#include <QJSEngine>
#include <QQmlEngine>
#include <QImageWriter>
#include <QProcess>
#include <QColorSpace>
#include <QPainter>
#include "SemanticEngine.h"
#include "ImageDecoder.h"
#include "AdvancedRecipe.h"
#include "ExportMetadata.h"
#include "AppController.h"

class ControllerTests : public QObject {
    Q_OBJECT
private slots:
    void masterExportAndSixteenBitMetadata() {
        std::fprintf(stderr,"TEST masterExportAndSixteenBitMetadata: enter\n");
        QTemporaryDir dir;
        QImage source(256,128,QImage::Format_RGBA64);
        source.setColorSpace(QColorSpace(QColorSpace::AdobeRgb));
        for(int y=0;y<source.height();++y){auto row=reinterpret_cast<QRgba64*>(source.scanLine(y));for(int x=0;x<source.width();++x)row[x]=QRgba64::fromRgba64(12000+x,22000+x*2,32000+y,65535);}
        const QString input=dir.filePath(QString::fromUtf8("Ảnh gốc.png"));QVERIFY(source.save(input));
        QString metadataError;QByteArray metadataOutput;QString helper=qEnvironmentVariable("TBRETOCH_EXIFTOOL_PATH");
        QVERIFY2(!helper.isEmpty(),"Metadata runtime must be configured in CI");
        QVERIFY2(runMetadataTool(helper,{"-Artist=TB Test","-Copyright=Original photographer","-overwrite_original",input},&metadataOutput,&metadataError),qPrintable(metadataError));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});QSignalSpy errors(&c,&AppController::errorOccurred);QSignalSpy exports(&c,&AppController::exportFinished);
        c.importFiles({input});QTRY_VERIFY(!c.busy());c.exportCurrent(QUrl::fromLocalFile(dir.path()),"master",100);QTRY_VERIFY_WITH_TIMEOUT(exports.count()==1||!c.busy(),30000);QVERIFY2(exports.count()==1,errors.isEmpty()?"No exported file":qPrintable(errors.last()[0].toString()));
        QFile original(input),exact(exports[0][0].toString());QVERIFY(original.open(QIODevice::ReadOnly));QVERIFY(exact.open(QIODevice::ReadOnly));QCOMPARE(exact.readAll(),original.readAll());
        exports.clear();c.setSetting("exposure",.1);c.endSettingEdit();
        for(const QString &format:{QString("png"),QString("tiff")}){
            c.exportCurrent(QUrl::fromLocalFile(dir.path()),format,100);QTRY_VERIFY_WITH_TIMEOUT(exports.count()==1||!c.busy(),30000);QVERIFY2(exports.count()==1,errors.isEmpty()?"No exported file":qPrintable(errors.last()[0].toString()));
            QString path=exports[0][0].toString();if(format=="png")QVERIFY(QFileInfo(path).size()<source.sizeInBytes()/2);QImage result(path);QCOMPARE(result.size(),source.size());QCOMPARE(result.depth(),64);
            auto row=reinterpret_cast<const QRgba64*>(result.constScanLine(20));QVERIFY(row[21].red()!=row[20].red());QCOMPARE(result.colorSpace().iccProfile(),source.colorSpace().iccProfile());
            QVERIFY2(runMetadataTool(helper,{"-Artist","-Copyright","-Orientation#",path},&metadataOutput,&metadataError),qPrintable(metadataError));const auto text=metadataOutput;QVERIFY2(text.contains("TB Test"),text.constData());QVERIFY2(text.contains("Original photographer"),text.constData());QVERIFY2(text.contains("1"),text.constData());exports.clear();
        }
        QCOMPARE(errors.count(),0);
    }
    void semanticPortraitAndTargetedTools() {
        std::fprintf(stderr,"TEST semanticPortraitAndTargetedTools: enter\n");
        const QString fixture=qEnvironmentVariable("TBRETOCH_PORTRAIT_FIXTURE");if(fixture.isEmpty())QSKIP("Real portrait fixture not configured");
        QTemporaryDir dir;QImage source(fixture);QVERIFY(!source.isNull());source=source.scaled(640,640,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        auto analysis=analysePortrait(source,dir.filePath("analysis"),true,true);QVERIFY(analysis);QVERIFY(analysis->faces.size()>0);QCOMPARE(analysis->faces[0].landmarks.size(),478);
        auto defaults=semanticDefaults();QCOMPARE(applyPortraitRecipe(source,defaults,*analysis),source);
        for(const QString &key:{QString("skinSoftening"),QString("textureRecovery"),QString("faceShine"),QString("skinUnify"),QString("eyeBags"),QString("darkCircles"),QString("faceWidth"),QString("jaw"),QString("chin"),QString("vShape"),QString("eyeSize"),QString("noseWidth"),QString("lipSize"),QString("doubleChin"),QString("iris"),QString("eyeWhites"),QString("catchlight"),QString("teethWhitening"),QString("lipstick"),QString("blush"),QString("eyeliner"),QString("eyeshadow"),QString("eyebrow"),QString("hairSmooth"),QString("hairShine"),QString("bgBlur"),QString("bgCleanup"),QString("lensBlur")}) {
            auto settings=defaults;settings[key]=70;auto result=applyPortraitRecipe(source,settings,*analysis);QCOMPARE(result.size(),source.size());QVERIFY2(result!=source.convertToFormat(QImage::Format_RGBA64),qPrintable("No effect: "+key));
        }
        auto settings=defaults;settings["mask_hair_exposure"]=1.;auto result=applyPortraitRecipe(source,settings,*analysis);QVERIFY(result!=source.convertToFormat(QImage::Format_RGBA64));
        QImage asset(source.size(),QImage::Format_RGB32);asset.fill(QColor(20,50,180));QString file=dir.filePath("replacement.png");QVERIFY(asset.save(file));
        for(const QString &key:{QString("sky"),QString("background")}) {auto recipe=defaults;recipe[key+"File"]=file;recipe[key=="sky"?"skyReplacement":"backgroundReplacement"]=100;QVERIFY(applyPortraitRecipe(source,recipe,*analysis)!=source.convertToFormat(QImage::Format_RGBA64));}
        // Synthetic local defects inside the real clothing segmentation, with a
        // meaningful repair target rather than expecting clean fabric to change.
        QImage cloth=semanticMask(*analysis,"clothes",source.size());QPoint center(-1,-1);
        for(int y=source.height()/2;y<source.height()-10&&center.x()<0;++y)for(int x=20;x<source.width()-20;++x)if(qGray(cloth.pixel(x,y))>220&&qGray(cloth.pixel(x+8,y+8))>220){center={x+4,y+4};break;}
        QVERIFY(center.x()>0);
        for(const QString &key:{QString("wrinkleRemoval"),QString("lintRemoval"),QString("stainRemoval")}) {QImage defect=source.convertToFormat(QImage::Format_RGBA64);QPainter painter(&defect);painter.setPen(QPen(key=="lintRemoval"?Qt::white:Qt::black,2));painter.drawLine(center-QPoint(4,0),center+QPoint(4,0));if(key=="stainRemoval"){painter.setBrush(QColor(180,20,30));painter.drawEllipse(center,4,4);}painter.end();auto recipe=defaults;recipe[key]=100;QVERIFY2(applyPortraitRecipe(defect,recipe,*analysis)!=defect,qPrintable(key));}
        QImage texture(4096,256,QImage::Format_RGBA64);
        for(int y=0;y<texture.height();++y){auto row=reinterpret_cast<QRgba64*>(texture.scanLine(y));for(int x=0;x<texture.width();++x){quint16 value=qRound((.5+.04*std::sin(x*2*3.141592653589793/40))*65535);row[x]=QRgba64::fromRgba64(value,value,value,65535);}}
        auto region=*analysis;QImage full(texture.size(),QImage::Format_Grayscale8);full.fill(255);region.masks["faceSkin"]=full;
        auto soft=defaults;soft["skinSoftening"]=100;auto softened=applyPortraitRecipe(texture,soft,region);auto regionMask=semanticMask(region,"faceSkin",texture.size(),0);
        double beforeGradient=0,afterGradient=0;
        for(int y=1;y<texture.height()-1;++y){auto originalRow=reinterpret_cast<const QRgba64*>(texture.constScanLine(y));auto editedRow=reinterpret_cast<const QRgba64*>(softened.constScanLine(y));for(int x=1;x<texture.width()-1;++x)if(qGray(regionMask.pixel(x,y))>250){beforeGradient+=qAbs(int(originalRow[x].red())-originalRow[x-1].red());afterGradient+=qAbs(int(editedRow[x].red())-editedRow[x-1].red());}}
        QVERIFY(beforeGradient>0);QVERIFY(afterGradient<beforeGradient*.98);
        QImage group(source.width()*2,source.height(),QImage::Format_RGBA64);QPainter groupPainter(&group);groupPainter.drawImage(0,0,source);groupPainter.drawImage(source.width(),0,source);groupPainter.end();
        auto people=analysePortrait(group,dir.filePath("group"));QCOMPARE(people->faces.size(),2);
        auto faceRecipe=defaults;faceRecipe["face_0_mask_faceSkin_exposure"]=1.;auto isolated=applyPortraitRecipe(group,faceRecipe,*people);
        QVERIFY(isolated.copy(0,0,source.width(),source.height())!=group.copy(0,0,source.width(),source.height()));
        QCOMPARE(isolated.copy(source.width(),0,source.width(),source.height()),group.copy(source.width(),0,source.width(),source.height()));
        QImage blank(256,256,QImage::Format_RGB32);blank.fill(QColor(90,100,110));auto empty=analysePortrait(blank,dir.filePath("blank"));QCOMPARE(empty->faces.size(),0);
    }
    void codecAndUnicodeImport() {
        std::fprintf(stderr,"TEST codecAndUnicodeImport: enter\n");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString folder = dir.filePath(QString::fromUtf8("Ảnh cưới # 100%"));
        QVERIFY(QDir().mkpath(folder));
        QImage image(120, 80, QImage::Format_ARGB32);
        image.fill(qRgba(60, 80, 100, 128));
        QVariantList urls;
        for (const QByteArray &format : {QByteArray("jpg"), QByteArray("png"), QByteArray("webp"), QByteArray("tiff")}) {
            const QString path = QDir(folder).filePath(QString::fromUtf8("Cô dâu 01.") + QString::fromLatin1(format));
            QVERIFY2(image.save(path, format.constData()), format.constData());
            urls.push_back(QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded));
        }
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        QSignalSpy errors(&c, &AppController::errorOccurred);
        // Exercise the same JavaScript-array -> invokable boundary as QML.
        QJSEngine engine;
        QQmlEngine::setObjectOwnership(&c, QQmlEngine::CppOwnership);
        engine.globalObject().setProperty("controller", engine.newQObject(&c));
        QJSValue selection = engine.newArray(urls.size());
        for (int i = 0; i < urls.size(); ++i) selection.setProperty(i, urls[i].toString());
        engine.globalObject().setProperty("selection", selection);
        const QJSValue result = engine.evaluate("controller.importFiles(selection)");
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.images().size() == 4, qPrintable(c.importDetails()));
        QCOMPARE(errors.count(), 0);
        QVERIFY(c.importDetails().isEmpty());
        c.selectImage(1);
        const QImage preview(QUrl(c.currentPreviewUrl()).toLocalFile());
        QCOMPARE(preview.pixelColor(10, 10).alpha(), 128);
    }

    void invalidAndMixedSelection() {
        std::fprintf(stderr,"TEST invalidAndMixedSelection: enter\n");
        QTemporaryDir dir;
        const QString valid = dir.filePath("valid.png");
        const QString invalid = dir.filePath("broken.jpg");
        QImage image(32, 32, QImage::Format_RGB32);
        image.fill(Qt::gray);
        QVERIFY(image.save(valid));
        QFile broken(invalid);
        QVERIFY(broken.open(QIODevice::WriteOnly));
        broken.write("not an image");
        broken.close();
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        QSignalSpy errors(&c, &AppController::errorOccurred);
        c.importFiles({QUrl::fromLocalFile(invalid), QUrl::fromLocalFile(valid), dir.filePath("missing.png")});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QCOMPARE(c.images().size(), 1);
        QCOMPARE(c.currentIndex(), 0);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.first().first().toString(), QString("error.partialImport"));
        QVERIFY(c.importDetails().contains("[decode]"));
        QVERIFY(c.importDetails().contains("[path]"));
    }

    void heicDecodeAndExport() {
        std::fprintf(stderr,"TEST heicDecodeAndExport: enter\n");
        const QString fixture = qEnvironmentVariable("TBRETOCH_HEIC_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set TBRETOCH_HEIC_FIXTURE to the upstream HEIC fixture");
        QTemporaryDir dir;
        const QString path = dir.filePath(QString::fromUtf8("Cô dâu.heic"));
        QVERIFY(QFile::copy(fixture, path));
        QString error;
        const QImage original = decodeImage(path, 0, &error);
        QVERIFY2(!original.isNull(), qPrintable(error));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        c.importFiles({QUrl::fromLocalFile(path)});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.images().size() == 1, qPrintable(c.importDetails()));
        QSignalSpy exported(&c, &AppController::exportFinished);
        c.exportCurrent(QUrl::fromLocalFile(dir.path()), "png", 98);
        QTRY_VERIFY_WITH_TIMEOUT(exported.count()==1||!c.busy(),30000);QCOMPARE(exported.count(),1);
        const QImage output(exported.first()[0].toString());
        QCOMPARE(output.size(), original.size());
    }

    void blockedCacheFallsBack() {
        std::fprintf(stderr,"TEST blockedCacheFallsBack: enter\n");
        QTemporaryDir dir;
        const QString path = dir.filePath("source.png");
        QImage image(32, 32, QImage::Format_RGB32);
        image.fill(Qt::gray);
        QVERIFY(image.save(path));
        QFile blocker(dir.filePath("not-a-directory"));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        const QByteArray previous = qgetenv("TBRETOCH_CACHE_ROOT");
        qputenv("TBRETOCH_CACHE_ROOT", blocker.fileName().toUtf8());
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        c.importFiles({QUrl::fromLocalFile(path)});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        qputenv("TBRETOCH_CACHE_ROOT", previous);
        QVERIFY2(c.images().size() == 1, qPrintable(c.importDetails()));
        QVERIFY(!QImage(QUrl(c.currentPreviewUrl()).toLocalFile()).isNull());
    }

    void rawDecodeAndExport() {
        std::fprintf(stderr,"TEST rawDecodeAndExport: enter\n");
        QTemporaryDir dir;
        const QString path = dir.filePath(QString::fromUtf8("Ảnh gốc.dng"));
        QVERIFY(QFile::copy(QStringLiteral(":/fixtures/sample.dng"), path));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        c.importFiles({path});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.images().size() == 1, qPrintable(c.importDetails()));
        QSignalSpy exported(&c, &AppController::exportFinished);
        c.exportCurrent(QUrl::fromLocalFile(dir.path()), "png", 98);
        QTRY_VERIFY_WITH_TIMEOUT(exported.count()==1||!c.busy(),30000);QCOMPARE(exported.count(),1);
        const QImage output(exported.first()[0].toString());
        QVERIFY(!output.isNull());
        QVERIFY(output.width() >= 100);
        QVERIFY(output.height() >= 80);
    }

    void geometryAndColorEffects() {
        std::fprintf(stderr,"TEST geometryAndColorEffects: enter\n");
        QImage source(100,80,QImage::Format_ARGB32);
        source.fill(qRgba(255,0,0,128));
        source.setPixelColor(90,70,QColor(0,0,255,128));
        QVariantMap settings = advancedDefaults();
        QCOMPARE(applyAdvancedRecipe(source,settings),source);
        settings["cropLeft"] = 10;
        settings["cropRight"] = 20;
        settings["cropTop"] = 10;
        settings["rotation"] = 1;
        const QImage crop = applyAdvancedRecipe(source,settings);
        QCOMPARE(crop.size(),QSize(72,70));
        QCOMPARE(crop.pixelColor(10,10).alpha(),128);
        settings = advancedDefaults();
        settings["hsl_redHue"] = 100;
        const QImage hue = applyAdvancedRecipe(source,settings);
        QVERIFY(hue.pixelColor(20,20).green() > 80);
        QCOMPARE(hue.pixelColor(90,70),source.pixelColor(90,70));
        settings = advancedDefaults();
        settings["vignette"] = 100;
        const QImage shade = applyAdvancedRecipe(source,settings);
        QVERIFY(shade.pixelColor(0,0).red() < shade.pixelColor(50,40).red());
        settings = advancedDefaults();
        settings["grain"] = 50;
        QCOMPARE(applyAdvancedRecipe(source,settings),applyAdvancedRecipe(source,settings));
        QCOMPARE(source.pixelColor(20,20),QColor(255,0,0,128));
    }

    void presetSyncPreviewAndBatchExport() {
        std::fprintf(stderr,"TEST presetSyncPreviewAndBatchExport: enter\n");
        QTemporaryDir dir;
        QImage source(100,80,QImage::Format_ARGB32);
        source.fill(qRgba(255,0,0,128));
        const QString path = dir.filePath("source.png");
        QVERIFY(source.save(path));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        c.importFiles({QUrl::fromLocalFile(path),QUrl::fromLocalFile(path)});
        QTRY_VERIFY(!c.busy());
        c.setSetting("hsl_redHue",100);
        c.setSetting("cropLeft",10);
        c.setSetting("rotation",1);
        c.endSettingEdit();
        QVERIFY(c.useRenderedPreview());
        QTRY_VERIFY_WITH_TIMEOUT(!c.previewBusy(),15000);
        const QImage preview(QUrl(c.renderedPreviewUrl()).toLocalFile());
        QCOMPARE(preview.size(),QSize(80,90));
        QVERIFY(preview.pixelColor(20,20).green() > 80);
        const QUrl preset = QUrl::fromLocalFile(dir.filePath("preset.json"));
        c.savePreset(preset);
        QVERIFY(QFileInfo(preset.toLocalFile()).size() > 0);
        c.selectImage(1);
        c.setSetting("cropTop",20);
        c.endSettingEdit();
        c.selectImage(0);
        c.syncSelected("color");
        c.selectImage(1);
        QCOMPARE(c.currentSettings()["hsl_redHue"].toInt(),100);
        QCOMPARE(c.currentSettings()["cropTop"].toInt(),20);
        QCOMPARE(c.currentSettings()["rotation"].toInt(),0);
        c.undo();
        QCOMPARE(c.currentSettings()["hsl_redHue"].toInt(),0);
        c.loadPreset(preset);
        QCOMPARE(c.currentSettings()["rotation"].toInt(),1);
        QCOMPARE(c.currentSettings()["cropTop"].toInt(),0);
        c.undo();
        QCOMPARE(c.currentSettings()["cropTop"].toInt(),20);
        c.redo();
        QSignalSpy exported(&c,&AppController::exportFinished);
        QSignalSpy finished(&c,&AppController::exportQueueFinished);
        c.exportSelected(QUrl::fromLocalFile(dir.path()),"png",98);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,30000);
        QCOMPARE(exported.count(),2);
        QCOMPARE(c.exportCompleted(),2);
        QCOMPARE(finished.first()[0].toInt(),2);
        QCOMPARE(finished.first()[1].toInt(),0);
        QVERIFY(exported[0][0].toString() != exported[1][0].toString());
        const QImage output(exported[0][0].toString());
        QCOMPARE(output.size(),QSize(80,90));
        QVERIFY(output.pixelColor(20,20).green() > 80);
        QCOMPARE(output.pixelColor(20,20).alpha(),128);
    }

    void invalidPresetDoesNotChangeImageAndQueueCancels() {
        std::fprintf(stderr,"TEST invalidPresetDoesNotChangeImageAndQueueCancels: enter\n");
        QTemporaryDir dir;
        QImage source(32,32,QImage::Format_RGB32); source.fill(Qt::gray);
        const QString path = dir.filePath("source.png"); QVERIFY(source.save(path));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        c.importFiles({path,path,path}); QTRY_VERIFY(!c.busy());
        const QVariantMap before = c.currentSettings();
        const QString preset = dir.filePath("invalid.json");
        QFile f(preset); QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{\"schema\":1,\"settings\":{\"exposure\":2,\"unknown\":3}}");f.close();
        QSignalSpy errors(&c,&AppController::errorOccurred);
        c.loadPreset(QUrl::fromLocalFile(preset));
        QCOMPARE(errors.count(),1); QCOMPARE(c.currentSettings(),before);
        QSignalSpy finished(&c,&AppController::exportQueueFinished);
        c.exportSelected(QUrl::fromLocalFile(dir.path()),"png",98);
        c.cancelExport();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,15000);
        QVERIFY(finished.first()[2].toBool());
        QVERIFY(c.exportCompleted() < 3);
    }

    void imageWorkflow() {
        std::fprintf(stderr,"TEST imageWorkflow: enter\n");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage source(6000, 4000, QImage::Format_RGB32);
        source.fill(QColor(60, 80, 100));
        const QString path = dir.filePath("source.png");
        QVERIFY(source.save(path));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        QSignalSpy errors(&c, &AppController::errorOccurred);
        c.importFiles({QUrl::fromLocalFile(path), QUrl::fromLocalFile(path)});
        QVERIFY(c.busy());
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(c.images().size(), 2);
        QCOMPARE(c.currentIndex(), 0);
        QImageReader preview(QUrl(c.currentPreviewUrl()).toLocalFile());
        QVERIFY(preview.size().width() <= 2200);

        c.beginSettingEdit();
        c.setSetting("exposure", 0.5);
        c.setSetting("exposure", 1.0);
        c.endSettingEdit();
        QVERIFY(c.canUndo());
        c.undo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 0.0);
        QVERIFY(!c.canUndo());
        c.redo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 1.0);

        c.resetCurrentSettings();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 0.0);
        c.undo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 1.0);
        c.copySettings();
        c.selectImage(1);
        c.pasteSettings();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 1.0);
        c.undo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 0.0);
        c.redo();

        QSignalSpy exported(&c, &AppController::exportFinished);
        c.exportCurrent(QUrl::fromLocalFile(dir.path()), "png", 98);
        QVERIFY(c.busy());
        QTRY_COMPARE_WITH_TIMEOUT(exported.count(), 1, 60000);
        QCOMPARE(errors.count(), 0);
        const QList<QVariant> result = exported.takeFirst();
        QCOMPARE(result[2].toInt(), 6000);
        QCOMPARE(result[3].toInt(), 4000);
        const QImage output(result[0].toString());
        QCOMPARE(output.size(), source.size());
        const QColor px = output.pixelColor(100, 100);
        QVERIFY(qAbs(px.red() - 120) <= 1);
        QVERIFY(qAbs(px.green() - 160) <= 1);
        QVERIFY(qAbs(px.blue() - 200) <= 1);
        QCOMPARE(source.pixelColor(100, 100), QColor(60, 80, 100));
    }

    void selectionCommitsPendingEdit() {
        std::fprintf(stderr,"TEST selectionCommitsPendingEdit: enter\n");
        QTemporaryDir dir;
        QImage img(80, 60, QImage::Format_RGB32);
        img.fill(Qt::gray);
        QVERIFY(img.save(dir.filePath("test.png")));
        AppController c;QObject::connect(&c,&AppController::errorOccurred,&c,[](const QString &error){std::fprintf(stderr,"Controller error: %s\n",qPrintable(error));});
        c.importFiles({QUrl::fromLocalFile(dir.filePath("test.png")), QUrl::fromLocalFile(dir.filePath("test.png"))});
        QTRY_VERIFY(!c.busy());
        c.setSetting("contrast", 23);
        c.selectImage(1);
        c.selectImage(0);
        QVERIFY(c.canUndo());
        c.undo();
        QCOMPARE(c.currentSettings().value("contrast").toDouble(), 0.0);
        c.setSetting("exposure", 500);
        c.endSettingEdit();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 3.0);
        c.setSetting("unknown", 30);
        QVERIFY(!c.currentSettings().contains("unknown"));
    }
};

int main(int argc,char **argv) {
    std::setvbuf(stdout,nullptr,_IONBF,0);std::setvbuf(stderr,nullptr,_IONBF,0);
    std::fprintf(stderr,"Native tests: entering QApplication\n");
    QApplication app(argc,argv);std::fprintf(stderr,"Native tests: QApplication ready\n");
    ControllerTests tests;
    std::fprintf(stderr,"Native tests: constructing logger\n");
    QTest::qInit(&tests,argc,argv);
    std::fprintf(stderr,"Native tests: logger ready, running cases\n");
    int result=QTest::qRun();
    std::fprintf(stderr,"Native tests: cases complete\n");
    QTest::qCleanup();return result;
}
#include "ControllerTests.moc"
