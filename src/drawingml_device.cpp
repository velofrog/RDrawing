#include <Rcpp.h>
#include <cmath>
#include <algorithm>
#include "drawingml_device.h"
#include "drawingml_xml.h"
#include "platform_specific.h"
#include "zip_container.h"

//' @export
// [[Rcpp::export]]
void DrawingMLDevice(double width = 23.5 / 2.54, double height = 14.5 / 2.54,
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
      throw Rcpp::exception("DrawingMLDevice: Unable to allocation memory");
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

    // RDrawingML can't clip, but if canClip is false then geom_text labels where x or y are Inf are not displayed
    dev->canClip = TRUE;
    dev->canChangeGamma = FALSE;
    dev->canHAdj = 2; // Sometimes this is 1 (devWindows), some device drivers use 2 (devX11, devQuartz)
    dev->startps = pointsize;
    dev->startcol = R_RGBA(0, 0, 0, 255);
    dev->startfill = R_TRANWHITE;
    dev->startlty = LTY_SOLID;
    dev->startfont = 1;
    dev->displayListOn = FALSE; // Unsure. Seems screen's are true, others (png, pdf, etc) are false

    dev->hasTextUTF8 = TRUE;
    dev->wantSymbolUTF8 = TRUE;
    dev->canGenMouseDown = dev->canGenMouseMove = dev->canGenMouseUp = dev->canGenKeybd = FALSE;
    dev->haveTransparency = 2;
    dev->haveTransparentBg = 2;
    dev->haveRaster = 2; // 2=>yes, but do we?
    dev->haveCapture = dev->haveLocator = 1;

    // methods
    dev->activate = DrawingMLDevice_activate;
    dev->close = DrawingMLDevice_close;
    dev->onExit = DrawingMLDevice_onExit;
    dev->newFrameConfirm = DrawingMLDevice_newFrameConfirm;
    dev->newPage = DrawingMLDevice_newPage;
    dev->cap = DrawingMLDevice_cap;
    dev->size = DrawingMLDevice_size;
    dev->mode = DrawingMLDevice_mode;
    dev->clip = DrawingMLDevice_clip;

    dev->metricInfo = DrawingMLDevice_metricInfo;
    dev->strWidth = DrawingMLDevice_strWidth;

    dev->path = DrawingMLDevice_path;
    dev->raster = DrawingMLDevice_raster;
    dev->rect = DrawingMLDevice_rect;
    dev->line = DrawingMLDevice_line;
    dev->circle = DrawingMLDevice_circle;

    dev->deviceSpecific = new ML_Context();

    gdd = GEcreateDevDesc(dev);
    GEaddDevice2(gdd, "DrawingMLDevice");
  } END_SUSPEND_INTERRUPTS;
}

ML_Colour::ML_Colour() {
  red = green = blue = 255;
  alpha = 0;
}

