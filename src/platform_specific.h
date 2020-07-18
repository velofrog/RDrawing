// Platform Specific Functions
#pragma once
#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/GraphicsEngine.h>
#include <string>
#include <memory>

struct Drawing_TextBounds {
  double width;
  double height;
  double ascent;
  double descent;

  bool empty() { return ((width == 0) || (height == 0));}
};

class PlatformDeviceDriver {
  public:
  PlatformDeviceDriver() {};
  virtual ~PlatformDeviceDriver() {};

  virtual void TextBoundingRect(const pGEcontext gc, const std::string& text, const bool UTF8,
                                Drawing_TextBounds& bounds);

  virtual std::string PlatformFontFamily(const pGEcontext gc) const { return ""; };
  virtual bool PlatformTextBoundingRect(const std::string& family, const bool bold, const bool italic, const double pointsize,
                                        const std::string& text, const bool UTF8, const bool symbol,
                                        Drawing_TextBounds& bounds) { return false; };
};

std::unique_ptr<PlatformDeviceDriver> NewPlatformDeviceDriver();



