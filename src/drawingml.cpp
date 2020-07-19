#include <Rcpp.h>
#include "drawingml.h"

XMLNode ML_nvSpPr(int id, std::string name = "") {
  return
    XMLNode("a:nvSpPr") <<
      XMLNode("a:cNvPr", {{"id",std::to_string(id)},{"name",name}}) <<
      XMLNode("a:cNvSpPr");
}

XMLNode ML_nvGrpSpPr(int id, std::string name = "") {
  return
    XMLNode("a:nvGrpSpPr") <<
      XMLNode("a:cNvPr", {{"id",std::to_string(id)},{"name",name}}) <<
      XMLNode("a:cNvGrpSpPr");
}

XMLNode ML_xfrm(double x, double y, double width, double height) {
  return
    XMLNode("a:xfrm") <<
      XMLNode("a:off", {{"x",emu::str(x)},{"y",emu::str(y)}}) <<
      XMLNode("a:ext", {{"cx",emu::str(width)},{"cy",emu::str(height)}});
}

XMLNode ML_xfrm_rect(double x1, double y1, double x2, double y2) {
  std::vector<std::pair<std::string, std::string>> attr;
  if (x2 < x1) attr.push_back({"flipH","1"});
  if (y2 < y1) attr.push_back({"flipV","1"});

  XMLNode node("a:xfrm", attr);
  node << XMLNode("a:off", {{"x",emu::str(std::min(x1, x2))},{"y",emu::str(std::min(y1, y2))}});
  node << XMLNode("a:ext", {{"cx",emu::str(std::abs(x2-x1))},{"cy",emu::str(std::abs(y2-y1))}});

  return node;
}

XMLNode ML_xfrm_rect(double x1, double y1, double x2, double y2, double rotate) {
  std::vector<std::pair<std::string, std::string>> attr;
  if (x2 < x1) attr.push_back({"flipH","1"});
  if (y2 < y1) attr.push_back({"flipV","1"});
  attr.push_back({"rot",std::to_string(static_cast<int>(-60000.0 * rotate))});

  XMLNode node("a:xfrm", attr);
  node << XMLNode("a:off", {{"x",emu::str(std::min(x1, x2))},{"y",emu::str(std::min(y1, y2))}});
  node << XMLNode("a:ext", {{"cx",emu::str(std::abs(x2-x1))},{"cy",emu::str(std::abs(y2-y1))}});

  return node;
}

XMLNode ML_xfrm(double x, double y, double width, double height, double ch_x, double ch_y, double ch_width, double ch_height) {
  return
    XMLNode("a:xfrm") <<
      XMLNode("a:off", {{"x",emu::str(x)},{"y",emu::str(y)}}) <<
      XMLNode("a:ext", {{"cx",emu::str(width)},{"cy",emu::str(height)}}) <<
      XMLNode("a:chOff", {{"x",emu::str(ch_x)},{"y",emu::str(ch_y)}}) <<
      XMLNode("a:chExt", {{"cx",emu::str(ch_width)},{"cy",emu::str(ch_height)}});
}

XMLNode ML_solidFill(Drawing_Colour colour) {
  return
    XMLNode("a:solidFill") << (
        XMLNode("a:srgbClr", {{"val",colour.str_rgb()}}) <<
          XMLNode("a:alpha", {{"val",colour.str_alpha()}}));
}

XMLNode ML_ln(double width, Drawing_LineType linetype, Drawing_Colour colour) {
  if (linetype == Drawing_LineType::DRAWING_LINE_BLANK) {
    linetype = Drawing_LineType::DRAWING_LINE_SOLID;
    width = 0;
  }

  return
    XMLNode("a:ln", {{"w",emu::str(width)}}) <<
      ML_solidFill(colour) <<
        XMLNode("a:prstDash", {{"val",Drawing_LineType_str(linetype)}});
}

XMLNode ML_ln(Drawing_Attributes attributes) {
  return ML_ln(attributes.lineWidth, attributes.lineType, attributes.lineColour);
}

XMLNode ML_pt(double x, double y) {
  return XMLNode("a:pt", {{"x",emu::str(x)},{"y",emu::str(y)}});
}

