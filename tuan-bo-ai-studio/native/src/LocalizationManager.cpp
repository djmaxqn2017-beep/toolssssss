#include "LocalizationManager.h"

#include <QHash>
#include <QSettings>

namespace {
struct TrPair { const char *vi; const char *en; };

const QHash<QString, TrPair> &dictionary() {
    static const QHash<QString, TrPair> d = {
        {"app.name", {"TBRetoch", "TBRetoch"}},
        {"nav.library", {"Thư viện", "Library"}},
        {"nav.edit", {"Chỉnh sửa", "Edit"}},
        {"nav.compare", {"So sánh", "Compare"}},
        {"nav.ai", {"AI", "AI"}},
        {"nav.sync", {"Đồng bộ", "Sync"}},
        {"nav.export", {"Xuất ảnh", "Export"}},
        {"action.addImages", {"+ Thêm ảnh", "+ Add Images"}},
        {"action.importImages", {"Nhập ảnh", "Import Images"}},
        {"action.copy", {"Sao chép", "Copy"}},
        {"action.paste", {"Dán", "Paste"}},
        {"action.reset", {"Đặt lại", "Reset"}},
        {"action.undo", {"Hoàn tác", "Undo"}},
        {"action.redo", {"Làm lại", "Redo"}},
        {"action.before", {"Trước", "Before"}},
        {"action.after", {"Sau", "After"}},
        {"action.fit", {"Vừa khung hình", "Fit"}},
        {"action.export", {"Xuất ảnh", "Export"}},
        {"action.rotateLeft", {"Xoay trái", "Rotate Left"}},
        {"action.rotateRight", {"Xoay phải", "Rotate Right"}},
        {"action.flipH", {"Lật ngang", "Flip Horizontal"}},
        {"action.flipV", {"Lật dọc", "Flip Vertical"}},
        {"dialog.addImages", {"Thêm ảnh vào TBRetoch", "Add images to TBRetoch"}},
        {"dialog.chooseExportFolder", {"Chọn thư mục xuất ảnh", "Choose export folder"}},
        {"filter.images", {"Ảnh (*.jpg *.jpeg *.png *.webp *.bmp *.tif *.tiff)", "Images (*.jpg *.jpeg *.png *.webp *.bmp *.tif *.tiff)"}},
        {"filter.all", {"Tất cả (*.*)", "All files (*.*)"}},
        {"viewer.noImage", {"Chưa chọn ảnh", "No image selected"}},
        {"viewer.nativeOffline", {"Không gian làm việc native GPU • Offline", "Native GPU workspace • Offline"}},
        {"viewer.imagesCount", {"%1 ảnh", "%1 images"}},
        {"status.ready", {"GPU Preview • CPU Export • Offline", "GPU Preview • CPU Export • Offline"}},
        {"status.preview", {"Đang tạo ảnh xem trước…", "Generating previews…"}},
        {"status.export", {"Đang xuất từ ảnh gốc…", "Exporting from original image…"}},
        {"status.exportDone", {"Đã xuất %1 × %2 • %3 MB", "Exported %1 × %2 • %3 MB"}},
        {"error.noExportFolder", {"Chưa chọn thư mục xuất.", "No export folder selected."}},
        {"error.readOriginal", {"Không đọc được ảnh gốc để xuất.", "Could not read the original image for export."}},
        {"error.exportFailed", {"Xuất ảnh thất bại.", "Export failed."}},
        {"tab.color", {"Màu", "Color"}},
        {"tab.portrait", {"Chân dung", "Portrait"}},
        {"tab.background", {"Phông nền", "Background"}},
        {"tab.clothing", {"Trang phục", "Clothing"}},
        {"tab.lighting", {"Ánh sáng", "Lighting"}},
        {"tab.crop", {"Cắt", "Crop"}},
        {"tab.details", {"Chi tiết", "Details"}},
        {"tab.aiTools", {"Công cụ AI", "AI Tools"}},
        {"section.basic", {"Điều chỉnh cơ bản", "Basic Adjustments"}},
        {"section.toneCurve", {"Đường cong tông màu", "Tone Curve"}},
        {"section.hsl", {"HSL", "HSL"}},
        {"section.colorGrading", {"Phân hạng màu", "Color Grading"}},
        {"section.export", {"Xuất ảnh", "Export"}},
        {"section.face", {"Khuôn mặt", "Face"}},
        {"section.skin", {"Da", "Skin"}},
        {"section.eyes", {"Mắt", "Eyes"}},
        {"section.teethMouth", {"Răng & Miệng", "Teeth & Mouth"}},
        {"section.makeup", {"Trang điểm AI", "AI Makeup"}},
        {"section.hair", {"Tóc", "Hair"}},
        {"section.body", {"Hình thể", "Body"}},
        {"section.background", {"Phông nền", "Background"}},
        {"section.clothing", {"Trang phục", "Clothing"}},
        {"section.mask", {"Mặt nạ", "Mask"}},
        {"control.exposure", {"Phơi sáng", "Exposure"}},
        {"control.contrast", {"Tương phản", "Contrast"}},
        {"control.highlights", {"Vùng sáng", "Highlights"}},
        {"control.shadows", {"Vùng tối", "Shadows"}},
        {"control.whites", {"Điểm trắng", "Whites"}},
        {"control.blacks", {"Điểm đen", "Blacks"}},
        {"control.temperature", {"Nhiệt độ màu", "Temperature"}},
        {"control.tint", {"Sắc độ", "Tint"}},
        {"control.vibrance", {"Độ rực", "Vibrance"}},
        {"control.saturation", {"Độ bão hòa", "Saturation"}},
        {"control.clarity", {"Độ trong", "Clarity"}},
        {"control.dehaze", {"Khử sương", "Dehaze"}},
        {"control.fade", {"Phai màu", "Fade"}},
        {"control.skinSoftening", {"Làm mềm da", "Skin Softening"}},
        {"control.textureRecovery", {"Khôi phục kết cấu da", "Texture Recovery"}},
        {"control.blemishRemoval", {"Xóa khuyết điểm da", "Blemish Removal"}},
        {"control.acne", {"Mụn", "Acne"}},
        {"control.darkSpots", {"Vết thâm", "Dark Spots"}},
        {"control.freckles", {"Tàn nhang", "Freckles"}},
        {"control.faceShine", {"Giảm bóng dầu", "Face Shine"}},
        {"control.skinUnify", {"Đều màu da", "Skin Tone Unify"}},
        {"control.eyeBags", {"Bọng mắt", "Eye Bags"}},
        {"control.darkCircles", {"Quầng thâm mắt", "Dark Circles"}},
        {"control.wrinkles", {"Nếp nhăn", "Wrinkles"}},
        {"control.doubleChin", {"Nọng cằm", "Double Chin"}},
        {"control.faceWidth", {"Độ rộng khuôn mặt", "Face Width"}},
        {"control.jaw", {"Hàm", "Jaw"}},
        {"control.chin", {"Cằm", "Chin"}},
        {"control.vShape", {"Mặt V-line", "V Shape"}},
        {"control.eyeSize", {"Kích thước mắt", "Eye Size"}},
        {"control.noseWidth", {"Độ rộng mũi", "Nose Width"}},
        {"control.lipSize", {"Kích thước môi", "Lip Size"}},
        {"control.teethWhitening", {"Làm trắng răng", "Teeth Whitening"}},
        {"control.iris", {"Mống mắt", "Iris"}},
        {"control.eyeWhites", {"Lòng trắng mắt", "Eye Whites"}},
        {"control.catchlight", {"Điểm sáng mắt", "Catchlight"}},
        {"control.lipstick", {"Son môi", "Lipstick"}},
        {"control.blush", {"Má hồng", "Blush"}},
        {"control.eyeliner", {"Kẻ mắt", "Eyeliner"}},
        {"control.eyeshadow", {"Phấn mắt", "Eyeshadow"}},
        {"control.eyebrow", {"Chân mày", "Eyebrow"}},
        {"control.hairSmooth", {"Làm mượt tóc", "Hair Smooth"}},
        {"control.flyaway", {"Tóc con", "Flyaway Hair"}},
        {"control.hairVolume", {"Độ phồng tóc", "Hair Volume"}},
        {"control.hairColor", {"Màu tóc", "Hair Color"}},
        {"control.waist", {"Eo", "Waist"}},
        {"control.shoulders", {"Vai", "Shoulders"}},
        {"control.arms", {"Cánh tay", "Arms"}},
        {"control.hips", {"Hông", "Hips"}},
        {"control.legs", {"Chân", "Legs"}},
        {"control.height", {"Chiều cao", "Height"}},
        {"control.posture", {"Tư thế", "Posture"}},
        {"control.wrinkleRemoval", {"Xóa nếp nhăn trang phục", "Clothing Wrinkle Removal"}},
        {"control.lintRemoval", {"Xóa xơ vải", "Lint Removal"}},
        {"control.stainRemoval", {"Giảm vết bẩn", "Stain Reduction"}},
        {"control.bgCleanup", {"Làm sạch phông nền", "Background Cleanup"}},
        {"control.bgBlur", {"Làm mờ phông nền", "Background Blur"}},
        {"control.lensBlur", {"Mờ ống kính", "Lens Blur"}},
        {"control.skyReplacement", {"Thay bầu trời", "Sky Replacement"}},
        {"control.relight", {"Tái tạo ánh sáng", "Relight"}},
        {"control.subjectLight", {"Ánh sáng chủ thể", "Subject Light"}},
        {"control.rimLight", {"Ánh sáng viền", "Rim Light"}},
        {"control.vignette", {"Tối góc", "Vignette"}},
        {"control.sharpen", {"Làm nét", "Sharpen"}},
        {"control.denoise", {"Khử nhiễu", "Denoise"}},
        {"control.upscale", {"Nâng độ phân giải", "Upscale"}},
        {"export.format", {"Định dạng", "Format"}},
        {"export.quality", {"Chất lượng JPEG/WebP", "JPEG/WebP Quality"}},
        {"export.originalResolution", {"Xuất từ ảnh gốc đủ độ phân giải", "Export from full-resolution original"}},
        {"language.label", {"Ngôn ngữ", "Language"}},
        {"language.vi", {"Tiếng Việt", "Vietnamese"}},
        {"language.en", {"Tiếng Anh", "English"}},
        {"info.gpuPreview", {"Xem trước bằng GPU • Xuất từ ảnh gốc", "GPU preview • Export from original"}},
        {"info.semanticPending", {"Đang xây engine semantic thật, không dùng slider giả.", "Building real semantic engine; no fake sliders."}}
    };
    return d;
}
}

LocalizationManager::LocalizationManager(QObject *parent) : QObject(parent) {
    QSettings settings;
    const QString saved = settings.value(QStringLiteral("ui/language"), QStringLiteral("vi")).toString();
    m_language = (saved == QStringLiteral("en")) ? QStringLiteral("en") : QStringLiteral("vi");
}

QString LocalizationManager::language() const { return m_language; }

void LocalizationManager::setLanguage(const QString &language) {
    const QString normalized = (language == QStringLiteral("en")) ? QStringLiteral("en") : QStringLiteral("vi");
    if (normalized == m_language) return;
    m_language = normalized;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/language"), m_language);
    emit languageChanged();
}

QStringList LocalizationManager::availableLanguages() const {
    return {QStringLiteral("vi"), QStringLiteral("en")};
}

QString LocalizationManager::t(const QString &key) const {
    const auto it = dictionary().constFind(key);
    if (it == dictionary().cend()) return key;
    return m_language == QStringLiteral("en")
        ? QString::fromUtf8(it->en)
        : QString::fromUtf8(it->vi);
}
