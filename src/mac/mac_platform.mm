#include <Rcpp.h>
#import <Foundation/Foundation.h>
#include "../platform_specific.h"

struct MacOSDeviceDriver: PlatformDeviceDriver {
public:
  MacOSDeviceDriver();
  virtual ~MacOSDeviceDriver();

  virtual std::string PlatformFontFamily(const pGEcontext gc) const;
  virtual bool PlatformTextBoundingRect(const std::string& family, const bool bold, const bool italic,
                                        const double pointsize, const std::string& text, const bool UTF8,
                                        const bool symbol, Drawing_TextBounds& bounds);
private:
  CTFontRef CreateMacOSFont(const std::string& family, const bool bold, const bool italic, int pointsize);
  std::string MacOSFontName(const std::string& fontfamily, const bool bold, const bool italic);

};

std::unique_ptr<PlatformDeviceDriver> NewPlatformDeviceDriver() {
  return std::make_unique<MacOSDeviceDriver>();
}

MacOSDeviceDriver::MacOSDeviceDriver() {

}

MacOSDeviceDriver::~MacOSDeviceDriver() {

}

std::string MacOSDeviceDriver::PlatformFontFamily(const pGEcontext gc) const {
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

std::string MacOSDeviceDriver::MacOSFontName(const std::string& fontfamily, const bool bold, const bool italic) {
  std::string name = fontfamily;

  if (bold && italic) {
    name += " Bold Oblique";
  } else if (bold) {
    name += " Bold";
  } else if (italic) {
    name += " Oblique";
  }

  return name;
}

CTFontRef MacOSDeviceDriver::CreateMacOSFont(const std::string& family, const bool bold, const bool italic, int pointsize) {
  CTFontRef font = NULL;

  CFStringRef cf_fontname = CFStringCreateWithCString(kCFAllocatorDefault,
    MacOSFontName(family, bold, italic).c_str(), kCFStringEncodingUTF8);

  if (cf_fontname == NULL) return NULL;

  font = CTFontCreateWithName(cf_fontname, pointsize, NULL);
  CFRelease(cf_fontname);

  return font;
}


bool MacOSDeviceDriver::PlatformTextBoundingRect(const std::string& family, const bool bold, const bool italic,
                                                 const double pointsize, const std::string& text, const bool UTF8,
                                                 const bool symbol, Drawing_TextBounds& bounds) {
  bounds.ascent = bounds.descent = bounds.width = bounds.height = 0;

  if (text.empty()) return false;

  CFStringRef cf_text = CFStringCreateWithCString(kCFAllocatorDefault,
    text.c_str(), (false ? kCFStringEncodingMacSymbol : kCFStringEncodingUTF8));

  if (cf_text == NULL) return false;

  CTFontRef font = CreateMacOSFont(family, bold, italic, pointsize);
  if (font == NULL) {
    Rcpp::Rcerr << "Failed to create font\n";
    CFRelease(cf_text);
    return false;
  }

  CFIndex length = CFStringGetLength(cf_text);
  if (length <= 0) {
    CFRelease(font);
    CFRelease(cf_text);
    return false;
  }

  UniChar *characters = (UniChar *)malloc(sizeof(UniChar) * length);
  if (characters == NULL) {
    Rcpp::Rcerr << "Memory allocation failed in PlatformTextBoundingRect\n";
    CFRelease(font);
    CFRelease(cf_text);
    return false;
  }

  CGGlyph *glyphs = (CGGlyph *)malloc(sizeof(CGGlyph) * length);
  if (glyphs == NULL) {
    Rcpp::Rcerr << "Memory allocation failed in PlatformTextBoundingRect\n";
    free(characters);
    CFRelease(font);
    CFRelease(cf_text);
    return false;
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

  return true;
}
