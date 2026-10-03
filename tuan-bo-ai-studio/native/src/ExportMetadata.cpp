#include "ExportMetadata.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
bool preserveExportMetadata(const QString &source,const QString &destination,QSize size,QString *error) {
    QString executable=qEnvironmentVariable("TBRETOCH_EXIFTOOL_PATH");
    if(executable.isEmpty())executable=QDir(QCoreApplication::applicationDirPath()).filePath("exiftool.exe");
    if(!QFileInfo::exists(executable))executable=QStandardPaths::findExecutable("exiftool");
    if(executable.isEmpty()){*error=QStringLiteral("Metadata helper is missing from the installation");return false;}
    QProcess process;
    process.start(executable,{"-charset","filename=UTF8","-TagsFromFile",source,"-all:all","-Artist","-Copyright","-Author","-Description","-Title","-Comment","-Keywords","-XMP-dc:Creator<Artist","-XMP-dc:Rights<Copyright","-XMP-dc:Description<Description","-XMP-dc:Title<Title","--ICC_Profile","-Orientation#=1","-ThumbnailImage=","-PreviewImage=","-PreviewTIFF=","-JpgFromRaw=","-OtherImage=","-ExifImageWidth="+QString::number(size.width()),"-ExifImageHeight="+QString::number(size.height()),"-overwrite_original",destination});
    if(!process.waitForStarted(10000)||!process.waitForFinished(60000)){process.kill();process.waitForFinished();*error=QStringLiteral("Cannot preserve photo metadata: ")+process.errorString();return false;}
    if(process.exitStatus()!=QProcess::NormalExit||process.exitCode()!=0){*error=QString::fromUtf8(process.readAllStandardError());return false;}
    return true;
}
