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
#include "xml.h"

struct emu {
  static std::string str(double v) {
    return std::to_string(std::lround(v * 12700)); // 72 dots per inch, 914400 EMU => one inch
  }
};

struct ML_TextBounds {
  double width;
  double height;
  double ascent;
  double descent;

  bool empty() { return ((width == 0) || (height == 0));}
};

struct ML_Colour {
  int alpha;
  int red;
  int green;
  int blue;

  ML_Colour();
  ML_Colour(int colour);
  std::string str_rgb() const;
  std::string str_alpha() const;
};

enum ML_LineType {
  ML_LINE_BLANK = -1,
  ML_LINE_SOLID = 0,
  ML_LINE_DASHED = 4 + (4<<4),
  ML_LINE_DOTTED = 1 + (3<<4),
  ML_LINE_DOTDASH = 1 + (3<<4) + (4<<8) + (3<<12),
  ML_LINE_LONGDASH = 7 + (3<<4),
  ML_LINE_TWODASH = 2 + (2<<4) + (6<<8) + (2<<12)
};

std::string ML_LineType_str(ML_LineType lty);

enum ML_LineEnd {
  ML_ROUND_CAP = 1,
  ML_BUTT_CAP = 2,
  ML_SQUARE_CAP = 3
};

enum ML_LineJoin {
  ML_ROUND_JOIN = 1,
  ML_MITRE_JOIN = 2,
  ML_BEVEL_JOIN = 3
};

struct ML_Attributes {
  ML_Colour lineColour;
  ML_Colour fillColour;
  double lineWidth;
  ML_LineType lineType;
  ML_LineEnd lineEnd;
  ML_LineJoin lineJoin;
  double lineMitre;

  double pointSize;
  double hAdjustment;
  double rotation;
  bool bold;
  bool italic;
  std::string font;

  ML_Attributes() {};
  ML_Attributes(const pGEcontext gc);
};

struct ML_Group;
struct ML_Line;
struct ML_Rect;
struct ML_Circle;
struct ML_Polyline;
struct ML_Polygon;
struct ML_Text;

using ML_Geom = std::variant<ML_Group, ML_Line, ML_Rect, ML_Circle, ML_Polyline, ML_Polygon, ML_Text>;

struct ML_BaseType {
  int id;
  std::string name;

  virtual std::vector<XMLNode> xml() const = 0;
};

struct ML_Group : ML_BaseType {
  double x, y;
  double width, height;
  std::vector<ML_Geom> interior_objects;

  ML_Group(int id, double width, double height, std::string name = "") {
    this->id = id;
    this->x = 0;
    this->y = 0;
    this->width = width;
    this->height = height;
    this->name = name;
  }

  ML_Group(int id, double x, double y, double width, double height, std::string name, std::vector<ML_Geom> &interior_objects) :
    ML_Group(id, width, height, name) {
    this->x = x;
    this->y = y;
    this->interior_objects = interior_objects;
  }

  virtual std::vector<XMLNode> xml() const;
};

struct ML_Rect : ML_BaseType {
  double x0, y0, x1, y1;
  ML_Attributes attributes;

  ML_Rect(int id, double x0, double y0, double x1, double y1, ML_Attributes attributes) {
    this->id = id;
    this->x0 = x0;
    this->y0 = y0;
    this->x1 = x1;
    this->y1 = y1;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const;
};

struct ML_Line : ML_BaseType {
  double x1, y1, x2, y2;
  ML_Attributes attributes;

  ML_Line(int id, double x1, double y1, double x2, double y2, ML_Attributes attributes) {
    this->id = id;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const;
};

struct ML_Circle : ML_BaseType {
  double x, y;
  double radius;
  ML_Attributes attributes;

  ML_Circle(int id, double x, double y, double radius, ML_Attributes attributes) {
    this->id = id;
    this->x = x;
    this->y = y;
    this->radius = radius;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const;
};

struct ML_Polyline : ML_BaseType {
  std::vector<std::pair<double, double>> points;
  ML_Attributes attributes;

  ML_Polyline(int id, std::vector<std::pair<double, double>> &points, ML_Attributes attributes) {
    this->id = id;
    this->points = points;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const;
};

struct ML_Polygon : ML_BaseType {
  std::vector<std::pair<double, double>> points;
  ML_Attributes attributes;

  ML_Polygon(int id, std::vector<std::pair<double, double>> &points, ML_Attributes attributes) {
    this->id = id;
    this->points = points;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const;
};

struct ML_Text : ML_BaseType {
  double x0, y0, x1, y1;
  std::string text;
  ML_Attributes attributes;

  ML_Text(int id, double x0, double y0, double x1, double y1, std::string &text, ML_Attributes attributes) {
    this->id = id;
    this->x0 = x0;
    this->y0 = y0;
    this->x1 = x1;
    this->y1 = y1;
    this->text = text;
    this->attributes = attributes;
  }
  virtual std::vector<XMLNode> xml() const;
};


struct ML_Context {
  int id;
  double canvasWidth;
  double canvasHeight;

  std::vector<ML_Geom> objects;

  void initialise(double width, double height);
};

std::vector<XMLNode> ML_IterateOver(const std::vector<ML_Geom>& objects);

void DrawingMLDevice_activate(pDevDesc dd);
Rboolean DrawingMLDevice_newFrameConfirm(pDevDesc dd);
void DrawingMLDevice_onExit(pDevDesc dd);
void DrawingMLDevice_mode(int mode, pDevDesc dd);
void DrawingMLDevice_close(pDevDesc dd);
void DrawingMLDevice_clip(double x0, double x1, double y0, double y1, pDevDesc dd);
void DrawingMLDevice_newPage(const pGEcontext gc, pDevDesc dd);
SEXP DrawingMLDevice_cap(pDevDesc dd);
void DrawingMLDevice_size(double *left, double *right, double *top, double *bottom, pDevDesc dd);
void DrawingMLDevice_metricInfo(int c, const pGEcontext gc, double *ascent, double *descent, double *width, pDevDesc dd);
double DrawingMLDevice_strWidth(const char *str, const pGEcontext gc, pDevDesc pp);
void DrawingMLDevice_raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
void DrawingMLDevice_rect(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd);
void DrawingMLDevice_line(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd);
void DrawingMLDevice_circle(double x, double y, double r, const pGEcontext gc, pDevDesc dd);
void DrawingMLDevice_polyline(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
void DrawingMLDevice_polygon(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
void DrawingMLDevice_text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);

