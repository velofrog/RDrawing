#define STRICT_R_HEADERS
#include <Rcpp.h>
#include "../platform_specific.h"
#include "win_string.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cmath>

double FontPointSize(const pGEcontext gc) {
  return (gc == NULL ? 10 : gc->ps * gc->cex);
}

int FontPixelHeight(const pGEcontext gc, int dpiY) {
  return MulDiv(static_cast<int>(std::floor(FontPointSize(gc) + 0.5)), dpiY, 72);
}

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

HFONT CreateWindowsFont(const pGEcontext gc, int pixelHeight) {
  HFONT hFont;
  bool bold = (gc == NULL ? false : (gc->fontface == 2 || gc->fontface == 4));
  bool italic = (gc == NULL ? false : (gc->fontface == 3 || gc->fontface == 4));

  hFont = CreateFontA(-pixelHeight, 0, 0, 0,
                      (bold ? FW_BOLD : FW_REGULAR), (italic ? 1 : 0),
                      0, 0, 0, 0, 0, 0, 0, FontFamilyName(gc).c_str());

  if (hFont == NULL) {
    // Fallback to Arial
    Rcpp::Rcerr << "Cannot create font '" << FontFamilyName(gc) << "'. Falling back to Arial\n";
    hFont = CreateFontA(-pixelHeight, 0, 0, 0,
                        (bold ? FW_BOLD : FW_REGULAR), (italic ? 1 : 0),
                        0, 0, 0, 0, 0, 0, 0, "Arial");
    if (hFont == NULL) {
      Rcpp::Rcerr << "Cannot create fallback font (Arial)\n";
      return NULL;
    }
  }

  return hFont;
}

void TextBoundingRect(const pGEcontext gc, const std::string &text, ML_TextBounds &bounds) {
  bounds.ascent = bounds.descent = bounds.width = bounds.height = 0;
  if (text.empty()) return;

  std::wstring wstr;

  if (!UTF8ToWideChar(text, wstr)) {
    Rcpp::Rcerr << "UTF8 conversion error\n";
    return;
  }

  HDC hDC = GetDC(NULL);
  SetMapMode(hDC, MM_TEXT);

  HFONT sysFont, hFont;
  SIZE size = {0};
  TEXTMETRICA tm;

  int dpiY = GetDeviceCaps(hDC, LOGPIXELSY);
  int fontHeight = FontPixelHeight(gc, dpiY);

  hFont = CreateWindowsFont(gc, FontPixelHeight(gc, dpiY));
  if (hFont == NULL) {
    ReleaseDC(NULL, hDC);
    return;
  }

  sysFont = (HFONT)SelectObject(hDC, hFont);

  GetTextExtentPoint32W(hDC, wstr.c_str(), wstr.length(), &size);
  GetTextMetricsA(hDC, &tm);
  if (tm.tmPitchAndFamily & TMPF_TRUETYPE) {
    OUTLINETEXTMETRICA otm = {0};
    otm.otmSize = sizeof(OUTLINETEXTMETRICA);
    if (GetOutlineTextMetricsA(hDC, sizeof(OUTLINETEXTMETRICA), &otm) != 0) {
      SelectObject(hDC, sysFont);
      DeleteObject(hFont);
      fontHeight = otm.otmEMSquare;
      hFont = CreateWindowsFont(gc, fontHeight);
      SelectObject(hDC, hFont);
      GetTextMetricsA(hDC, &tm);
    }
  }

  bounds.ascent = static_cast<double>(tm.tmAscent) / static_cast<double>(fontHeight) * FontPointSize(gc);
  bounds.descent = -static_cast<double>(tm.tmDescent) / static_cast<double>(fontHeight) * FontPointSize(gc);
  bounds.height = bounds.ascent - bounds.descent;
  bounds.width = static_cast<double>(size.cx) / static_cast<double>(GetDeviceCaps(hDC, LOGPIXELSX)) * 72.0f;

  SelectObject(hDC, sysFont);
  DeleteObject(hFont);
  ReleaseDC(NULL, hDC);
}
