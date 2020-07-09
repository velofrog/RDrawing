#include <Rcpp.h>
#include <cmath>
#include <algorithm>
#include "drawingml_device.h"
#include "drawingml_xml.h"
#include "zip_container.h"
#define UTF_CPP_CPLUSPLUS 201703L
#include "utf8.h"

const double DrawingML_FontHeightScalar = 277.0 / 90.0 / 2.54;

//' @export
// [[Rcpp::export]]
void OfficeGraphicDevice(double width = 23.5 / 2.54, double height = 14.5 / 2.54,
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

    // ROfficeGraphics can't clip, but if canClip is false then geom_text labels where x or y are Inf are not displayed
    dev->canClip = TRUE;
    dev->canChangeGamma = FALSE;
    dev->canHAdj = 2; // Sometimes this is 1 (devWindows), some device drivers use 2 (devX11, devQuartz)
    dev->startps = pointsize;
    dev->startcol = R_RGBA(0, 0, 0, 255);
    dev->startfill = R_TRANWHITE;
    dev->startlty = LTY_SOLID;
    dev->startfont = 1;
    dev->displayListOn = FALSE; // Unsure. Seems screen's are true, others (png, pdf, etc) are false

    //dev->hasTextUTF8 = TRUE;
    //dev->wantSymbolUTF8 = TRUE;
    dev->canGenMouseDown = dev->canGenMouseMove = dev->canGenMouseUp = dev->canGenKeybd = FALSE;
    dev->haveTransparency = 2;
    dev->haveTransparentBg = 2;
    dev->haveRaster = 2; // 2=>yes, but unimplemented
    dev->haveCapture = dev->haveLocator = 1;
    dev->hasTextUTF8 = TRUE;
#ifdef __APPLE__
    dev->wantSymbolUTF8 = TRUE;
#else
    dev->wantSymbolUTF8 = TRUE;
#endif

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
    dev->strWidth = DrawingMLDevice_strWidth; // For Symbol on MacOS
    dev->strWidthUTF8 = DrawingMLDevice_strWidth;

    dev->raster = DrawingMLDevice_raster;
    dev->rect = DrawingMLDevice_rect;
    dev->line = DrawingMLDevice_line;
    dev->circle = DrawingMLDevice_circle;
    dev->path = NULL;
    dev->polyline = DrawingMLDevice_polyline;
    dev->polygon = DrawingMLDevice_polygon;
    dev->text = DrawingMLDevice_text;
    dev->textUTF8 = DrawingMLDevice_text;

    dev->deviceSpecific = new ML_Context();

    gdd = GEcreateDevDesc(dev);
    GEaddDevice2(gdd, "OfficeGraphicsDevice");
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

std::string ML_Alignment::str_alignment() const {
  if (align <= 0.4) return "l";
  if (align <= 0.6) return "ctr";
  return "r";
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

ML_Attributes::ML_Attributes(const PlatformDeviceDriver &platform, const pGEcontext gc) {
  if (gc) {
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
    font = platform.PlatformFontFamily(gc);
  }
}

ML_Context::ML_Context() {
  platform = NewPlatformDeviceDriver();
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
  // Hence this pretty awful switch
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
      case 4:
        ptr = std::get_if<4>(&object); break;
      case 5:
        ptr = std::get_if<5>(&object); break;
      case 6:
        ptr = std::get_if<6>(&object); break;
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

XMLNode XML_xfrm_rect(double x1, double y1, double x2, double y2, double rotate) {
  std::vector<std::pair<std::string, std::string>> attr;
  if (x2 < x1) attr.push_back({"flipH","1"});
  if (y2 < y1) attr.push_back({"flipV","1"});
  attr.push_back({"rot",std::to_string(static_cast<int>(-60000.0 * rotate))});

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

XMLNode XML_pt(double x, double y) {
  return XMLNode("a:pt", {{"x",emu::str(x)},{"y",emu::str(y)}});
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

std::vector<XMLNode> ML_Polyline::xml() const {
  if (points.size() < 2) return {};

  auto minmax_x = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.first < rhs.first;});
  double x0 = minmax_x.first->first;
  double x1 = minmax_x.second->first;

  auto minmax_y = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.second < rhs.second;});
  double y0 = minmax_y.first->second;
  double y1 = minmax_y.second->second;

  XMLNode pathnode =
    XMLNode("a:path", {{"w",emu::str(x1-x0)},{"h",emu::str(y1-y0)}}) <<
      (XMLNode("a:moveTo") << XML_pt(points[0].first-x0, points[0].second-y0));

  for (std::size_t idx=1; idx<points.size(); idx++)
    pathnode = pathnode << (XMLNode("a:lnTo") << XML_pt(points[idx].first-x0, points[idx].second-y0));

  return {
    XMLNode("a:sp") <<
      XMLNodes({
        XML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            XML_xfrm(x0, y0, x1-x0, y1-y0),
            XMLNode("a:custGeom") << XMLNode("a:avLst") << XMLNode("a:gdLst") <<
            XMLNode("a:ahLst") << XMLNode("a:cxnLst") << (XMLNode("a:pathLst") << pathnode),
            XML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> ML_Polygon::xml() const {
  if (points.size() < 2) return {};

  auto minmax_x = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.first < rhs.first;});
  double x0 = minmax_x.first->first;
  double x1 = minmax_x.second->first;

  auto minmax_y = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.second < rhs.second;});
  double y0 = minmax_y.first->second;
  double y1 = minmax_y.second->second;

  XMLNode pathnode =
    XMLNode("a:path", {{"w",emu::str(x1-x0)},{"h",emu::str(y1-y0)}}) <<
      (XMLNode("a:moveTo") << XML_pt(points[0].first-x0, points[0].second-y0));

  for (std::size_t idx=1; idx<points.size(); idx++)
    pathnode = pathnode << (XMLNode("a:lnTo") << XML_pt(points[idx].first-x0, points[idx].second-y0));

  pathnode = pathnode << XMLNode("a:close");

  return {
      XMLNode("a:sp") <<
        XMLNodes({
          XML_nvSpPr(id, name),
          XMLNode("a:spPr") <<
            XMLNodes({
              XML_xfrm(x0, y0, x1-x0, y1-y0),
              XMLNode("a:custGeom") << XMLNode("a:avLst") << XMLNode("a:gdLst") <<
              XMLNode("a:ahLst") << XMLNode("a:cxnLst") << (XMLNode("a:pathLst") << pathnode),
              XML_solidFill(attributes.fillColour),
              XML_ln(attributes)
            })
        })
    };
}

std::vector<XMLNode> ML_Text::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        (
          XMLNode("a:nvSpPr") <<
            XMLNode("a:cNvPr", {{"id",std::to_string(id)},{"name",name}}) <<
            XMLNode("a:cNvSpPr", {{"txBox","1"}})
        ),
        XMLNode("a:spPr") <<
          XMLNodes({
            XML_xfrm_rect(x0, y0, x1, y1, attributes.rotation),
            XMLNode("a:prstGeom", {{"prst","rect"}}),
            XMLNode("a:noFill")
          }),
        XMLNode("a:txSp") <<
          XMLNodes({
            XMLNode("a:txBody") <<
              XMLNodes({
                XMLNode("a:bodyPr", {{"wrap","none"},{"lIns","0"},{"tIns","0"},
                                     {"rIns","0"},{"bIns","0"},{"anchor","b"},
                                     {"anchorCtr","1"}}) <<
                  XMLNode("a:spAutoFit"),
                XMLNode("a:p") <<
                  XMLNodes({
                    XMLNode("a:pPr", {{"algn",align.str_alignment()}}),
                    XMLNode("a:r") <<
                      XMLNodes({
                        XMLNode("a:rPr", {{"sz",std::to_string(static_cast<int>(100.0 * attributes.pointSize))},
                                          {"b",(attributes.bold ? "1" : "0")},{"i",(attributes.italic ? "1":"0")},
                                          {"dirty","0"}}) <<
                          XMLNodes({
                            XML_solidFill(attributes.lineColour), // use colour, not fill colour for text
                            XMLNode("a:latin", {{"typeface",attributes.font}}),
                            XMLNode("a:cs", {{"typeface",attributes.font}})
                          }),
                        XMLNode("a:t", text)
                      }) // a:r
                  }) // a:p
              }),  // a:txBody
            XMLNode("a:useSpRect")
          }) // a:txSp
      }) // a:sp
    };
}


