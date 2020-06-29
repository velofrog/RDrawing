#include <Rcpp.h>
#include "../platform_specific.h"

std::string FontFamilyName(const pGEcontext gc) {
  if (gc == NULL) {
    return "Arial";
  } else if (gc->fontface == 5) {
    return "Symbol";
  } else if (strlen(gc->fontfamily) == 0) {
    return "Arial";
  } else if (strcmp(gc->fontfamily, "serif") == 0) {
    return "Times New Roman";
  } else if (strcmp(gc->fontfamily, "sans") == 0) {
    return "Arial";
  } else if (strcmp(gc->fontfamily, "mono") == 0) {
    return "Courier New";
  }

  return gc->fontfamily;
}

std::string FontName(const pGEcontext gc) {
  if (gc == NULL) {
    return "Arial";
  } else {
    std::string fontname = FontFamilyName(gc);
    if (gc->fontface == 2)
      fontname += " Bold";
    else if (gc->fontface == 3)
      fontname += " Italic";
    else if (gc->fontface == 4)
      fontname += " Bold Oblique";

    return fontname;
  }
}

CTFontRef CreateFont(const pGEcontext gc) {
  CTFontRef font = NULL;

  CFStringRef cf_fontname = CFStringCreateWithCString(kCFAllocatorDefault,
                                                      FontName(gc).c_str(), kCFStringEncodingUTF8);

  if (cf_fontname == NULL) return NULL;

  font = CTFontCreateWithName(cf_fontname, (gc == NULL ? 10 : gc->ps * gc->cex), NULL);
  CFRelease(cf_fontname);

  return font;
}

void TextBoundingRect(const pGEcontext gc, const std::string &text, ML_TextBounds &bounds) {
  bounds.ascent = 0;
  bounds.descent = 0;
  bounds.width = 12 * text.size();
  bounds.height = gc->ps;
}
