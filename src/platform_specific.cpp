#include "platform_specific.h"

void PlatformDeviceDriver::TextBoundingRect(const pGEcontext gc, const std::string &text, const bool UTF8, ML_TextBounds &bounds) {
    std::string fontfamily = PlatformFontFamily(gc);
    bool bold = (gc ? (gc->fontface == 2 || gc->fontface == 4) : false);
    bool italic = (gc ? (gc->fontface == 3 || gc->fontface == 4) : false);

    bounds.ascent = bounds.descent = bounds.width = bounds.height = 0;

    try {
        PlatformTextBoundingRect(fontfamily, bold, italic,
            (gc ? gc->ps * gc->cex : 10),
            text, UTF8, gc->fontface == 5, bounds);
    }

    catch (std::exception& e) {

    }
}
