#pragma once
#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/GraphicsEngine.h>
#include <cmath>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include "xml.h"
#include "platform_specific.h"

struct emu {
  static std::string str(double v) {
    return std::to_string(std::lround(v * 12700)); // 72 dots per inch, 914400 EMU => one inch
  }
};

struct Drawing_Colour {
  int alpha;
  int red;
  int green;
  int blue;

  Drawing_Colour();
  Drawing_Colour(int colour);
  std::string str_rgb() const;
  std::string str_alpha() const;
};

enum Drawing_LineType {
  DRAWING_LINE_BLANK = -1,
  DRAWING_LINE_SOLID = 0,
  DRAWING_LINE_DASHED = 4 + (4<<4),
  DRAWING_LINE_DOTTED = 1 + (3<<4),
  DRAWING_LINE_DOTDASH = 1 + (3<<4) + (4<<8) + (3<<12),
  DRAWING_LINE_LONGDASH = 7 + (3<<4),
  DRAWING_LINE_TWODASH = 2 + (2<<4) + (6<<8) + (2<<12)
};

std::string Drawing_LineType_str(Drawing_LineType lty);

enum Drawing_LineEnd {
  DRAWING_ROUND_CAP = 1,
  DRAWING_BUTT_CAP = 2,
  DRAWING_SQUARE_CAP = 3
};

enum Drawing_LineJoin {
  DRAWING_ROUND_JOIN = 1,
  DRAWING_MITRE_JOIN = 2,
  DRAWING_BEVEL_JOIN = 3
};

struct Drawing_Attributes {
  Drawing_Colour lineColour;
  Drawing_Colour fillColour;
  double lineWidth;
  Drawing_LineType lineType;
  Drawing_LineEnd lineEnd;
  Drawing_LineJoin lineJoin;
  double lineMitre;

  double pointSize;
  double hAdjustment;
  double rotation;
  bool bold;
  bool italic;
  std::string font;

  Drawing_Attributes() {};
  Drawing_Attributes(const PlatformDeviceDriver &platform, const pGEcontext gc);
};

struct Drawing_Alignment {
  double align;

  Drawing_Alignment() : align(0.5) {}
  Drawing_Alignment(double hjust) : align(hjust) {}
  std::string str_alignment() const;
};