void DrawingMLDevice_activate(pDevDesc) {
}

Rboolean DrawingMLDevice_newFrameConfirm(pDevDesc) {
  return TRUE;
}

void DrawingMLDevice_onExit(pDevDesc) {
}

void DrawingMLDevice_close(pDevDesc dd) {
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  std::vector<ML_Geom> canvas;
  canvas.push_back(ML_Group(0, context->canvasWidth, context->canvasHeight, "Canvas"));
  canvas.push_back(ML_Group(1, 0, 0, context->canvasWidth, context->canvasHeight, "MainGroup", context->objects));

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

void DrawingMLDevice_mode(int mode, pDevDesc dd) {
}

void DrawingMLDevice_clip(double x0, double x1, double y0, double y1, pDevDesc dd) {
  // No clipping in DrawingML
}

SEXP DrawingMLDevice_cap(pDevDesc dd) {
  return R_NilValue;
}

void DrawingMLDevice_size(double *left, double *right, double *top, double *bottom, pDevDesc dd) {
  *left = dd->left;
  *right = dd->right;
  *top = dd->top;
  *bottom = dd->bottom;
}

void DrawingMLDevice_newPage(const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;

  context->initialise(dd->right, dd->bottom);
}

void DrawingMLDevice_metricInfo(int c, const pGEcontext gc, double *ascent, double *descent, double *width, pDevDesc dd) {
  *ascent = *descent = *width = 0.0;
  if (gc == NULL) return;

  std::string str;

  // hasUTF8 set to TRUE. c < 0 is a unicode point
  if (c < 0) {
    utf8::append(-c, str);
  } else
    str.push_back(c);

  ML_TextBounds bounds;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;
  context->platform->TextBoundingRect(gc, str, false, bounds);

  *descent = -bounds.descent * DrawingML_FontHeightScalar;
  *ascent = bounds.ascent;
  *width = bounds.width;
  Rcpp::Rcout << "Metrics [Ascent: " << *ascent << "; Descent: " << *descent << "]. [Width: " << *width << "]\n";
}

double DrawingMLDevice_strWidth(const char *str, const pGEcontext gc, pDevDesc dd) {
  if (str == NULL) return 0;
  if (strlen(str) == 0) return 0;

  ML_TextBounds bounds;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return 0;
  if (context->platform == nullptr) return 0;

  context->platform->TextBoundingRect(gc, str, true, bounds);

  return bounds.width;
}

void DrawingMLDevice_raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd) {
  throw Rcpp::exception("Raster operation not supported");
}

