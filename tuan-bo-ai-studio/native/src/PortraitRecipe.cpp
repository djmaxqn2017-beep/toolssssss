#include "SemanticEngine.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <QtMath>
#include <QColorSpace>
#include <stdexcept>

namespace {
cv::Mat pixels(const QImage &image) {
    QImage q=image.convertToFormat(QImage::Format_RGBA64);cv::Mat out(q.height(),q.width(),CV_32FC3);
    for(int y=0;y<q.height();++y){auto row=reinterpret_cast<const QRgba64*>(q.constScanLine(y));auto d=out.ptr<cv::Vec3f>(y);for(int x=0;x<q.width();++x)d[x]={row[x].red()/65535.f,row[x].green()/65535.f,row[x].blue()/65535.f};}return out;
}
cv::Mat mask(const PortraitAnalysis&a,const QString&name,cv::Size size,int face=-1) {
    QImage q=semanticMask(a,name,{size.width,size.height},face).convertToFormat(QImage::Format_Grayscale8);
    if(q.isNull())return cv::Mat::zeros(size,CV_32F);
    cv::Mat m(q.height(),q.width(),CV_8U,const_cast<uchar*>(q.constBits()),q.bytesPerLine());cv::Mat out;m.convertTo(out,CV_32F,1./255);return out;
}
void blend(cv::Mat &dst,const cv::Mat &effect,const cv::Mat &m,double amount) {
    if(amount==0)return;for(int y=0;y<dst.rows;++y){auto d=dst.ptr<cv::Vec3f>(y);auto e=effect.ptr<cv::Vec3f>(y);auto w=m.ptr<float>(y);for(int x=0;x<dst.cols;++x){float a=qBound(0.,double(w[x])*amount,1.);d[x]=d[x]*(1-a)+e[x]*a;}}
}
cv::Mat smooth(const cv::Mat &src,double radius) {cv::Mat out;cv::GaussianBlur(src,out,{0,0},qMax(.6,radius));return out;}
void tone(cv::Mat &rgb,const cv::Mat &m,double exposure,double contrast,double saturation,double temperature) {
    if(exposure==0 && contrast==0 && saturation==0 && temperature==0)return;
    cv::Mat effect=rgb.clone();for(int y=0;y<rgb.rows;++y){auto p=effect.ptr<cv::Vec3f>(y);for(int x=0;x<rgb.cols;++x){auto v=p[x]*float(std::pow(2.,exposure));v[0]+=temperature*.001;v[2]-=temperature*.001;float l=.2126*v[0]+.7152*v[1]+.0722*v[2];for(int c=0;c<3;++c)v[c]=float((l+(v[c]-l)*(1+saturation/100)-.5)*(1+contrast/100)+.5);p[x]=v;}}blend(rgb,effect,m,1);
}
// Repair only detected small defects inside the semantic region. Feature masks
// prevent mouth, eyes and eyebrows from entering the skin repair selection.
void repair(cv::Mat &rgb,const cv::Mat &region,double amount,int mode) {
    if(amount<=0)return;cv::Mat gray;cv::cvtColor(rgb,gray,cv::COLOR_RGB2GRAY);
    double scale=qMax(1.,rgb.cols/1024.);cv::Mat low=smooth(gray,(mode==2?12:3)*scale),candidate;
    if(mode==0){cv::Mat diff=low-gray;candidate=diff>.045;}
    else if(mode==1){cv::Mat diff;cv::absdiff(gray,low,diff);candidate=diff>.10;}
    else if(mode==2){cv::Mat lab;cv::cvtColor(rgb,lab,cv::COLOR_RGB2Lab);std::vector<cv::Mat> ch;cv::split(lab,ch);cv::Mat ab0=smooth(ch[1],15*scale),ab1=smooth(ch[2],15*scale),distance;cv::magnitude(ch[1]-ab0,ch[2]-ab1,distance);candidate=distance>12;}
    else {cv::Mat u;gray.convertTo(u,CV_8U,255);cv::Mat black;cv::morphologyEx(u,black,cv::MORPH_BLACKHAT,cv::getStructuringElement(cv::MORPH_ELLIPSE,{int(6*scale)*2+1,int(6*scale)*2+1}));candidate=black>18;}
    cv::Mat allowed=region>.7;cv::bitwise_and(candidate,allowed,candidate);
    cv::Mat labels,stats,centers;int count=cv::connectedComponentsWithStats(candidate,labels,stats,centers);cv::Mat selected=cv::Mat::zeros(candidate.size(),CV_8U);
    for(int i=1;i<count;++i){int area=stats.at<int>(i,cv::CC_STAT_AREA);if(area>=2*scale*scale&&area<(mode==2?1600:350)*scale*scale)selected.setTo(255,labels==i);}
    cv::dilate(selected,selected,cv::getStructuringElement(cv::MORPH_ELLIPSE,{3,3}));cv::bitwise_and(selected,allowed,selected);
    if(cv::countNonZero(selected)==0)return;
    cv::Rect roi=cv::boundingRect(selected);const int pad=qMax(4,int(scale*10));
    roi={qMax(0,roi.x-pad),qMax(0,roi.y-pad),qMin(rgb.cols,roi.x+roi.width+pad)-qMax(0,roi.x-pad),qMin(rgb.rows,roi.y+roi.height+pad)-qMax(0,roi.y-pad)};
    cv::Mat target=rgb(roi),repaired=target.clone();
    // OpenCV supports float inpainting for single-channel input. Repair each
    // channel at native 16-bit scale, avoiding an 8-bit quantization round trip.
    for(int c=0;c<3;++c){cv::Mat plane,out;cv::extractChannel(target,plane,c);plane*=65535;cv::inpaint(plane,selected(roi),out,3*scale,cv::INPAINT_TELEA);out/=65535;cv::insertChannel(out,repaired,c);}
    cv::Mat weight;selected(roi).convertTo(weight,CV_32F,1./255);weight=smooth(weight,scale);blend(target,repaired,weight,amount);
}
void makeup(cv::Mat &rgb,const cv::Mat &m,double amount,cv::Vec3f color) {
    if(amount==0)return;
    cv::Mat effect=rgb.clone();for(int y=0;y<rgb.rows;++y){auto p=effect.ptr<cv::Vec3f>(y);for(int x=0;x<rgb.cols;++x){float lum=.2126*p[x][0]+.7152*p[x][1]+.0722*p[x][2];float cl=.2126*color[0]+.7152*color[1]+.0722*color[2];p[x]=color*(lum/qMax(.1f,cl));}}blend(rgb,effect,m,amount*.65);
}
void warpFace(cv::Mat &rgb,const PortraitAnalysis&a,int face,const QVariantMap&s,const QString&prefix) {
    const auto get=[&](const char*k){return s.value(prefix+QString(k)).toDouble()/100;};
    if(get("faceWidth")==0&&get("jaw")==0&&get("chin")==0&&get("vShape")==0&&get("eyeSize")==0&&get("noseWidth")==0&&get("lipSize")==0&&get("doubleChin")==0)return;
    const auto&f=a.faces[face];if(f.landmarks.size()!=478)return;
    cv::Mat mx(rgb.size(),CV_32F),my(rgb.size(),CV_32F);for(int y=0;y<rgb.rows;++y)for(int x=0;x<rgb.cols;++x){mx.at<float>(y,x)=x;my.at<float>(y,x)=y;}
    auto point=[&](int i){auto p=f.landmarks[i];return cv::Point2f(p.x()*rgb.cols,p.y()*rgb.rows);};
    const auto field=[&](cv::Point2f center,double rx,double ry,double horizontal,double vertical){rx=qMax(2.,rx);ry=qMax(2.,ry);int x0=qMax(0,int(center.x-rx*2)),x1=qMin(rgb.cols,int(center.x+rx*2+1)),y0=qMax(0,int(center.y-ry*2)),y1=qMin(rgb.rows,int(center.y+ry*2+1));for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){double dx=(x-center.x)/rx,dy=(y-center.y)/ry,w=std::exp(-2*(dx*dx+dy*dy));mx.at<float>(y,x)+=horizontal*(x-center.x)*w;my.at<float>(y,x)+=vertical*(y-center.y)*w;}};
    const double width=cv::norm(point(234)-point(454)),height=cv::norm(point(10)-point(152));
    field(point(1),width*.55,height*.6,-get("faceWidth")*.18,0);
    field((point(172)+point(397))*.5,width*.55,height*.3,(get("jaw")+get("vShape"))*.18,0);
    field(point(152),width*.3,height*.23,0,-get("chin")*.25-get("doubleChin")*.15);
    for(auto ids:{std::pair<int,int>{33,133},{362,263}})field((point(ids.first)+point(ids.second))*.5,width*.18,height*.10,-get("eyeSize")*.32,-get("eyeSize")*.32);
    field(point(1),width*.14,height*.17,-get("noseWidth")*.30,0);
    field(point(13),width*.22,height*.10,-get("lipSize")*.25,-get("lipSize")*.25);
    cv::Mat changed;cv::remap(rgb,changed,mx,my,cv::INTER_CUBIC,cv::BORDER_REFLECT_101);blend(rgb,changed,mask(a,"subject",rgb.size()),1);
}
}
QImage applyPortraitRecipe(QImage image,const QVariantMap &s,const PortraitAnalysis&a) {
    if(!hasSemanticSettings(s))return image;
    QImage original=image.convertToFormat(QImage::Format_RGBA64);cv::Mat rgb=pixels(original);double scale=qMax(.5,rgb.cols/1024.);
    const auto process=[&](int face,const QString&prefix){
        const auto val=[&](const char*k){return qBound(0.,s.value(prefix+QString(k)).toDouble()/100,1.);};
        bool active=false;
        for(const char *key:{"blemishRemoval","skinSoftening","textureRecovery","faceShine","skinUnify","eyeBags","darkCircles","wrinkles","iris","eyeWhites","catchlight","teethWhitening","lipstick","blush","eyeliner","eyeshadow","eyebrow"})active |= val(key)!=0;
        if(!active)return;
        cv::Mat skin=mask(a,"faceSkin",rgb.size(),face),under=mask(a,"underEyes",rgb.size(),face);
        repair(rgb,skin,val("blemishRemoval"),0);repair(rgb,skin,val("wrinkles"),3);
        if(val("skinSoftening")||val("textureRecovery")){cv::Mat base=smooth(rgb,1.3*scale),low;cv::bilateralFilter(base,low,9,.08,4*scale);cv::Mat effect=low+(rgb-base)*.55;blend(rgb,effect,skin,val("skinSoftening")*.8);blend(rgb,rgb+(rgb-base)*.8,skin,val("textureRecovery"));}
        if(val("skinUnify")){cv::Mat lab;cv::cvtColor(rgb,lab,cv::COLOR_RGB2Lab);auto average=cv::mean(lab,skin>.7);auto low=smooth(lab,18*scale);for(int y=0;y<lab.rows;++y){auto p=lab.ptr<cv::Vec3f>(y);auto l=low.ptr<cv::Vec3f>(y);for(int x=0;x<lab.cols;++x){p[x][1]+=(average[1]-l[x][1])*.55;p[x][2]+=(average[2]-l[x][2])*.55;}}cv::Mat effect;cv::cvtColor(lab,effect,cv::COLOR_Lab2RGB);blend(rgb,effect,skin,val("skinUnify"));}
        if(val("faceShine")){cv::Mat gray;cv::cvtColor(rgb,gray,cv::COLOR_RGB2GRAY);cv::Mat bright=(gray-.65)*3;cv::max(bright,0,bright);blend(rgb,smooth(rgb,7*scale),skin.mul(bright),val("faceShine"));tone(rgb,skin.mul(bright),-.25*val("faceShine"),0,0,0);}
        if(val("eyeBags"))blend(rgb,smooth(rgb,4*scale),under,val("eyeBags")*.6);tone(rgb,under,.4*val("darkCircles"),0,-8*val("darkCircles"),0);
        tone(rgb,mask(a,"iris",rgb.size(),face),.22*val("iris"),35*val("iris"),45*val("iris"),0);
        tone(rgb,mask(a,"whites",rgb.size(),face),.4*val("eyeWhites"),0,-65*val("eyeWhites"),0);
        tone(rgb,mask(a,"teeth",rgb.size(),face),.35*val("teethWhitening"),0,-70*val("teethWhitening"),-8*val("teethWhitening"));
        makeup(rgb,mask(a,"lips",rgb.size(),face),val("lipstick"),{.72f,.13f,.24f});makeup(rgb,mask(a,"cheeks",rgb.size(),face),val("blush")*.6,{.8f,.30f,.34f});
        makeup(rgb,mask(a,"eyelids",rgb.size(),face),val("eyeshadow")*.7,{.38f,.19f,.28f});tone(rgb,mask(a,"brows",rgb.size(),face),-.8*val("eyebrow"),20*val("eyebrow"),0,0);
        if(val("eyeliner")){cv::Mat eyes=mask(a,"eyes",rgb.size(),face),eroded,edge;cv::erode(eyes,eroded,cv::getStructuringElement(cv::MORPH_ELLIPSE,{qMax(3,int(scale*2)+1),qMax(3,int(scale*2)+1)}));edge=eyes-eroded;tone(rgb,edge,-val("eyeliner"),0,-20,0);}
        if(face>=0&&val("catchlight")){cv::Mat m=cv::Mat::zeros(rgb.size(),CV_32F);const auto&f=a.faces[face];for(int i:{468,473}){auto p=f.landmarks[i];cv::circle(m,{int(p.x()*rgb.cols-scale*1.5),int(p.y()*rgb.rows-scale*1.5)},qMax(1,int(scale*1.6)),cv::Scalar(1),-1);}blend(rgb,cv::Mat(rgb.size(),CV_32FC3,cv::Scalar(.98,.98,.98)),m.mul(mask(a,"iris",rgb.size(),face)),val("catchlight"));}
    };
    // Global facial settings are evaluated per face so catchlights and geometry
    // work for multiple people. Per-face overrides are then applied independently.
    for(int i=0;i<a.faces.size();++i)process(i,{});
    for(int i=0;i<a.faces.size();++i)process(i,QString("face_%1_").arg(i));
    for(int i=0;i<a.faces.size();++i){warpFace(rgb,a,i,s,{});warpFace(rgb,a,i,s,QString("face_%1_").arg(i));}
    const auto val=[&](const char*k){return qBound(0.,s.value(k).toDouble()/100,1.);};
    auto clothes=mask(a,"clothes",rgb.size()),hair=mask(a,"hair",rgb.size()),bg=mask(a,"background",rgb.size());
    repair(rgb,clothes,val("wrinkleRemoval"),3);repair(rgb,clothes,val("lintRemoval"),1);repair(rgb,clothes,val("stainRemoval"),2);
    if(val("hairSmooth"))blend(rgb,smooth(rgb,1.5*scale),hair,val("hairSmooth")*.7);tone(rgb,hair,.15*val("hairShine"),25*val("hairShine"),8*val("hairShine"),0);
    if(val("bgCleanup")){auto low=smooth(rgb,18*scale);blend(rgb,low,bg,val("bgCleanup")*.8);}
    if(val("bgBlur"))blend(rgb,smooth(rgb,qMax(.5,val("bgBlur")*22*scale)),bg,val("bgBlur"));
    if(val("lensBlur")){
        auto depth=mask(a,"depth",rgb.size());if(!a.masks.contains("depth"))throw std::runtime_error("Depth model did not produce a depth map");
        auto subject=mask(a,"subject",rgb.size());double focus=cv::mean(depth,subject>.7)[0];if(cv::countNonZero(subject>.7)==0)focus=depth.at<float>(depth.rows/2,depth.cols/2);
        cv::Mat distance;cv::absdiff(depth,cv::Scalar(focus),distance);cv::Mat accum=rgb.clone();
        for(int level=1;level<=5;++level){int radius=qMax(1,int(level*val("lensBlur")*4*scale));cv::Mat kernel=cv::Mat::zeros(radius*2+1,radius*2+1,CV_32F);cv::circle(kernel,{radius,radius},radius,cv::Scalar(1),-1);kernel/=cv::sum(kernel)[0];cv::Mat weighted=rgb.clone();for(int y=0;y<rgb.rows;++y){auto p=weighted.ptr<cv::Vec3f>(y);auto m=bg.ptr<float>(y);for(int x=0;x<rgb.cols;++x)p[x]*=m[x];}cv::Mat blurred,denom;cv::filter2D(weighted,blurred,-1,kernel);cv::filter2D(bg,denom,-1,kernel);for(int y=0;y<rgb.rows;++y){auto p=blurred.ptr<cv::Vec3f>(y);auto m=denom.ptr<float>(y);for(int x=0;x<rgb.cols;++x)p[x]/=qMax(.001f,m[x]);}cv::Mat m;cv::inRange(distance,cv::Scalar((level-1)*.12),cv::Scalar(level==5?2:level*.12),m);m.convertTo(m,CV_32F,1./255);blend(accum,blurred,m.mul(bg),val("lensBlur"));}rgb=accum;
    }
    for(auto replacement:{std::pair<const char*,const char*>{"skyReplacement","skyFile"},{"backgroundReplacement","backgroundFile"}}){if(!val(replacement.first))continue;QImage asset(s.value(replacement.second).toString());if(asset.isNull())throw std::runtime_error("Choose a replacement image first");asset=asset.scaled(image.size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);asset=asset.copy((asset.width()-image.width())/2,(asset.height()-image.height())/2,image.width(),image.height());cv::Mat region=mask(a,QString(replacement.first)=="skyReplacement"?"sky":"background",rgb.size());if(QString(replacement.first)=="skyReplacement")region=region.mul(bg);blend(rgb,pixels(asset),region,val(replacement.first));}
    for(const auto&name:semanticMaskNames()) {
        double e=s.value("mask_"+name+"_exposure").toDouble(),c=s.value("mask_"+name+"_contrast").toDouble(),sat=s.value("mask_"+name+"_saturation").toDouble(),t=s.value("mask_"+name+"_temperature").toDouble();
        if(e||c||sat||t)tone(rgb,mask(a,name,rgb.size()),e,c,sat,t);
    }
    for(int face=0;face<a.faces.size();++face)for(const QString &name:{QString("faceSkin"),QString("eyes"),QString("lips"),QString("teeth")}) {
        const QString key=QString("face_%1_mask_%2_").arg(face).arg(name);
        double e=s.value(key+"exposure").toDouble(),c=s.value(key+"contrast").toDouble(),sat=s.value(key+"saturation").toDouble(),t=s.value(key+"temperature").toDouble();
        if(e||c||sat||t)tone(rgb,mask(a,name,rgb.size(),face),e,c,sat,t);
    }
    for(int y=0;y<original.height();++y){auto d=reinterpret_cast<QRgba64*>(original.scanLine(y));auto p=rgb.ptr<cv::Vec3f>(y);for(int x=0;x<original.width();++x)d[x]=QRgba64::fromRgba64(qBound(0,qRound(p[x][0]*65535),65535),qBound(0,qRound(p[x][1]*65535),65535),qBound(0,qRound(p[x][2]*65535),65535),d[x].alpha());}
    return original;
}
