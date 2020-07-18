#include <Rcpp.h>
#include <cmath>
#include <algorithm>
#include "drawing_device.h"
#include "drawingml.h"
#include "zip_container.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include "utf8.h"

const double Drawing_FontHeightScalar = 277.0 / 90.0 / 2.54;

//' @export
// [[Rcpp::export]]
void DrawingDevice(double width = 23.5 / 2.54, double height = 14.5 / 2.54,
                   double pointsize = 10, std::string font = "Arial") {

  if (std::isnan(width) || (width <= 0)) width = 23.5 / 2.54;
  if (std::isnan(height) || (height <= 0)) height = 14.5 / 2.54;
  if (std::isnan(pointsize) || (pointsize <= 0)) pointsize = 10;

  R_GE_checkVersionOrDie(R_GE_version);
  R_CheckDeviceAvailable();

  BEGIN_SUSPEND_INTERRUPTS {
    pGEDevDesc gdd;
    pDevDesc dev;
    const double dpi = 72.0;

    dev = (pDevDesc) calloc(1, sizeof(DevDesc));
    if (dev == NULL) {
      throw Rcpp::exception("OfficeGraphicDevice: Unable to allocation memory");
    }

    dev->left = dev->clipLeft = 0;
    dev->top = dev->clipTop = 0;
    dev->right = dev->clipRight = width * dpi;
    dev->bottom = dev->clipBottom = height * dpi;

    // From R manual
    dev->xCharOffset = 0.49;
    dev->yCharOffset = 1.0 / 3.0;
    dev->yLineBias = 0.2;

    dev->ipr[0] = 1.0 / dpi;
    dev->ipr[1] = 1.0 / dpi;
    dev->cra[0] = 0.9 * pointsize;
    dev->cra[1] = 1.2 * pointsize;

    dev->gamma = dev->startgamma = 1.0;

    // We can't clip, but if canClip is false then geom_text labels where x or y are Inf are not displayed
    dev->canClip = TRUE;
    dev->canChangeGamma = FALSE;
    dev->canHAdj = 2; // Some device drivers use 1 (devWindows), some device drivers use 2 (devX11, devQuartz)
    dev->startps = pointsize;
    dev->startcol = R_RGBA(0, 0, 0, 255);
    dev->startfill = R_TRANWHITE;
    dev->startlty = LTY_SOLID;
    dev->startfont = 1;
    dev->displayListOn = FALSE; // Unsure. Seems screen's are true, others (png, pdf, etc) are false

    dev->canGenMouseDown = dev->canGenMouseMove = dev->canGenMouseUp = dev->canGenKeybd = FALSE;
    dev->haveTransparency = 2;
    dev->haveTransparentBg = 2;
    dev->haveRaster = 2; // 2=>yes, but unimplemented
    dev->haveCapture = dev->haveLocator = 1;
    dev->hasTextUTF8 = TRUE;
    dev->wantSymbolUTF8 = TRUE; // not sure. 

    // methods
    dev->activate = DrawingDevice_activate;
    dev->close = DrawingDevice_close;
    dev->onExit = DrawingDevice_onExit;
    dev->newFrameConfirm = DrawingDevice_newFrameConfirm;
    dev->newPage = DrawingDevice_newPage;
    dev->cap = DrawingDevice_cap;
    dev->size = DrawingDevice_size;
    dev->mode = DrawingDevice_mode;
    dev->clip = DrawingDevice_clip;

    dev->metricInfo = DrawingDevice_metricInfo;
    dev->strWidth = DrawingDevice_strWidth; // For Symbol on MacOS
    dev->strWidthUTF8 = DrawingDevice_strWidth;

    dev->raster = DrawingDevice_raster;
    dev->rect = DrawingDevice_rect;
    dev->line = DrawingDevice_line;
    dev->circle = DrawingDevice_circle;
    dev->path = NULL;
    dev->polyline = DrawingDevice_polyline;
    dev->polygon = DrawingDevice_polygon;
    dev->text = DrawingDevice_text;
    dev->textUTF8 = DrawingDevice_text;

    dev->deviceSpecific = new DrawingML_Context();

    gdd = GEcreateDevDesc(dev);
    GEaddDevice2(gdd, "DrawingDevice");
  } END_SUSPEND_INTERRUPTS;
}

