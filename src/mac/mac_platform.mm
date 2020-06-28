#include <Rcpp.h>
#import <Foundation/Foundation.h>
#include "../platform_specific.h"

std::string FontFamilyName(const pGEcontext gc) {
  if (gc == NULL) {
    return "Helvetica";
  } else if (gc->fontface == 5) {
    return "Symbol";
  } else if (strlen(gc->fontfamily) == 0) {
    return "Helvetica";
  } else if (strcmp(gc->fontfamily, "serif") == 0) {
    if (gc->fontface <= 1) return "Times Roman"; else return "Times";
  } else if (strcmp(gc->fontfamily, "sans") == 0) {
    return "Helvetica";
  } else if (strcmp(gc->fontfamily, "mono") == 0) {
    return "Courier";
  }

  return gc->fontfamily;
}

std::string FontName(const pGEcontext gc) {
  if (gc == NULL) {
    return "Helvetica";
  } else {
    std::string fontname = FontFamilyName(gc);
    if (gc->fontface == 2)
      fontname += " Bold";
    else if (gc->fontface == 3)
      fontname += " Oblique";
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
  bounds.ascent = bounds.descent = bounds.width = bounds.height = 0;

  if (text.empty()) return;

  CFStringRef cf_text = CFStringCreateWithCString(kCFAllocatorDefault,
    text.c_str(), (gc == NULL || gc->fontface != 5 ? kCFStringEncodingUTF8 : kCFStringEncodingMacSymbol));

  if (cf_text == NULL) return;

  CTFontRef font = CreateFont(gc);
  if (font == NULL) {
    Rcpp::Rcerr << "Failed to create font\n";
    CFRelease(cf_text);
    return;
  }

  CFIndex length = CFStringGetLength(cf_text);
  if (length <= 0) {
    CFRelease(font);
    CFRelease(cf_text);
    return;
  }

  UniChar *characters = (UniChar *)malloc(sizeof(UniChar) * length);
  if (characters == NULL) {
    Rcpp::Rcerr << "Memory allocation failed in TextBoundingRect\n";
    CFRelease(font);
    CFRelease(cf_text);
    return;
  }

  CGGlyph *glyphs = (CGGlyph *)malloc(sizeof(CGGlyph) * length);
  if (glyphs == NULL) {
    Rcpp::Rcerr << "Memory allocation failed in TextBoundingRect\n";
    free(characters);
    CFRelease(font);
    CFRelease(cf_text);
    return;
  }

  CFStringGetCharacters(cf_text, CFRangeMake(0, length), characters);
  CTFontGetGlyphsForCharacters(font, characters, glyphs, length);
  double totalAdvances = CTFontGetAdvancesForGlyphs(font, kCTFontOrientationDefault, glyphs, NULL, length);

  bounds.ascent = CTFontGetAscent(font);
  bounds.descent = -CTFontGetDescent(font);
  bounds.width = totalAdvances;
  bounds.height = bounds.ascent - bounds.descent;

  free(glyphs);
  free(characters);
  CFRelease(font);
  CFRelease(cf_text);
}
