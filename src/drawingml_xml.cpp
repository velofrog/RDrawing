#include <Rcpp.h>
#include "drawingml_device.h"

std::string MLContainer_Content_Types() {
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

std::string MLContainer_Theme1(bool full_theme) {
  XML doc;
  doc.setRoot(
    XMLNode("a:clipboardTheme", {{"xmlns:a", "http://schemas.openxmlformats.org/drawingml/2006/main"}})
  );

  return doc.write();
}

std::string MLContainer_Relationships() {
  XML doc;
  doc.setRoot(
    XMLNode("Relationships", {{"xmlns", "http://schemas.openxmlformats.org/package/2006/relationships"}}) <<
      XMLNode("Relationship", {{"Id","rId1"},
                               {"Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing"},
                               {"Target", "clipboard/drawings/drawing1.xml"}})
  );

  return doc.write();
}

std::string MLContainer_DrawingRelationships() {
  XML doc;
  doc.setRoot(
    XMLNode("Relationships", {{"xmlns", "http://schemas.openxmlformats.org/package/2006/relationships"}}) <<
      XMLNode("Relationship", {{"Id","rId1"},
      {"Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme"},
      {"Target", "../theme/theme1.xml"}})
  );

  return doc.write();
}

std::string MLContainer_Drawing(const std::vector<ML_Geom>& objects) {
  XML doc;

  std::vector<XMLNode> nodes = ML_IterateOver(objects);

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



