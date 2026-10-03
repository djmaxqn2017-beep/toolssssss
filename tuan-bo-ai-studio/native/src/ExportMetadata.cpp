#include "ExportMetadata.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
bool runMetadataTool(const QString &executable,const QStringList &arguments,QByteArray *output,QString *error) {
    QByteArray payload("-charset\nfilename=UTF8\n");
    for(const QString &argument:arguments) {
        if(argument.contains('\n')||argument.contains('\r')) {
            if(error)*error=QStringLiteral("Metadata arguments contain an unsupported line break");
            return false;
        }
        payload+=argument.toUtf8();payload+='\n';
    }
    QProcess process;
    // A UTF-8 argument stream avoids the Windows command-line code page and
    // keeps Unicode file names and tag-copy expressions intact.
    process.start(executable,{"-@","-"});
    if(!process.waitForStarted(10000)) {
        if(error)*error=QStringLiteral("Cannot start metadata helper: ")+process.errorString();
        return false;
    }
    if(process.write(payload)!=payload.size()) {
        process.kill();process.waitForFinished(2000);
        if(error)*error=QStringLiteral("Cannot send metadata arguments");
        return false;
    }
    process.closeWriteChannel();
    if(!process.waitForFinished(60000)) {
        process.kill();process.waitForFinished(2000);
        if(error)*error=QStringLiteral("Cannot preserve photo metadata: ")+process.errorString();
        return false;
    }
    const QByteArray stdoutBytes=process.readAllStandardOutput();
    if(output)*output=stdoutBytes;
    if(process.exitStatus()!=QProcess::NormalExit||process.exitCode()!=0) {
        if(error)*error=QString::fromUtf8(process.readAllStandardError()+stdoutBytes);
        return false;
    }
    return true;
}
bool preserveExportMetadata(const QString &source,const QString &destination,QSize size,QString *error) {
    QString executable=qEnvironmentVariable("TBRETOCH_EXIFTOOL_PATH");
    if(executable.isEmpty())executable=QDir(QCoreApplication::applicationDirPath()).filePath("exiftool.exe");
    if(!QFileInfo::exists(executable))executable=QStandardPaths::findExecutable("exiftool");
    if(executable.isEmpty()){*error=QStringLiteral("Metadata helper is missing from the installation");return false;}
    return runMetadataTool(executable,{"-TagsFromFile",source,"-all:all","-Artist","-Copyright","-Author","-Description","-Title","-Comment","-Keywords","-XMP-dc:Creator<Artist","-XMP-dc:Rights<Copyright","-XMP-dc:Description<Description","-XMP-dc:Title<Title","--ICC_Profile","-Orientation#=1","-ThumbnailImage=","-PreviewImage=","--PreviewTIFF","-JpgFromRaw=","-OtherImage=","-ExifImageWidth="+QString::number(size.width()),"-ExifImageHeight="+QString::number(size.height()),"-overwrite_original",destination},nullptr,error);
}