ML_Colour::ML_Colour(int colour) {
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

std::string ML_Colour::str_rgb() const {
  char buffer[24];

  std::snprintf(buffer, sizeof(buffer), "%02X%02X%02X", red, green, blue);
  return buffer;
}

std::string ML_Colour::str_alpha() const {
  return std::to_string(100000 * alpha / 255);
}

std::string ML_LineType_str(ML_LineType lty) {
  switch (lty) {
    case ML_LINE_BLANK: return "blank";
    case ML_LINE_SOLID: return "solid";
    case ML_LINE_DASHED: return "dash";
    case ML_LINE_DOTTED: return "sysDot";
    case ML_LINE_DOTDASH: return "dashDot";
    case ML_LINE_LONGDASH: return "lgDash";
    case ML_LINE_TWODASH: return "lgDashDot";
  }

  return "solid";
}

ML_Attributes::ML_Attributes(const pGEcontext gc) {
  lineColour = gc->col;
  fillColour = gc->fill;
  lineWidth = gc->lwd;
  lineType = static_cast<ML_LineType>(gc->lty);
  lineEnd = static_cast<ML_LineEnd>(gc->lend);
  lineJoin = static_cast<ML_LineJoin>(gc->ljoin);
  lineMitre = gc->lmitre;
  pointSize = gc->ps * gc->cex;
  bold = gc->fontface == 2 || gc->fontface == 4;
  italic = gc->fontface == 3 || gc->fontface == 4;
  font = FontFamilyName(gc);
}

void ML_Context::initialise(double width, double height) {
  objects.clear();
  canvasWidth = width;
  canvasHeight = height;
  id = 2; // id 0=>Canvas, id 1=>MainGroup
}

std::vector<XMLNode> ML_IterateOver(const std::vector<ML_Geom>& objects) {
  std::vector<XMLNode> nodes;

#ifdef __APPLE__
  // visit(...) is unavailable on default macOS target (macosx-version-min=10.13)
  // as std::visit can throw a bad_variant_access, which is only avaiable in macOS >= 10.14
  for (const auto &object : objects) {
    ML_BaseType const* ptr = nullptr;
    switch (object.index()) { // object is a std::variant. Requires hardcoding number of underlying types
    case 0:
      ptr = std::get_if<0>(&object); break;
    case 1:
      ptr = std::get_if<1>(&object); break;
    case 2:
      ptr = std::get_if<2>(&object); break;
    case 3:
      ptr = std::get_if<3>(&object); break;
    }

    if (ptr != nullptr) {
      for (const auto &node : ptr->xml()) {
        nodes.push_back(node);
      }
    }
  }
#else
  for (const auto &object : objects) {
    for (const auto &node : std::visit(
        [](const ML_BaseType& arg) -> std::vector<XMLNode> { return arg.xml(); }, object))
      nodes.push_back(node);
  }
#endif

  return nodes;
}

XMLNode XML_nvSpPr(int id, std::string name = "") {
  return
  XMLNode("a:nvSpPr") <<
    XMLNode("a:cNvPr", {{"id",std::to_string(id)},{"name",name}}) <<
    XMLNode("a:cNvSpPr");
}

XMLNode XML_nvGrpSpPr(int id, std::string name = "") {
  return
  XMLNode("a:nvGrpSpPr") <<
    XMLNode("a:cNvPr", {{"id",std::to_string(id)},{"name",name}}) <<
    XMLNode("a:cNvGrpSpPr");
}

XMLNode XML_xfrm(double x, double y, double width, double height) {
  return
  XMLNode("a:xfrm") <<
    XMLNode("a:off", {{"x",emu::str(x)},{"y",emu::str(y)}}) <<
    XMLNode("a:ext", {{"cx",emu::str(width)},{"cy",emu::str(height)}});
}

XMLNode XML_xfrm_rect(double x1, double y1, double x2, double y2) {
  std::vector<std::pair<std::string, std::string>> attr;
  if (x2 < x1) attr.push_back({"flipH","1"});
  if (y2 < y1) attr.push_back({"flipV","1"});

  XMLNode node("a:xfrm", attr);
  node << XMLNode("a:off", {{"x",emu::str(std::min(x1, x2))},{"y",emu::str(std::min(y1, y2))}});
  node << XMLNode("a:ext", {{"cx",emu::str(std::abs(x2-x1))},{"cy",emu::str(std::abs(y2-y1))}});

  return node;
}

XMLNode XML_xfrm(double x, double y, double width, double height, double ch_x, double ch_y, double ch_width, double ch_height) {
  return
  XMLNode("a:xfrm") <<
    XMLNode("a:off", {{"x",emu::str(x)},{"y",emu::str(y)}}) <<
    XMLNode("a:ext", {{"cx",emu::str(width)},{"cy",emu::str(height)}}) <<
    XMLNode("a:chOff", {{"x",emu::str(ch_x)},{"y",emu::str(ch_y)}}) <<
    XMLNode("a:chExt", {{"cx",emu::str(ch_width)},{"cy",emu::str(ch_height)}});
}

XMLNode XML_solidFill(ML_Colour colour) {
  return
  XMLNode("a:solidFill") << (
      XMLNode("a:srgbClr", {{"val",colour.str_rgb()}}) <<
        XMLNode("a:alpha", {{"val",colour.str_alpha()}}));
}

XMLNode XML_ln(double width, ML_LineType linetype, ML_Colour colour) {
  if (linetype == ML_LineType::ML_LINE_BLANK) {
    linetype = ML_LineType::ML_LINE_SOLID;
    width = 0;
  }

  return
  XMLNode("a:ln", {{"w",emu::str(width)}}) <<
    XML_solidFill(colour) <<
      XMLNode("a:prstDash", {{"val",ML_LineType_str(linetype)}});
}

XMLNode XML_ln(ML_Attributes attributes) {
  return XML_ln(attributes.lineWidth, attributes.lineType, attributes.lineColour);
}

std::vector<XMLNode> ML_Group::xml() const {
  if (interior_objects.size() == 0) {
    return {
      XML_nvGrpSpPr(id, name),
      XMLNode("a:grpSpPr") <<
        XML_xfrm(0, 0, width, height, 0, 0, width, height)
    };
  }

  XMLNode node =
    XMLNode("a:grpSp") <<
      XML_nvGrpSpPr(id, name) <<
        (XMLNode("a:grpSpPr") << XML_xfrm(0, 0, width, height, 0, 0, width, height));

  node = node << ML_IterateOver(interior_objects);

  return {node};
}

std::vector<XMLNode> ML_Rect::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        XML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            XML_xfrm_rect(x0, y0, x1, y1),
            XMLNode("a:prstGeom", {{"prst","rect"}}),
            XML_solidFill(attributes.fillColour),
            XML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> ML_Line::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        XML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            XML_xfrm_rect(x1, y1, x2, y2),
            XMLNode("a:prstGeom", {{"prst","line"}}),
            XML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> ML_Circle::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        XML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            XML_xfrm(x - radius, y - radius, radius * 2, radius * 2),
            XMLNode("a:prstGeom", {{"prst","ellipse"}}),
            XML_solidFill(attributes.fillColour),
            XML_ln(attributes)
          })
      })
  };
}