struct Drawing_Geom {
  int id;
  std::string name;
  virtual ~Drawing_Geom() {}

  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Group : Drawing_Geom {
  double x, y;
  double width, height;
  std::vector<std::shared_ptr<Drawing_Geom>> interior_objects;

  Drawing_Group(int id, double width, double height, std::string name = "") {
    this->id = id;
    this->x = 0;
    this->y = 0;
    this->width = width;
    this->height = height;
    this->name = name;
  }

  Drawing_Group(int id, double x, double y, double width, double height, std::string name, std::vector<std::shared_ptr<Drawing_Geom>> &interior_objects) :
    Drawing_Group(id, width, height, name) {
    this->x = x;
    this->y = y;
    this->interior_objects = interior_objects;
  }

  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Rect : Drawing_Geom {
  double x0, y0, x1, y1;
  Drawing_Attributes attributes;

  Drawing_Rect(int id, double x0, double y0, double x1, double y1, Drawing_Attributes attributes) {
    this->id = id;
    this->x0 = x0;
    this->y0 = y0;
    this->x1 = x1;
    this->y1 = y1;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Line : Drawing_Geom {
  double x1, y1, x2, y2;
  Drawing_Attributes attributes;

  Drawing_Line(int id, double x1, double y1, double x2, double y2, Drawing_Attributes attributes) {
    this->id = id;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Circle : Drawing_Geom {
  double x, y;
  double radius;
  Drawing_Attributes attributes;

  Drawing_Circle(int id, double x, double y, double radius, Drawing_Attributes attributes) {
    this->id = id;
    this->x = x;
    this->y = y;
    this->radius = radius;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Polyline : Drawing_Geom {
  std::vector<std::pair<double, double>> points;
  Drawing_Attributes attributes;

  Drawing_Polyline(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) {
    this->id = id;
    this->points = points;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Polygon : Drawing_Geom {
  std::vector<std::pair<double, double>> points;
  Drawing_Attributes attributes;

  Drawing_Polygon(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) {
    this->id = id;
    this->points = points;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Text : Drawing_Geom {
  double x0, y0, x1, y1;
  std::string text;
  Drawing_Alignment align;
  Drawing_Attributes attributes;

  Drawing_Text(int id, double x0, double y0, double x1, double y1, const std::string &text, Drawing_Alignment align, Drawing_Attributes attributes) {
    this->id = id;
    this->x0 = x0;
    this->y0 = y0;
    this->x1 = x1;
    this->y1 = y1;
    this->text = text;
    this->align = align;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const = 0;
};

struct Drawing_Context {
  int id;
  double canvasWidth;
  double canvasHeight;

  std::vector<std::shared_ptr<Drawing_Geom>> objects;
  std::unique_ptr<PlatformDeviceDriver> platform;

  Drawing_Context();
  virtual ~Drawing_Context() {}
  virtual void initialise(double width, double height) = 0;
  virtual void group(int id, double width, double height, std::string name = "") = 0;
  virtual void group(int id, double x, double y, double width, double height, std::string name, std::vector<std::shared_ptr<Drawing_Geom>> &interior_objects) = 0;
  virtual void rect(int id, double x0, double y0, double x1, double y1, Drawing_Attributes attributes) = 0;
  virtual void line(int id, double x1, double y1, double x2, double y2, Drawing_Attributes attributes) = 0;
  virtual void circle(int id, double x, double y, double radius, Drawing_Attributes attributes) = 0;
  virtual void polyline(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) = 0;
  virtual void polygon(int id, std::vector<std::pair<double, double>> &points, Drawing_Attributes attributes) = 0;
  virtual void text(int id, double x0, double y0, double x1, double y1, const std::string &text, Drawing_Alignment align, Drawing_Attributes attributes) = 0;

  virtual std::vector<std::pair<std::string, std::string>> container() = 0;
};

void DrawingDevice_activate(pDevDesc dd);
Rboolean DrawingDevice_newFrameConfirm(pDevDesc dd);
void DrawingDevice_onExit(pDevDesc dd);
void DrawingDevice_mode(int mode, pDevDesc dd);
void DrawingDevice_close(pDevDesc dd);
void DrawingDevice_clip(double x0, double x1, double y0, double y1, pDevDesc dd);
void DrawingDevice_newPage(const pGEcontext gc, pDevDesc dd);
SEXP DrawingDevice_cap(pDevDesc dd);
void DrawingDevice_size(double *left, double *right, double *top, double *bottom, pDevDesc dd);
void DrawingDevice_metricInfo(int c, const pGEcontext gc, double *ascent, double *descent, double *width, pDevDesc dd);
double DrawingDevice_strWidth(const char *str, const pGEcontext gc, pDevDesc pp);
void DrawingDevice_raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
void DrawingDevice_rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd);
void DrawingDevice_line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd);
void DrawingDevice_circle(double x, double y, double r, const pGEcontext gc, pDevDesc dd);
void DrawingDevice_polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
void DrawingDevice_polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
void DrawingDevice_text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
SEXP DrawingDevice_setPattern(SEXP pattern, pDevDesc dd);
void DrawingDevice_releasePattern(SEXP ref, pDevDesc dd);
SEXP DrawingDevice_setClipPath(SEXP path, SEXP ref, pDevDesc dd);
void DrawingDevice_releaseClipPath(SEXP path, pDevDesc dd);
SEXP DrawingDevice_setMask(SEXP path, SEXP ref, pDevDesc dd);
void DrawingDevice_releaseMask(SEXP ref, pDevDesc dd);

