#include <Rcpp.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <cmath>
#include "../platform_specific.h"
#include "../utf8.h"

struct FontDetails {
  std::string fontfamily;
  std::string name;
  std::string style;
  std::string file;
  bool bold;
  bool italic;

  FontDetails() {
    bold = false;
    italic = false;
  }
};

struct UnixDeviceDriver: PlatformDeviceDriver {
  FT_Library library;
  FT_Face face;
  FontDetails details;

  public:
  UnixDeviceDriver();
  virtual ~UnixDeviceDriver();

  virtual std::string PlatformFontFamily(const pGEcontext gc) const;
  virtual bool PlatformTextBoundingRect(const std::string &family, const bool bold, const bool italic, const double pointsize,
                                        const std::string &text, bool UTF8,
                                        ML_TextBounds &bounds);
  private:
  void LoadFont(const std::string &family, const bool bold, const bool italic);

};

std::unique_ptr<PlatformDeviceDriver> NewPlatformDeviceDriver() {
  return std::make_unique<UnixDeviceDriver>();
}

UnixDeviceDriver::UnixDeviceDriver() {
  library = nullptr;
  face = nullptr;

  if (FT_Init_FreeType(&library) != FT_Err_Ok)
    throw std::runtime_error("Failed to initialise FreeType library");
}

UnixDeviceDriver::~UnixDeviceDriver() {
  if (face) FT_Done_Face(face);
  if (library) FT_Done_FreeType(library);
}

std::string UnixDeviceDriver::PlatformFontFamily(const pGEcontext gc) const {
  if (gc == NULL) return "Arial";

  if (gc->fontface == 5) {
    return "Symbol"; // or OpenSymbol
  } else if (strlen(gc->fontfamily) == 0) {
    return "Arial";
  } else if (strcmp(gc->fontfamily, "serif") == 0) {
    return "Times New Roman";
  } else if (strcmp(gc->fontfamily, "sans") == 0) {
    return "Arial";
  } else if (strcmp(gc->fontfamily, "mono") == 0) {
    return "Courier";
  }

  return gc->fontfamily;
}


void UnixDeviceDriver::LoadFont(const std::string &family, const bool bold, const bool italic) {
  // Check if font face is already loaded
  if ((family == details.fontfamily) && (bold == details.bold) && (italic == details.italic) && (face)) return;

  // Remove old face if its set
  if (face) FT_Done_Face(face);
  face = nullptr;

  // Use FontConfig to find match
  FcInit();
  FcConfig *config = FcInitLoadConfigAndFonts();
  FcPattern *pattern = FcPatternCreate();
  FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)family.c_str());
  if (bold) FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_BOLD);
  if (italic) FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ITALIC);
  FcConfigSubstitute(config, pattern, FcMatchPattern);
  FcDefaultSubstitute(pattern);

  FcResult result;
  FcPattern *font = FcFontMatch(config, pattern, &result);
  if (font) {
    FcChar8 *file, *fontfamily, *style, *name;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
        FcPatternGetString(font, FC_FAMILY, 0, &fontfamily) == FcResultMatch &&
        FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch &&
        FcPatternGetString(font, FC_FULLNAME, 0, &name) == FcResultMatch) {
      details.file.assign((char *)file);
      details.fontfamily.assign((char *)fontfamily);
      details.bold = bold;
      details.italic = italic;
      details.name.assign((char *)name);

      if (FT_New_Face(library, (char *)file, 0, &face) != FT_Err_Ok) {
        face = nullptr;
        throw std::runtime_error("Failed to load font");
      }
    }
  }

  FcPatternDestroy(font);
  FcPatternDestroy(pattern);
  FcConfigDestroy(config);
  FcFini();
}

bool UnixDeviceDriver::PlatformTextBoundingRect(const std::string &family, const bool bold, const bool italic, const double pointsize,
                                                const std::string &text, const bool UTF8, ML_TextBounds &bounds) {

  bounds.ascent = bounds.descent = bounds.width = bounds.height = 0;

  try {
    LoadFont(family, bold, italic);
  }

  catch (std::exception &e) {
    return false;
  }

  if (face == nullptr) return false;

  // Set DPI = 72 => pixels = pointsize
  FT_Set_Char_Size(face, 0, std::floor(pointsize * 64 + 0.5), 72, 72);

  std::string::const_iterator b = text.begin();
  std::string::const_iterator e = text.end();
  double total_advance = 0;
  int glyph_index;

  while (b != e) {
    uint32_t charcode;
    try {
      if (UTF8) {
        charcode = utf8::next(b, e);
      } else {
        charcode = static_cast<uint8_t>(*b);
      }
    }

    catch (std::exception &e) {
      break;
    }

    glyph_index = FT_Get_Char_Index(face, charcode);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT) == FT_Err_Ok) {
      total_advance += face->glyph->advance.x >> 6; // x units are 1/64ths, need to divide by 64 to convert into pixels
    }
  }

  bounds.ascent = static_cast<double>(face->ascender) / static_cast<double>(face->units_per_EM) * pointsize;
  bounds.descent = static_cast<double>(face->descender) / static_cast<double>(face->units_per_EM) * pointsize;
  bounds.height = bounds.ascent - bounds.descent;
  bounds.width = total_advance;

  return true;
}