void DrawingMLDevice_activate(pDevDesc) {
}

Rboolean DrawingMLDevice_newFrameConfirm(pDevDesc) {
  return TRUE;
}

void DrawingMLDevice_onExit(pDevDesc) {
  Rcpp::Rcout << "_onExit\n";
}

void DrawingMLDevice_close(pDevDesc dd) {
  Rcpp::Rcout << "_close\n";

  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  std::vector<ML_Geom> canvas;
  canvas.push_back(ML_Group(0, context->canvasWidth, context->canvasHeight, "Canvas"));
  canvas.push_back(ML_Group(1, 0, 0, context->canvasWidth, context->canvasHeight, "MainGroup", context->objects));

  Rcpp::Rcout << MLContainer_Drawing(canvas);

  try {
    ZipAndSendToClipboard(
      {
        {"[Content_Types].xml", MLContainer_Content_Types()},
        {"_rels/.rels", MLContainer_Relationships()},
        {"clipboard/drawings/_rels/drawing1.xml.rels", MLContainer_DrawingRelationships()},
        {"clipboard/theme/theme1.xml", MLContainer_Theme1()},
        {"clipboard/drawings/drawing1.xml", MLContainer_Drawing(canvas)}
      }
    );

    delete context;
    dd->deviceSpecific = NULL;
  }

  catch (const std::exception& e) {
    throw Rcpp::exception(e.what());
  }
}

void DrawingMLDevice_mode(int mode, pDevDesc dd) {
}

void DrawingMLDevice_clip(double x0, double x1, double y0, double y1, pDevDesc dd) {
  // No clipping in DrawingML
}

SEXP DrawingMLDevice_cap(pDevDesc dd) {
  Rcpp::Rcout << "_cap\n";
  return R_NilValue;
}

void DrawingMLDevice_size(double *left, double *right, double *top, double *bottom, pDevDesc dd) {
  Rcpp::Rcout << "_size (" << dd->left << ", " << dd->top << ". " << dd->right << ", " << dd->bottom << ")\n";
  *left = dd->left;
  *right = dd->right;
  *top = dd->top;
  *bottom = dd->bottom;
}

void DrawingMLDevice_newPage(const pGEcontext gc, pDevDesc dd) {
  Rcpp::Rcout << "_newPage (" << dd->right << ", " << dd->bottom << ")\n";
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  context->initialise(dd->right, dd->bottom);
}

void DrawingMLDevice_metricInfo(int c, const pGEcontext gc, double *ascent, double *descent, double *width, pDevDesc dd) {
  Rcpp::Rcout << "_metricInfo(c: " << c << ")\n";
  *ascent = *descent = *width = 0.0;
  if (gc == NULL) return;

  std::string str;

  if (c >= 0 && c <= ((mbcslocale && gc->fontface != 5) ? 127 : 255)) {
    str.assign(1, (char)c);
  } else {
    if (c < 0) c = -c;
    str.assign(1, (char)c);
  }

  ML_Bounds bounds;
  TextBoundingRect(gc, str, bounds);

  *descent = -bounds.y;
  *ascent = bounds.height + bounds.y;
  *width = bounds.width;
}

double DrawingMLDevice_strWidth(const char *str, const pGEcontext gc, pDevDesc pp) {
  Rcpp::Rcout << "_strWidth\n";

  if (str == NULL) return 0;
  if (strlen(str) == 0) return 0;

  ML_Bounds bounds;
  TextBoundingRect(gc, str, bounds);

  return bounds.width;
}

void DrawingMLDevice_raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd) {
  throw Rcpp::exception("Raster operation not supported");
}

void DrawingMLDevice_path(double *x, double *y, int nploy, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd) {
  throw Rcpp::exception("Path operation not supported");
}

void DrawingMLDevice_rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd) {
  Rcpp::Rcout << "_rect\n";

  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  context->objects.push_back(ML_Rect(context->id++, x0, y0, x1, y1, gc));
}

void DrawingMLDevice_line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd) {
  Rcpp::Rcout << "_line\n";

  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  context->objects.push_back(ML_Line(context->id++, x1, y1, x2, y2, gc));
}

void DrawingMLDevice_circle(double x, double y, double r, const pGEcontext gc, pDevDesc dd) {
  Rcpp::Rcout << "_circle\n";

  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  context->objects.push_back(ML_Circle(context->id++, x, y, r, gc));
}