Drawing_Colour::Drawing_Colour() {
  red = green = blue = 255;
  alpha = 0;
}

Drawing_Colour::Drawing_Colour(int colour) {
  if (colour == NA_INTEGER) {
    red = green = blue = 255;
    alpha = 0;
  } else {
    red = R_RED(colour);
    green = R_GREEN(colour);
    blue = R_BLUE(colour);
    alpha = R_ALPHA(colour);
  }
}

std::string Drawing_Colour::str_rgb() const {
  char buffer[24];

  std::snprintf(buffer, sizeof(buffer), "%02X%02X%02X", red, green, blue);
  return buffer;
}

std::string Drawing_Colour::str_alpha() const {
  return std::to_string(100000 * alpha / 255);
}

std::string Drawing_Alignment::str_alignment() const {
  if (align <= 0.4) return "l";
  if (align <= 0.6) return "ctr";
  return "r";
}

std::string Drawing_LineType_str(Drawing_LineType lty) {
  switch (lty) {
    case DRAWING_LINE_BLANK: return "blank";
    case DRAWING_LINE_SOLID: return "solid";
    case DRAWING_LINE_DASHED: return "dash";
    case DRAWING_LINE_DOTTED: return "sysDot";
    case DRAWING_LINE_DOTDASH: return "dashDot";
    case DRAWING_LINE_LONGDASH: return "lgDash";
    case DRAWING_LINE_TWODASH: return "lgDashDot";
  }

  return "solid";
}

Drawing_Attributes::Drawing_Attributes(const PlatformDeviceDriver &platform, const pGEcontext gc) {
  if (gc) {
    lineColour = gc->col;
    fillColour = gc->fill;
    lineWidth = gc->lwd;
    lineType = static_cast<Drawing_LineType>(gc->lty);
    lineEnd = static_cast<Drawing_LineEnd>(gc->lend);
    lineJoin = static_cast<Drawing_LineJoin>(gc->ljoin);
    lineMitre = gc->lmitre;
    pointSize = gc->ps * gc->cex;
    bold = gc->fontface == 2 || gc->fontface == 4;
    italic = gc->fontface == 3 || gc->fontface == 4;
    font = platform.PlatformFontFamily(gc);
  }
}

Drawing_Context::Drawing_Context() {
  platform = NewPlatformDeviceDriver();
}

void Drawing_Context::initialise(double width, double height) {
  objects.clear();
  canvasWidth = width;
  canvasHeight = height;
  id = 2; // id 0=>Canvas, id 1=>MainGroup
}

void DrawingDevice_activate(pDevDesc) {
}

Rboolean DrawingDevice_newFrameConfirm(pDevDesc) {
  return TRUE;
}

void DrawingDevice_onExit(pDevDesc) {
}

void DrawingDevice_close(pDevDesc dd) {
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  try {
    ZipAndSendToClipboard(context->container());

    // explicitly delete, and set to NULL
    // otherwise R will try to free dd->deviceSpecific...
    delete context;
    dd->deviceSpecific = NULL;
  }

  catch (const std::exception& e) {
    if (dd->deviceSpecific) {
      delete context;
      dd->deviceSpecific = NULL;
    }

    throw Rcpp::exception(e.what());
  }
}

void DrawingDevice_mode(int mode, pDevDesc dd) {
}

void DrawingDevice_clip(double x0, double x1, double y0, double y1, pDevDesc dd) {
  // No clipping in DrawingML
}

SEXP DrawingDevice_cap(pDevDesc dd) {
  return R_NilValue;
}

void DrawingDevice_size(double *left, double *right, double *top, double *bottom, pDevDesc dd) {
  *left = dd->left;
  *right = dd->right;
  *top = dd->top;
  *bottom = dd->bottom;
}

void DrawingDevice_newPage(const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  context->initialise(dd->right, dd->bottom);
}

