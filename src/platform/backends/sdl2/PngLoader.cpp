#include "platform/backends/sdl2/PngLoader.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <png.h>

namespace gui_dev {

void FreePngImage(PngImage& image) {
    std::free(image.pixels);
    image.pixels = nullptr;
    image.width = 0;
    image.height = 0;
}

bool DecodePng(const char* path, PngImage& out) {
    out = PngImage{};

    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        std::fprintf(stderr, "[gui_dev] 打不开图片：%s\n", path);
        return false;
    }

    // 先用文件头快速排除非 PNG，避免 libpng 错误回调里的噪音。
    png_byte header[8] = {};
    if (std::fread(header, 1, sizeof(header), fp) != sizeof(header) || png_sig_cmp(header, 0, 8) != 0) {
        std::fprintf(stderr, "[gui_dev] 不是 PNG：%s\n", path);
        std::fclose(fp);
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr) {
        std::fclose(fp);
        return false;
    }
    png_infop info = png_create_info_struct(png);
    if (info == nullptr) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        std::fclose(fp);
        return false;
    }

    // libpng 用 longjmp 报错：这里只持有 C 分配的内存，避免与 C++ 析构交互。
    std::uint8_t* pixels = nullptr;
    png_bytep* rows = nullptr;
    if (setjmp(png_jmpbuf(png)) != 0) {
        std::fprintf(stderr, "[gui_dev] PNG 解码失败：%s\n", path);
        std::free(pixels);
        std::free(rows);
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    const png_uint_32 width = png_get_image_width(png, info);
    const png_uint_32 height = png_get_image_height(png, info);
    const int color_type = png_get_color_type(png, info);
    const int bit_depth = png_get_bit_depth(png, info);

    // 统一转成 8bit RGBA。
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS) != 0) {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    png_read_update_info(png, info);

    if (width == 0 || height == 0) {
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        return false;
    }

    const std::size_t stride = png_get_rowbytes(png, info);
    pixels = static_cast<std::uint8_t*>(std::malloc(stride * height));
    rows = static_cast<png_bytep*>(std::malloc(sizeof(png_bytep) * height));
    if (pixels == nullptr || rows == nullptr) {
        std::free(pixels);
        std::free(rows);
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        return false;
    }
    for (png_uint_32 y = 0; y < height; ++y) {
        rows[y] = pixels + stride * y;
    }

    png_read_image(png, rows);
    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(fp);
    std::free(rows);

    out.pixels = pixels;
    out.width = static_cast<int>(width);
    out.height = static_cast<int>(height);

    // 8bit RGBA 的 PNG 行距恒等于 width*4；如果不是（异常文件），压紧一份。
    const std::size_t tight = static_cast<std::size_t>(width) * 4;
    if (stride != tight) {
        std::uint8_t* compact = static_cast<std::uint8_t*>(std::malloc(tight * height));
        if (compact == nullptr) {
            FreePngImage(out);
            return false;
        }
        for (png_uint_32 y = 0; y < height; ++y) {
            std::memcpy(compact + tight * y, pixels + stride * y, tight);
        }
        std::free(pixels);
        out.pixels = compact;
    }
    return true;
}

} // namespace gui_dev
