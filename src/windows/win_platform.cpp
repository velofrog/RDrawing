#include <Rcpp.h>
#include "../platform_specific.h"

std::string FontFamilyName(const pGEcontext gc) {
    return "Arial";
}

std::string FontName(const pGEcontext gc) {
    return "Arial";
}

void TextBoundingRect(const pGEcontext gc, const std::string &text, ML_TextBounds &bounds) {
  bounds.ascent = 0;
  bounds.descent = 0;
  bounds.width = 12 * text.size();
  bounds.height = gc->ps;
}