std::vector<XMLNode> DrawingML_Group::xml() const {
  if (interior_objects.size() == 0) {
    return {
      ML_nvGrpSpPr(id, name),
      XMLNode("a:grpSpPr") <<
        ML_xfrm(0, 0, width, height, 0, 0, width, height)
    };
  }

  XMLNode node =
    XMLNode("a:grpSp") <<
      ML_nvGrpSpPr(id, name) <<
        (XMLNode("a:grpSpPr") << ML_xfrm(0, 0, width, height, 0, 0, width, height));

  std::vector<XMLNode> nodes;

  for (auto &object : interior_objects)
    for (auto &node : object->xml())
      nodes.push_back(node);

  node = node << nodes;

  return {node};
}

std::vector<XMLNode> DrawingML_Rect::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        ML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            ML_xfrm_rect(x0, y0, x1, y1),
            XMLNode("a:prstGeom", {{"prst","rect"}}),
            ML_solidFill(attributes.fillColour),
            ML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> DrawingML_Line::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        ML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            ML_xfrm_rect(x1, y1, x2, y2),
            XMLNode("a:prstGeom", {{"prst","line"}}),
            ML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> DrawingML_Circle::xml() const {
  return {
    XMLNode("a:sp") <<
      XMLNodes({
        ML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            ML_xfrm(x - radius, y - radius, radius * 2, radius * 2),
            XMLNode("a:prstGeom", {{"prst","ellipse"}}),
            ML_solidFill(attributes.fillColour),
            ML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> DrawingML_Polyline::xml() const {
  if (points.size() < 2) return {};

  auto minmax_x = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.first < rhs.first;});
  double x0 = minmax_x.first->first;
  double x1 = minmax_x.second->first;

  auto minmax_y = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.second < rhs.second;});
  double y0 = minmax_y.first->second;
  double y1 = minmax_y.second->second;

  XMLNode pathnode =
    XMLNode("a:path", {{"w",emu::str(x1-x0)},{"h",emu::str(y1-y0)}}) <<
      (XMLNode("a:moveTo") << ML_pt(points[0].first-x0, points[0].second-y0));

  for (std::size_t idx=1; idx<points.size(); idx++)
    pathnode = pathnode << (XMLNode("a:lnTo") << ML_pt(points[idx].first-x0, points[idx].second-y0));

  return {
    XMLNode("a:sp") <<
      XMLNodes({
        ML_nvSpPr(id, name),
        XMLNode("a:spPr") <<
          XMLNodes({
            ML_xfrm(x0, y0, x1-x0, y1-y0),
            XMLNode("a:custGeom") << XMLNode("a:avLst") << XMLNode("a:gdLst") <<
            XMLNode("a:ahLst") << XMLNode("a:cxnLst") << (XMLNode("a:pathLst") << pathnode),
            ML_ln(attributes)
          })
      })
  };
}

std::vector<XMLNode> DrawingML_Polygon::xml() const {
  if (points.size() < 2) return {};

  auto minmax_x = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.first < rhs.first;});
  double x0 = minmax_x.first->first;
  double x1 = minmax_x.second->first;

  auto minmax_y = std::minmax_element(points.begin(), points.end(), [](auto const& lhs, auto const& rhs){return lhs.second < rhs.second;});
  double y0 = minmax_y.first->second;
  double y1 = minmax_y.second->second;

  XMLNode pathnode =
    XMLNode("a:path", {{"w",emu::str(x1-x0)},{"h",emu::str(y1-y0)}}) <<
      (XMLNode("a:moveTo") << ML_pt(points[0].first-x0, points[0].second-y0));

  for (std::size_t idx=1; idx<points.size(); idx++)
    pathnode = pathnode << (XMLNode("a:lnTo") << ML_pt(points[idx].first-x0, points[idx].second-y0));

  pathnode = pathnode << XMLNode("a:close");

  return {
      XMLNode("a:sp") <<
        XMLNodes({
          ML_nvSpPr(id, name),
          XMLNode("a:spPr") <<
            XMLNodes({
              ML_xfrm(x0, y0, x1-x0, y1-y0),
              XMLNode("a:custGeom") << XMLNode("a:avLst") << XMLNode("a:gdLst") <<
              XMLNode("a:ahLst") << XMLNode("a:cxnLst") << (XMLNode("a:pathLst") << pathnode),
              ML_solidFill(attributes.fillColour),
              ML_ln(attributes)
            })
        })
    };
}

