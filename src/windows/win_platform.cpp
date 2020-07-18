#define STRICT_R_HEADERS
#include <Rcpp.h>
#include "../platform_specific.h"
#include "win_string.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cmath>

struct WindowsDeviceDriver: PlatformDeviceDriver {
public:
  WindowsDeviceDriver();
  virtual ~WindowsDeviceDriver();

  virtual std::string PlatformFontFamily(const pGEcontext gc) const;
  virtual bool PlatformTextBoundingRect(const std::string& family, const bool bold, const bool italic, const double pointsize,
                                        const std::string& text, const bool UTF8, const bool symbol,
                                        DD_TextBounds& bounds);
private:
  HFONT CreateWindowsFont(const std::string& family, const bool bold, const bool italic, int pixelHeight);

};

std::unique_ptr<PlatformDeviceDriver> NewPlatformDeviceDriver() {
  return std::make_unique<WindowsDeviceDriver>();
}

WindowsDeviceDriver::WindowsDeviceDriver() {
}

WindowsDeviceDriver::~WindowsDeviceDriver() {
}

std::string WindowsDeviceDriver::PlatformFontFamily(const pGEcontext gc) const {
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

HFONT WindowsDeviceDriver::CreateWindowsFont(const std::string& family, const bool bold, const bool italic, int pixelHeight) {
  HFONT hFont;
  hFont = CreateFontA(-pixelHeight, 0, 0, 0,
                      (bold ? FW_BOLD : FW_REGULAR), (italic ? 1 : 0),
                      0, 0, 0, 0, 0, 0, 0, family.c_str());

  if (hFont == NULL) {
    // Fallback to Arial
    Rcpp::Rcerr << "Cannot create font '" << family << "'. Falling back to Arial\n";
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

bool WindowsDeviceDriver::PlatformTextBoundingRect(const std::string& family, const bool bold, const bool italic, const double pointsize,
                                                   const std::string& text, const bool UTF8, const bool symbol,
                                                   DD_TextBounds& bounds) {

  bounds.ascent = bounds.descent = bounds.width = bounds.height = 0;
  if (text.empty()) return true;

  std::wstring wstr;

  if (UTF8) {
    if (!UTF8ToWideChar(text, wstr)) {
      Rcpp::Rcerr << "UTF8 conversion error\n";
      return false;
    }
  } else {
    if (!AnsiToWideChar(text, wstr)) {
      Rcpp::Rcerr << "Windows default ANSI to wide string conversion error\n";
      return false;
    }
  }

  HDC hDC = GetDC(NULL);
  SetMapMode(hDC, MM_TEXT);

  HFONT sysFont, hFont;
  SIZE size = {0};
  TEXTMETRICA tm;

  int dpiY = GetDeviceCaps(hDC, LOGPIXELSY);
  int fontHeight = MulDiv(static_cast<int>(std::floor(pointsize + 0.5)), dpiY, 72);

  hFont = CreateWindowsFont(family, bold, italic, fontHeight);
  if (hFont == NULL) {
    ReleaseDC(NULL, hDC);
    return false;
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
      hFont = CreateWindowsFont(family, bold, italic, fontHeight);
      SelectObject(hDC, hFont);
      GetTextMetricsA(hDC, &tm);
    }
  }

  bounds.ascent = static_cast<double>(tm.tmAscent) / static_cast<double>(fontHeight) * pointsize;
  bounds.descent = -static_cast<double>(tm.tmDescent) / static_cast<double>(fontHeight) * pointsize;
  bounds.height = bounds.ascent - bounds.descent;
  bounds.width = static_cast<double>(size.cx) / static_cast<double>(GetDeviceCaps(hDC, LOGPIXELSX)) * 72.0f;

  SelectObject(hDC, sysFont);
  DeleteObject(hFont);
  ReleaseDC(NULL, hDC);

  return true;
}
