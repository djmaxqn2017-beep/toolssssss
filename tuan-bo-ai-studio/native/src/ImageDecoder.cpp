#include <QColorSpace>
#include "ImageDecoder.h"
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QSet>
#include <libraw/libraw.h>
#include <libheif/heif.h>
#include <memory>

namespace {
constexpr qint64 maxPixels = 128LL * 1024 * 1024;
QImage scaled(QImage image, int maxSide) {
    if (maxSide > 0 && qMax(image.width(), image.height()) > maxSide)
        return image.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}
}

QImage decodeImage(const QString &path, int maxSide, QString *error) {
    const auto fail = [error](const QString &reason) { if (error) *error = reason; return QImage(); };
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return fail(file.errorString());
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QSet<QString> rawTypes = {"cr2", "cr3", "nef", "nrw", "arw", "sr2", "srf", "dng", "raf", "rw2", "orf", "pef", "raw", "3fr", "fff", "iiq", "rwl", "srw"};
    if (rawTypes.contains(ext)) {
        if (file.size() > 1024LL * 1024 * 1024) return fail("RAW exceeds 1 GiB input limit");
        QByteArray bytes = file.readAll(); // QFile preserves Unicode Windows paths.
        LibRaw raw;
        int rc = raw.open_buffer(bytes.data(), static_cast<size_t>(bytes.size()));
        if (rc != LIBRAW_SUCCESS) return fail(QString::fromUtf8(libraw_strerror(rc)));
        if (qint64(raw.imgdata.sizes.width) * raw.imgdata.sizes.height > maxPixels)
            return fail("RAW exceeds 128 megapixel decode limit");
        raw.imgdata.params.use_camera_wb = 1;
        raw.imgdata.params.output_color = 1; // sRGB
        raw.imgdata.params.output_bps = 16;
        raw.imgdata.params.half_size = maxSide > 0 && qMax(raw.imgdata.sizes.width, raw.imgdata.sizes.height) > maxSide * 2;
        rc = raw.unpack();
        if (rc == LIBRAW_SUCCESS) rc = raw.dcraw_process();
        if (rc != LIBRAW_SUCCESS) return fail(QString::fromUtf8(libraw_strerror(rc)));
        auto deleter = [](libraw_processed_image_t *p) { if (p) LibRaw::dcraw_clear_mem(p); };
        std::unique_ptr<libraw_processed_image_t, decltype(deleter)> bitmap(raw.dcraw_make_mem_image(&rc), deleter);
        if (!bitmap || rc != LIBRAW_SUCCESS || bitmap->type != LIBRAW_IMAGE_BITMAP || bitmap->colors != 3 || bitmap->bits != 16)
            return fail("RAW RGB conversion failed");
        QImage image(bitmap->width,bitmap->height,QImage::Format_RGBX64);
        const auto *src=reinterpret_cast<const quint16*>(bitmap->data);
        for(int y=0;y<image.height();++y){auto *dst=reinterpret_cast<QRgba64*>(image.scanLine(y));for(int x=0;x<image.width();++x){auto n=(y*image.width()+x)*3;dst[x]=QRgba64::fromRgba64(src[n],src[n+1],src[n+2],65535);}}
        image.setColorSpace(QColorSpace(QColorSpace::SRgb));
        return scaled(image, maxSide);
    }
    if (ext == "heic" || ext == "heif") {
        if (file.size() > 1024LL * 1024 * 1024) return fail("HEIF exceeds 1 GiB input limit");
        const QByteArray bytes = file.readAll();
        // vcpkg ships HEVC decoder plugins beside the application in the package.
        static const bool pluginsLoaded = [] {
            const QByteArray folder = QDir::toNativeSeparators(QCoreApplication::applicationDirPath()).toUtf8();
            heif_load_plugins(folder.constData(), nullptr, nullptr, 0);
            return true;
        }();
        Q_UNUSED(pluginsLoaded);
        std::unique_ptr<heif_context, decltype(&heif_context_free)> context(heif_context_alloc(), heif_context_free);
        if (!context) return fail("HEIF context allocation failed");
        heif_error result = heif_context_read_from_memory_without_copy(context.get(), bytes.constData(), static_cast<size_t>(bytes.size()), nullptr);
        if (result.code != heif_error_Ok) return fail(QString::fromUtf8(result.message));
        heif_image_handle *handlePtr = nullptr;
        result = heif_context_get_primary_image_handle(context.get(), &handlePtr);
        if (result.code != heif_error_Ok) return fail(QString::fromUtf8(result.message));
        std::unique_ptr<heif_image_handle, decltype(&heif_image_handle_release)> handle(handlePtr, heif_image_handle_release);
        if (qint64(heif_image_handle_get_width(handle.get())) * heif_image_handle_get_height(handle.get()) > maxPixels)
            return fail("HEIF exceeds 128 megapixel decode limit");
        heif_image *imagePtr = nullptr;
        result = heif_decode_image(handle.get(), &imagePtr, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);
        if (result.code != heif_error_Ok) return fail(QString::fromUtf8(result.message));
        std::unique_ptr<heif_image, decltype(&heif_image_release)> decoded(imagePtr, heif_image_release);
        int stride = 0;
        const uint8_t *data = heif_image_get_plane_readonly(decoded.get(), heif_channel_interleaved, &stride);
        if (!data) return fail("HEIF RGB plane missing");
        QImage image(data, heif_image_get_width(decoded.get(), heif_channel_interleaved),
                     heif_image_get_height(decoded.get(), heif_channel_interleaved), stride, QImage::Format_RGBA8888);
        return scaled(image.copy(), maxSide);
    }
    QImageReader reader(&file);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);
    const QSize size = reader.size();
    if (qint64(size.width()) * size.height() > maxPixels) return fail("Image exceeds 128 megapixel decode limit");
    if (maxSide > 0 && size.isValid() && qMax(size.width(), size.height()) > maxSide) {
        QSize target = size;
        target.scale(maxSide, maxSide, Qt::KeepAspectRatio);
        reader.setScaledSize(target);
    }
    QImage image = reader.read();
    if (image.isNull()) return fail(reader.errorString());
    return scaled(image, maxSide);
}