void DrawingDevice_metricInfo(int c, const pGEcontext gc, double *ascent, double *descent, double *width, pDevDesc dd) {
  *ascent = *descent = *width = 0.0;
  if (gc == NULL) return;

  std::string str;

  // Because hasUTF8 is set to TRUE, when c < 0 it's a unicode point
  if (c < 0) {
    Rcpp::Rcout << "Was asked about a unicode character -- " << -c << "\n";
    utf8::append(-c, str);
  } else
    str.push_back(c);

  Drawing_TextBounds bounds;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;
  context->platform->TextBoundingRect(gc, str, true, bounds);

  *descent = -bounds.descent * Drawing_FontHeightScalar;
  *ascent = bounds.ascent;
  *width = bounds.width;
}

double DrawingDevice_strWidth(const char *str, const pGEcontext gc, pDevDesc dd) {
  if (str == NULL) return 0;
  if (strlen(str) == 0) return 0;

  Drawing_TextBounds bounds;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return 0;
  if (context->platform == nullptr) return 0;

  context->platform->TextBoundingRect(gc, str, true, bounds);

  return bounds.width;
}

void DrawingDevice_raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd) {
  throw Rcpp::exception("Raster operation not supported");
}

void DrawingDevice_rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  context->rect(context->id++, x0, y0, x1, y1, {*context->platform, gc});
}

void DrawingDevice_line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  context->line(context->id++, x1, y1, x2, y2, {*context->platform, gc});
}

void DrawingDevice_circle(double x, double y, double r, const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  context->circle(context->id++, x, y, r, {*context->platform, gc});
}

void DrawingDevice_polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
  if (n < 2) return;
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  std::vector<std::pair<double, double>> points;
  for (int idx=0; idx<n; idx++)
    points.push_back({x[idx], y[idx]});

  context->polyline(context->id++, points, {*context->platform, gc});
}

void DrawingDevice_polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
  if (n < 2) return;
  if (dd == NULL) return;
  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  std::vector<std::pair<double, double>> points;
  for (int idx=0; idx<n; idx++)
    points.push_back({x[idx], y[idx]});

  context->polygon(context->id++, points, {*context->platform, gc});
}

void DrawingDevice_text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd) {
  if (str == NULL) return;
  if (strlen(str) == 0) return;
  if (gc == NULL) return;
  if ((gc->ps * gc->cex) < 0.5) {
    // Small text causes issues
    Rcpp::Rcerr << "Text too small (pointsize: " << gc->ps * gc->cex << ")\n";
    return;
  }

  Drawing_Context *context = (Drawing_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  Drawing_TextBounds bounds;
  context->platform->TextBoundingRect(gc, str, true, bounds);

  if (bounds.empty()) return;

  bounds.height *= Drawing_FontHeightScalar;

  Drawing_Attributes attributes(*context->platform, gc);
  attributes.rotation = rot;
  attributes.hAdjustment = 0; // We'll incorporate adjustment into the bounding rect

  // Adjust y for font descent
  y = y - bounds.descent * Drawing_FontHeightScalar;
  // DrawingML gets a unrotated bounding rect and the angle of rotation
  double d = bounds.width * hadj;
  double rotateAngle = rot * M_PI / 180.0f;
  double diagAngle = atan2((0.5*bounds.height), (0.5*bounds.width - d));
  double hyp = sqrt(std::pow(0.5*bounds.height, 2) + std::pow(0.5*bounds.width - d, 2));
  double cx = cos(rotateAngle + diagAngle) * hyp;
  double cy = -sin(rotateAngle + diagAngle) * hyp;

  double tx = x + cx - 0.5*bounds.width;    // top left point of unrotated rect
  double ty = y + cy - 0.5*bounds.height;   //

  // Do we need any special handling for symbol (fontface == 5)?
  if (gc->fontface == 5) {
    Rcpp::Rcout << "Asking for symbol. Char = " << str << "  [" << static_cast<int>(str[0]) << "]\n";
  }

  context->text(context->id++, tx, ty, tx + bounds.width, ty + bounds.height, str, hadj, attributes);
}
