#include "SemanticEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QVariantMap>
#include <QRegularExpression>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#define ORT_API_MANUAL_INIT
#include <onnxruntime_cxx_api.h>
#include <QDebug>
#include <array>
#include <mutex>
#include <cmath>
#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>
#endif

namespace {
QString modelRoot() {
    const QString override = qEnvironmentVariable("TBRETOCH_MODEL_DIR");
    return override.isEmpty() ? QDir(QCoreApplication::applicationDirPath()).filePath("models") : override;
}
#if defined(Q_OS_WIN)
int hardwareAdapter() {
    static const int adapter=[] {
        HMODULE module=LoadLibraryExW(L"dxgi.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!module)return -1;
        using Create=HRESULT(WINAPI *)(REFIID,void **);
        auto create=reinterpret_cast<Create>(GetProcAddress(module,"CreateDXGIFactory1"));
        IDXGIFactory1 *factory=nullptr;int result=-1;
        if(create&&SUCCEEDED(create(__uuidof(IDXGIFactory1),reinterpret_cast<void**>(&factory)))) {
            for(UINT index=0;;++index){IDXGIAdapter1 *device=nullptr;if(factory->EnumAdapters1(index,&device)==DXGI_ERROR_NOT_FOUND)break;if(!device)break;DXGI_ADAPTER_DESC1 info{};if(SUCCEEDED(device->GetDesc1(&info))&&!(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&info.VendorId!=0x1414)result=int(index);device->Release();if(result>=0)break;}
            factory->Release();
        }
        FreeLibrary(module);return result;
    }();return adapter;
}
#endif
struct Net {
    Ort::Session session{nullptr};
    std::string input;
    std::vector<std::string> outputs;
    QString backend = "CPU";
    Net(Ort::Env &env, const QString &name) {
        const QString path = QDir(modelRoot()).filePath(name);
        if (!QFile::exists(path)) throw std::runtime_error((QString("Missing installed model: ")+path).toStdString());
        const auto make = [&](bool gpu) {
            Ort::SessionOptions opts;
            opts.SetIntraOpNumThreads(2);
            opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            opts.SetLogSeverityLevel(3);
#if defined(Q_OS_WIN)
            if (gpu && !qEnvironmentVariableIsSet("TBRETOCH_AI_CPU")) {
                using Append = OrtStatus* (ORT_API_CALL *)(OrtSessionOptions*,int);
                auto fn = reinterpret_cast<Append>(GetProcAddress(GetModuleHandleW(L"onnxruntime.dll"),"OrtSessionOptionsAppendExecutionProvider_DML"));
                const int adapter=hardwareAdapter();
                if (fn && adapter>=0) {
                    opts.DisableMemPattern(); opts.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
                    Ort::ThrowOnError(fn(opts,adapter)); backend = "DirectML";
                }
            }
            session = Ort::Session(env,reinterpret_cast<const wchar_t*>(path.utf16()),opts);
#else
            Q_UNUSED(gpu);
            session = Ort::Session(env,path.toUtf8().constData(),opts);
#endif
        };
        try { make(true); } catch (const Ort::Exception &) { backend="CPU"; make(false); }
        Ort::AllocatorWithDefaultOptions allocator;
        input = session.GetInputNameAllocated(0,allocator).get();
        for (size_t i=0;i<session.GetOutputCount();++i) outputs.emplace_back(session.GetOutputNameAllocated(i,allocator).get());
    }
    std::vector<Ort::Value> run(std::vector<float> &data,const std::vector<int64_t> &shape) {
        auto memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
        auto tensor=Ort::Value::CreateTensor<float>(memory,data.data(),data.size(),shape.data(),shape.size());
        const char *in=input.c_str(); std::vector<const char*> out;
        for (const auto &s:outputs) out.push_back(s.c_str());
        return session.Run(Ort::RunOptions{nullptr},&in,&tensor,1,out.data(),out.size());
    }
};
struct Engine {
    Ort::Env env{nullptr};
    std::unique_ptr<Net> detector,mesh,segment,depth,sky;
    std::mutex mutex;
    Engine() {
        const auto *base=OrtGetApiBase();
        const auto *api=base?base->GetApi(ORT_API_VERSION):nullptr;
        if(!api)throw std::runtime_error("Installed AI runtime is incompatible. Reinstall TBRetoch 0.7.0.");
        Ort::InitApi(api);
        env=Ort::Env(ORT_LOGGING_LEVEL_ERROR,"TBRetoch");
        cv::setNumThreads(2);
        qInfo()<<"Native inference runtime"<<base->GetVersionString();
#if defined(Q_OS_WIN)
        wchar_t location[32768];auto module=GetModuleHandleW(L"onnxruntime.dll");
        if(module&&GetModuleFileNameW(module,location,32768))qInfo()<<"Inference runtime path"<<QString::fromWCharArray(location);
#endif
    }
    Net &net(std::unique_ptr<Net> &slot,const char *file) {
        if (!slot) slot=std::make_unique<Net>(env,QString::fromLatin1(file));
        return *slot;
    }
};
Engine &engine() { static Engine value; return value; }
cv::Mat rgbImage(const QImage &image) {
    QImage rgb=image.convertToFormat(QImage::Format_RGB888);
    return cv::Mat(rgb.height(),rgb.width(),CV_8UC3,const_cast<uchar*>(rgb.constBits()),rgb.bytesPerLine()).clone();
}
QImage grayImage(const cv::Mat &mask) {
    cv::Mat bytes; mask.convertTo(bytes,CV_8U,255);
    return QImage(bytes.data,bytes.cols,bytes.rows,int(bytes.step),QImage::Format_Grayscale8).copy();
}
cv::Mat maskFloat(const QImage &image,cv::Size size) {
    QImage gray=image.convertToFormat(QImage::Format_Grayscale8);
    if (gray.isNull()) return cv::Mat::zeros(size,CV_32F);
    cv::Mat bytes(gray.height(),gray.width(),CV_8U,const_cast<uchar*>(gray.constBits()),gray.bytesPerLine());
    cv::Mat resized,result; cv::resize(bytes,resized,size,0,0,cv::INTER_LINEAR); resized.convertTo(result,CV_32F,1.0/255);
    return result;
}
// Joint guided refinement follows image edges rather than blurring a bounding box.
cv::Mat refine(const cv::Mat &coarse,const cv::Mat &rgb) {
    cv::Mat p,I; cv::resize(coarse,p,rgb.size(),0,0,cv::INTER_LINEAR);
    cv::cvtColor(rgb,I,cv::COLOR_RGB2GRAY); I.convertTo(I,CV_32F,1.0/255);
    cv::Mat meanI,meanP,II,Ip; const cv::Size window(13,13);
    cv::boxFilter(I,meanI,-1,window); cv::boxFilter(p,meanP,-1,window);
    cv::boxFilter(I.mul(I),II,-1,window); cv::boxFilter(I.mul(p),Ip,-1,window);
    cv::Mat a,b; cv::divide(Ip-meanI.mul(meanP),II-meanI.mul(meanI)+.001,a);
    b=meanP-a.mul(meanI); cv::boxFilter(a,a,-1,window); cv::boxFilter(b,b,-1,window);
    cv::Mat result=a.mul(I)+b; cv::max(result,0,result); cv::min(result,1,result); return result;
}
std::vector<float> chw(const cv::Mat &rgb,double scale,double bias=0) {
    std::vector<float> result(size_t(rgb.rows)*rgb.cols*3);
    for (int y=0;y<rgb.rows;++y) for (int x=0;x<rgb.cols;++x) for(int c=0;c<3;++c)
        result[size_t(c)*rgb.rows*rgb.cols+y*rgb.cols+x]=float(rgb.at<cv::Vec3b>(y,x)[c]*scale+bias);
    return result;
}
struct Detection { double score; cv::Rect2d box; std::array<cv::Point2d,6> points; };
double overlap(const cv::Rect2d &a,const cv::Rect2d &b) { double area=(a&b).area(); return area/std::max(1e-9,a.area()+b.area()-area); }
std::vector<Detection> detect(Net &net,const cv::Mat &rgb) {
    const double scale=128.0/std::max(rgb.rows,rgb.cols), px=(128-rgb.cols*scale)/2,py=(128-rgb.rows*scale)/2;
    cv::Mat canvas; cv::warpAffine(rgb,canvas,cv::Matx23d(scale,0,px,0,scale,py),{128,128});
    auto data=chw(canvas,1.0/127.5,-1); auto output=net.run(data,{1,3,128,128});
    const float *reg=output[0].GetTensorData<float>(),*scores=output[1].GetTensorData<float>();
    std::vector<Detection> candidates; int anchor=0;
    for (const auto grid: {std::pair<int,int>{16,2},{8,6}})
        for (int y=0;y<grid.first;++y) for (int x=0;x<grid.first;++x) for(int k=0;k<grid.second;++k,++anchor) {
            const double score=1/(1+std::exp(-double(scores[anchor]))); if(score<.6)continue;
            const float *r=reg+anchor*16;
            const double ax=(x+.5)/grid.first,ay=(y+.5)/grid.first;
            const double cx=(r[0]+ax*128-px)/scale,cy=(r[1]+ay*128-py)/scale;
            Detection d{score,{cx-r[2]/scale/2,cy-r[3]/scale/2,r[2]/scale,r[3]/scale},{}};
            for(int j=0;j<6;++j)d.points[j]={(r[4+j*2]+ax*128-px)/scale,(r[5+j*2]+ay*128-py)/scale};
            candidates.push_back(d);
        }
    std::sort(candidates.begin(),candidates.end(),[](const auto&a,const auto&b){return a.score>b.score;});
    std::vector<Detection> result;
    while(!candidates.empty()) {
        Detection top=candidates.front(),sum{0,{0,0,0,0},{}}; std::vector<Detection> remaining;
        for(const auto &d:candidates) {
            if(overlap(top.box,d.box)<=.3){remaining.push_back(d);continue;}
            sum.score+=d.score;sum.box.x+=d.box.x*d.score;sum.box.y+=d.box.y*d.score;
            sum.box.width+=d.box.width*d.score;sum.box.height+=d.box.height*d.score;
            for(int j=0;j<6;++j)sum.points[j]+=d.points[j]*d.score;
        }
        if(sum.score<=0)break;
        top.box={sum.box.x/sum.score,sum.box.y/sum.score,sum.box.width/sum.score,sum.box.height/sum.score};
        for(int j=0;j<6;++j)top.points[j]=sum.points[j]/sum.score;
        result.push_back(top);candidates=std::move(remaining);
    }
    return result;
}
cv::Point point(const FaceAnalysis &face,int index,cv::Size size) {
    const auto p=face.landmarks.at(index);return {cvRound(p.x()*size.width),cvRound(p.y()*size.height)};
}
cv::Mat polygon(const FaceAnalysis &face,const std::vector<int> &indices,cv::Size size) {
    cv::Mat mask=cv::Mat::zeros(size,CV_32F);std::vector<cv::Point> points;
    for(int idx:indices)points.push_back(point(face,idx,size));
    if(points.size()>2)cv::fillPoly(mask,std::vector<std::vector<cv::Point>>{points},cv::Scalar(1));
    return mask;
}
const std::vector<int> leftEye={33,160,158,133,153,144},rightEye={362,385,387,263,373,380};
const std::vector<int> lips={61,40,37,0,267,270,291,321,314,17,84,91};
const std::vector<int> mouth={78,82,13,312,308,317,14,87};
const std::vector<int> oval={10,338,297,332,284,251,389,356,454,323,361,288,397,365,379,378,400,377,152,148,176,149,150,136,172,58,132,93,234,127,162,21,54,103,67,109};
void featureMasks(PortraitAnalysis &a,const cv::Mat &rgb) {
    const cv::Size size=rgb.size();
    QMap<QString,cv::Mat> masks;
    for(const QString &key: {"eyes","lips","teeth","iris","whites","brows","underEyes","eyelids","cheeks","jaw","mouth"})masks.insert(key,cv::Mat::zeros(size,CV_32F));
    for(const auto &face:a.faces) {
        cv::Mat eyes=polygon(face,leftEye,size)+polygon(face,rightEye,size);
        cv::Mat lip=polygon(face,lips,size),inner=polygon(face,mouth,size); lip-=inner;cv::max(lip,0,lip);
        cv::Mat iris=cv::Mat::zeros(size,CV_32F);
        for(int start:{468,473}) {
            std::vector<int> ids={start+1,start+2,start+3,start+4};iris+=polygon(face,ids,size);
        }
        iris=iris.mul(eyes);
        cv::Mat gray;cv::cvtColor(rgb,gray,cv::COLOR_RGB2GRAY);gray.convertTo(gray,CV_32F,1.0/255);
        cv::Mat enamel=gray>.45,whitePixels=gray>.35;
        enamel.convertTo(enamel,CV_32F,1.0/255);whitePixels.convertTo(whitePixels,CV_32F,1.0/255);
        cv::Mat teeth=inner.mul(enamel); // visible enamel only; protect dark mouth/lips
        cv::Mat whites=(eyes-iris).mul(whitePixels);
        cv::max(masks["eyes"],eyes,masks["eyes"]);cv::max(masks["lips"],lip,masks["lips"]);
        cv::max(masks["mouth"],inner,masks["mouth"]);cv::max(masks["teeth"],teeth,masks["teeth"]);cv::max(masks["iris"],iris,masks["iris"]);cv::max(masks["whites"],whites,masks["whites"]);
        const auto add=[&](const QString &name,const std::vector<int>&ids){auto p=polygon(face,ids,size);cv::max(masks[name],p,masks[name]);};
        add("brows",{70,63,105,66,107,55,65,52,53,46});add("brows",{336,296,334,293,300,276,283,282,295,285});
        add("underEyes",{33,144,153,133,50,101,118,117});add("underEyes",{362,380,373,263,346,347,330,280});
        add("eyelids",{33,160,158,133,55,65,52,53});add("eyelids",{362,385,387,263,283,282,295,285});
        add("cheeks",{50,101,205,187,123,116});add("cheeks",{280,330,425,411,352,345});
        add("jaw",{172,136,150,149,176,148,152,377,400,378,379,365,397,288,361,323});
    }
    for(auto it=masks.begin();it!=masks.end();++it) {
        cv::max(it.value(),0,it.value());cv::min(it.value(),1,it.value());
        cv::GaussianBlur(it.value(),it.value(),{0,0},1);
        a.masks.insert(it.key(),grayImage(it.value()));
    }
    cv::Mat skin=maskFloat(a.masks.value("faceSkin"),size),protect=masks["eyes"]+masks["lips"]+masks["brows"]+masks["mouth"];
    cv::dilate(protect,protect,cv::getStructuringElement(cv::MORPH_ELLIPSE,{7,7}));cv::min(protect,1,protect);
    a.masks["faceSkin"]=grayImage(skin.mul(1-protect));
}
void saveAnalysis(const PortraitAnalysis &a,const QString &cache) {
    if(!QDir().mkpath(cache))throw std::runtime_error("Analysis cache is not writable");
    for(auto it=a.masks.cbegin();it!=a.masks.cend();++it)if(!it.value().save(QDir(cache).filePath(it.key()+".png")))throw std::runtime_error("Cannot save semantic mask");
    QJsonArray faces;
    for(const auto &f:a.faces) {
        QJsonArray points;for(const auto&p:f.landmarks)points.append(QJsonArray{p.x(),p.y(),p.z()});
        faces.append(QJsonObject{{"bounds",QJsonArray{f.bounds.x(),f.bounds.y(),f.bounds.width(),f.bounds.height()}},{"points",points},{"confidence",f.confidence}});
    }
    QSaveFile file(QDir(cache).filePath("analysis.json"));if(!file.open(QIODevice::WriteOnly))throw std::runtime_error(file.errorString().toStdString());
    file.write(QJsonDocument(QJsonObject{{"schema",3},{"backend",a.backend},{"faces",faces}}).toJson());if(!file.commit())throw std::runtime_error(file.errorString().toStdString());
}
std::shared_ptr<PortraitAnalysis> loadAnalysis(const QString &cache) {
    QFile file(QDir(cache).filePath("analysis.json"));if(!file.open(QIODevice::ReadOnly))return {};
    auto obj=QJsonDocument::fromJson(file.readAll()).object();if(obj.value("schema").toInt()!=3)return {};
    auto a=std::make_shared<PortraitAnalysis>();a->backend=obj.value("backend").toString();
    for(const auto &value:obj.value("faces").toArray()) {
        auto f=value.toObject();auto b=f.value("bounds").toArray();if(b.size()!=4)return {};
        FaceAnalysis face{{b[0].toDouble(),b[1].toDouble(),b[2].toDouble(),b[3].toDouble()},{},f.value("confidence").toDouble()};
        for(const auto &p:f.value("points").toArray()){auto q=p.toArray();if(q.size()!=3)return {};face.landmarks.append({float(q[0].toDouble()),float(q[1].toDouble()),float(q[2].toDouble())});}
        if(face.landmarks.size()!=478)return {};a->faces.append(face);
    }
    for(const auto &name:semanticMaskNames()) {QImage mask(QDir(cache).filePath(name+".png"));if(!mask.isNull())a->masks.insert(name,mask);}
    for(const char *name:{"depth","iris","whites","brows","underEyes","eyelids","cheeks","jaw","mouth"}) {QImage mask(QDir(cache).filePath(QString(name)+".png"));if(!mask.isNull())a->masks.insert(name,mask);}
    if(!a->masks.contains("subject")||!a->masks.contains("faceSkin"))return {};
    return a;
}
}

QStringList semanticMaskNames() {return {"subject","person","faceSkin","bodySkin","hair","clothes","background","eyes","lips","teeth","sky"};}
QString canonicalSettingKey(const QString &key) {
    static const QRegularExpression prefix("^face_([0-9]{1,2})_(.+)$");auto m=prefix.match(key);
    return m.hasMatch()&&m.captured(1).toInt()<32 ? m.captured(2):key;
}
QVariantMap semanticDefaults() {
    QVariantMap s;
    for(const char *key:{"blemishRemoval","skinSoftening","textureRecovery","faceShine","skinUnify","eyeBags","darkCircles","wrinkles","doubleChin","faceWidth","jaw","chin","vShape","eyeSize","noseWidth","lipSize","iris","eyeWhites","catchlight","teethWhitening","lipstick","blush","eyeliner","eyeshadow","eyebrow","bgCleanup","bgBlur","lensBlur","skyReplacement","wrinkleRemoval","lintRemoval","stainRemoval","hairSmooth","hairShine"})s.insert(key,0.0);
    s.insert("skyFile",QString());s.insert("backgroundFile",QString());s.insert("backgroundReplacement",0.0);
    for(const auto &mask:semanticMaskNames())for(const char *effect:{"exposure","contrast","saturation","temperature"})s.insert("mask_"+mask+"_"+effect,0.0);
    return s;
}
bool isSemanticKey(const QString &key) {return semanticDefaults().contains(canonicalSettingKey(key));}
bool hasSemanticSettings(const QVariantMap &settings) {
    const auto defaults=semanticDefaults();
    for(auto it=settings.cbegin();it!=settings.cend();++it) {
        auto key=canonicalSettingKey(it.key());if(!defaults.contains(key)||key.endsWith("File"))continue;
        if(it.value().toDouble()!=0)return true;
    }return false;
}

std::shared_ptr<PortraitAnalysis> analysePortrait(const QImage &source,const QString &cache,bool needDepth,bool needSky) {
    auto cached=loadAnalysis(cache);
    if(cached&&(!needDepth||cached->masks.contains("depth"))&&(!needSky||cached->masks.contains("sky")))return cached;
    if(source.isNull())throw std::runtime_error("Cannot analyse an empty image");
    auto &e=engine();std::lock_guard<std::mutex> lock(e.mutex);
    QImage proxy=source.scaled(1024,1024,Qt::KeepAspectRatio,Qt::SmoothTransformation);cv::Mat rgb=rgbImage(proxy);
    auto a=cached?cached:std::make_shared<PortraitAnalysis>();
    if(!cached) {
        auto &seg=e.net(e.segment,"selfie_multiclass.onnx");cv::Mat resizedInput;cv::resize(rgb,resizedInput,{256,256});
        std::vector<float> input(256*256*3);
        for(int y=0;y<256;++y)for(int x=0;x<256;++x)for(int c=0;c<3;++c)input[(y*256+x)*3+c]=resizedInput.at<cv::Vec3b>(y,x)[c]/127.5f-1;
        auto output=seg.run(input,{1,256,256,3});const float *logits=output[0].GetTensorData<float>();
        const std::array<QString,6> names={"background","hair","bodySkin","faceSkin","clothes","accessories"};
        std::array<cv::Mat,6> masks;for(auto&m:masks)m=cv::Mat(256,256,CV_32F);
        for(int y=0;y<256;++y)for(int x=0;x<256;++x) {
            const float *scores=logits+(y*256+x)*6;const float maximum=*std::max_element(scores,scores+6);
            double total=0;std::array<double,6> values;for(int c=0;c<6;++c){values[c]=std::exp(double(scores[c]-maximum));total+=values[c];}
            for(int c=0;c<6;++c)masks[c].at<float>(y,x)=float(values[c]/total);
        }
        for(int c=0;c<6;++c)a->masks.insert(names[c],grayImage(refine(masks[c],rgb)));
        a->masks.insert("subject",grayImage(1-maskFloat(a->masks["background"],rgb.size())));a->masks["person"]=a->masks["subject"];
        auto &det=e.net(e.detector,"blazeface.onnx");auto &mesh=e.net(e.mesh,"face_landmarker.onnx");
        auto detections=detect(det,rgb);
        // Overlapping crops recover smaller/group faces that the global 128px detector misses.
        if(std::max(rgb.cols,rgb.rows)>512)for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx) {
            cv::Rect roi(xx*rgb.cols/3,yy*rgb.rows/3,rgb.cols*2/3,rgb.rows*2/3);roi&=cv::Rect(0,0,rgb.cols,rgb.rows);
            for(auto d:detect(det,rgb(roi))) {
                d.box.x+=roi.x;d.box.y+=roi.y;for(auto&p:d.points){p.x+=roi.x;p.y+=roi.y;}
                bool duplicate=false;for(const auto &old:detections)if(overlap(old.box,d.box)>.3){duplicate=true;break;}
                if(!duplicate)detections.push_back(d);
            }
        }
        std::sort(detections.begin(),detections.end(),[](const auto&a,const auto&b){return a.box.x<b.box.x;});
        for(const auto &d:detections) {
            if(a->faces.size()>=32)break;
            const auto cx=d.box.x+d.box.width/2,cy=d.box.y+d.box.height/2,side=1.5*std::max(d.box.width,d.box.height);
            if(side<8)continue;const auto eye=d.points[1]-d.points[0];const double angle=std::atan2(eye.y,eye.x)*180/CV_PI;
            cv::Mat matrix=cv::getRotationMatrix2D({float(cx),float(cy)},angle,256/side);matrix.at<double>(0,2)+=128-cx;matrix.at<double>(1,2)+=128-cy;
            cv::Mat crop,inverse;cv::warpAffine(rgb,crop,matrix,{256,256});cv::invertAffineTransform(matrix,inverse);
            auto blob=chw(crop,1.0/255);auto result=mesh.run(blob,{1,3,256,256});const float *points=result[0].GetTensorData<float>();
            const double confidence=1/(1+std::exp(-double(result[1].GetTensorData<float>()[0])));if(confidence<.75)continue;
            FaceAnalysis face{{d.box.x/rgb.cols,d.box.y/rgb.rows,d.box.width/rgb.cols,d.box.height/rgb.rows},{},confidence};
            for(int i=0;i<478;++i)face.landmarks.append({float((inverse.at<double>(0,0)*points[i*3]+inverse.at<double>(0,1)*points[i*3+1]+inverse.at<double>(0,2))/rgb.cols),float((inverse.at<double>(1,0)*points[i*3]+inverse.at<double>(1,1)*points[i*3+1]+inverse.at<double>(1,2))/rgb.rows),float(points[i*3+2]*side/256/rgb.cols)});
            a->faces.append(face);
        }
        featureMasks(*a,rgb);a->backend=seg.backend;
    }
    if(needDepth&&!a->masks.contains("depth")) {
        auto &net=e.net(e.depth,"depth.onnx");cv::Mat resizedInput;cv::resize(rgb,resizedInput,{518,518});auto input=chw(resizedInput,1.0/255);
        const double mean[]={.485,.456,.406},stddev[]={.229,.224,.225};
        for(int c=0;c<3;++c)for(size_t i=size_t(c)*518*518;i<size_t(c+1)*518*518;++i)input[i]=float((input[i]-mean[c])/stddev[c]);
        auto output=net.run(input,{1,3,518,518});auto shape=output[0].GetTensorTypeAndShapeInfo().GetShape();
        cv::Mat raw(static_cast<int>(shape[shape.size()-2]),static_cast<int>(shape.back()),CV_32F,output[0].GetTensorMutableData<float>()),norm;
        cv::normalize(raw,norm,0,1,cv::NORM_MINMAX);a->masks["depth"]=grayImage(refine(norm,rgb));
    }
    if(needSky&&!a->masks.contains("sky")) {
        auto &net=e.net(e.sky,"sky.onnx");cv::Mat resizedInput;cv::resize(rgb,resizedInput,{320,320});auto input=chw(resizedInput,1.0/255);
        const double mean[]={.485,.456,.406},stddev[]={.229,.224,.225};for(int c=0;c<3;++c)for(size_t i=size_t(c)*320*320;i<size_t(c+1)*320*320;++i)input[i]=float((input[i]-mean[c])/stddev[c]);
        auto output=net.run(input,{1,3,320,320});auto shape=output[0].GetTensorTypeAndShapeInfo().GetShape();
        cv::Mat raw(static_cast<int>(shape[shape.size()-2]),static_cast<int>(shape.back()),CV_32F,output[0].GetTensorMutableData<float>());
        a->masks["sky"]=grayImage(refine(raw,rgb));
    }
    saveAnalysis(*a,cache);return a;
}

QImage semanticMask(const PortraitAnalysis &analysis,const QString &name,QSize size,int face) {
    auto image=analysis.masks.value(name);if(image.isNull())return {};
    if(face<0||face>=analysis.faces.size())return image.scaled(size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    auto base=maskFloat(image,{size.width(),size.height()});auto region=polygon(analysis.faces[face],oval,base.size());
    return grayImage(base.mul(region));
}
