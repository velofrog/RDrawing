// Platform Specific Functions
#pragma once
#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/GraphicsEngine.h>
#include <memory>

struct ML_TextBounds {
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

  void TextBoundingRect(const pGEcontext gc, const std::string &text, bool UTF8, 
                        ML_TextBounds &bounds);

  virtual std::string PlatformFontFamily(const pGEcontext gc) const { return ""; };
  virtual bool PlatformTextBoundingRect(const std::string &family, const bool bold, const bool italic, const double pointsize,
                                        const std::string &text, bool UTF8,
                                        ML_TextBounds &bounds) { return false; };
};

std::unique_ptr<PlatformDeviceDriver> NewPlatformDeviceDriver();