std::vector<XMLNode> DrawingML_Text::xml() const {
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
            ML_xfrm_rect(x0, y0, x1, y1, attributes.rotation),
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
                            ML_solidFill(attributes.lineColour), // use colour, not fill colour for text
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

void DrawingML_Context::initialise(double width, double height) {
  objects.clear();
  canvasWidth = width;
  canvasHeight = height;
  id = 2; // reserve id 0 for Canvas, id 1 for MainGroup
}

std::vector<std::pair<std::string, std::string>> DrawingML_Context::container() {
  std::vector<std::shared_ptr<Drawing_Geom>> canvas;

  canvas.emplace_back(std::make_shared<DrawingML_Group>(0, canvasWidth, canvasHeight, "Canvas"));
  canvas.emplace_back(std::make_shared<DrawingML_Group>(1, 0, 0, canvasWidth, canvasHeight, "MainGroup", objects));

  return
    {
      {"[Content_Types].xml", MLContainer_Content_Types()},
      {"_rels/.rels", MLContainer_Relationships()},
      {"clipboard/drawings/_rels/drawing1.xml.rels", MLContainer_DrawingRelationships()},
      {"clipboard/theme/theme1.xml", MLContainer_Theme1()},
      {"clipboard/drawings/drawing1.xml", MLContainer_Drawing(canvas)}
    };
}


std::string DrawingML_Context::MLContainer_Content_Types() {
  XML doc;
  doc.setRoot(
    XMLNode("Types", {{"xmlns", "http://schemas.openxmlformats.org/package/2006/content-types"}}) <<
      XMLNode("Default", {{"Extension","rels"}, {"ContentType", "application/vnd.openxmlformats-package.relationships+xml"}}) <<
      XMLNode("Default", {{"Extension","xml"}, {"ContentType", "application/xml"}}) <<
      XMLNode("Override", {{"PartName","/clipboard/drawings/drawing1.xml"}, {"ContentType", "application/vnd.openxmlformats-officedocument.drawing+xml"}}) <<
      XMLNode("Override", {{"PartName","/clipboard/theme/theme1.xml"}, {"ContentType", "application/vnd.openxmlformats-officedocument.theme+xml"}})
  );

  return doc.write();
}

std::string DrawingML_Context::MLContainer_Theme1(bool full_theme) {
  XML doc;
  doc.setRoot(
    XMLNode("a:clipboardTheme", {{"xmlns:a", "http://schemas.openxmlformats.org/drawingml/2006/main"}})
  );

  return doc.write();
}

std::string DrawingML_Context::MLContainer_Relationships() {
  XML doc;
  doc.setRoot(
    XMLNode("Relationships", {{"xmlns", "http://schemas.openxmlformats.org/package/2006/relationships"}}) <<
      XMLNode("Relationship", {{"Id","rId1"},
                               {"Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing"},
                               {"Target", "clipboard/drawings/drawing1.xml"}})
  );

  return doc.write();
}

std::string DrawingML_Context::MLContainer_DrawingRelationships() {
  XML doc;
  doc.setRoot(
    XMLNode("Relationships", {{"xmlns", "http://schemas.openxmlformats.org/package/2006/relationships"}}) <<
      XMLNode("Relationship", {{"Id","rId1"},
      {"Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme"},
      {"Target", "../theme/theme1.xml"}})
  );

  return doc.write();
}

std::string DrawingML_Context::MLContainer_Drawing(const std::vector<std::shared_ptr<Drawing_Geom>>& objects) {
  XML doc;

  std::vector<XMLNode> nodes;

  for (auto &object : objects)
    for (auto &node : object->xml())
      nodes.push_back(node);

  doc.setRoot(
    XMLNode("a:graphic", {{"xmlns:a", "http://schemas.openxmlformats.org/drawingml/2006/main"}}) <<
            (
                XMLNode("a:graphicData", {{"uri", "http://schemas.openxmlformats.org/drawingml/2006/lockedCanvas"}}) <<
                  (
                      XMLNode("lc:lockedCanvas", {{"xmlns:lc","http://schemas.openxmlformats.org/drawingml/2006/lockedCanvas"}}) <<
                        nodes
                  )
            )
  );

  return doc.write();
}