void DrawingMLDevice_rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  context->objects.push_back(ML_Rect(context->id++, x0, y0, x1, y1, {*context->platform, gc}));
}

void DrawingMLDevice_line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  context->objects.push_back(ML_Line(context->id++, x1, y1, x2, y2, {*context->platform, gc}));
}

void DrawingMLDevice_circle(double x, double y, double r, const pGEcontext gc, pDevDesc dd) {
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  context->objects.push_back(ML_Circle(context->id++, x, y, r, {*context->platform, gc}));
}

void DrawingMLDevice_polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
  if (n < 2) return;
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  std::vector<std::pair<double, double>> points;
  for (int idx=0; idx<n; idx++)
    points.push_back({x[idx], y[idx]});

  context->objects.push_back(ML_Polyline(context->id++, points, {*context->platform, gc}));
}

void DrawingMLDevice_polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
  if (n < 2) return;
  if (dd == NULL) return;
  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  std::vector<std::pair<double, double>> points;
  for (int idx=0; idx<n; idx++)
    points.push_back({x[idx], y[idx]});

  context->objects.push_back(ML_Polygon(context->id++, points, {*context->platform, gc}));
}

void DrawingMLDevice_text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd) {
  if (str == NULL) return;
  if (strlen(str) == 0) return;
  if (gc == NULL) return;
  if ((gc->ps * gc->cex) < 0.5) {
    // DrawingML doesn't appear to handle very small text
    Rcpp::Rcerr << "Text too small (pointsize: " << gc->ps * gc->cex << ")\n";
    return;
  }

  ML_Context *context = (ML_Context *)dd->deviceSpecific;
  if (context == NULL) return;
  if (context->platform == nullptr) return;

  ML_TextBounds bounds;
  context->platform->TextBoundingRect(gc, str, true, bounds);

  if (bounds.empty()) return;

  Rcpp::Rcout << "TextBounds [Ascent: " << bounds.ascent << "; Descent: " << bounds.descent << "]. [Width: " << bounds.width << "; Height: " << bounds.height << "]\n";

  bounds.height *= DrawingML_FontHeightScalar;

  ML_Attributes attributes(*context->platform, gc);
  attributes.rotation = rot;
  attributes.hAdjustment = 0; // We'll incorporate adjustment into the bounding rect

  // Adjust y for font descent
  y = y - bounds.descent * DrawingML_FontHeightScalar;
  // DrawingML gets a unrotated bounding rect and the angle of rotation
  double d = bounds.width * hadj;
  double rotateAngle = rot * M_PI / 180.0f;
  double diagAngle = atan2((0.5*bounds.height), (0.5*bounds.width - d));
  double hyp = sqrt(std::pow(0.5*bounds.height, 2) + std::pow(0.5*bounds.width-d, 2));
  double cx = cos(rotateAngle + diagAngle) * hyp;
  double cy = -sin(rotateAngle + diagAngle) * hyp;

  double tx = x + cx - 0.5*bounds.width;    // top left point of unrotated rect
  double ty = y + cy - 0.5*bounds.height;   //

  // Do we need any special handling for symbol (fontface == 5)?

  context->objects.push_back(ML_Text(context->id++, tx, ty, tx + bounds.width, ty + bounds.height,
                                     str, hadj, attributes));
}
