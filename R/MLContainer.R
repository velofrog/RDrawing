#' @export
MLContainer_Content_Types = function() {
  # start with this xml header in order to get standalone attribute
  doc = xml2::read_xml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Root />")

  node = xml2::xml_replace(xml2::xml_root(doc), "Types")
  xml2::xml_attr(node, "xmlns") = "http://schemas.openxmlformats.org/package/2006/content-types"

  node %>% xml2::xml_add_child("Default") %T>% xml2::xml_set_attr("Extension", "rels") %T>%
    xml2::xml_set_attr("ContentType", "application/vnd.openxmlformats-package.relationships+xml")
  node %>% xml2::xml_add_child("Default") %T>% xml2::xml_set_attr("Extension", "xml") %T>%
    xml2::xml_set_attr("ContentType", "application/xml")
  node %>% xml2::xml_add_child("Override") %T>% xml2::xml_set_attr("PartName", "/clipboard/drawings/drawing1.xml") %T>%
    xml2::xml_set_attr("ContentType", "application/vnd.openxmlformats-officedocument.drawing+xml")
  node %>% xml2::xml_add_child("Override") %T>% xml2::xml_set_attr("PartName", "/clipboard/theme/theme1.xml") %T>%
    xml2::xml_set_attr("ContentType", "application/vnd.openxmlformats-officedocument.theme+xml")

  conn = rawConnection(raw(0), "r+")
  xml2::write_xml(doc, conn, options = "")
  result = rawConnectionValue(conn)
  close(conn)

  return(result)
}

#' @export
MLContainer_Theme1 = function(full_theme = FALSE) {
  # start with this xml header in order to get standalone attribute
  doc = xml2::read_xml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
                       <a:clipboardTheme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" />")

  conn = rawConnection(raw(0), "r+")
  xml2::write_xml(doc, conn, options = "")
  result = rawConnectionValue(conn)
  close(conn)

  return(result)
}

#' @export
MLContainer_Relationships = function() {
  doc = xml2::read_xml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Root />")

  node = xml2::xml_replace(xml2::xml_root(doc), "Relationships")
  xml2::xml_attr(node, "xmlns") = "http://schemas.openxmlformats.org/package/2006/relationships"

  node %>% xml2::xml_add_child("Relationship") %T>%
    xml2::xml_set_attr("Id", "rId1") %T>%
    xml2::xml_set_attr("Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing") %T>%
    xml2::xml_set_attr("Target", "clipboard/drawings/drawing1.xml")

  conn = rawConnection(raw(0), "r+")
  xml2::write_xml(doc, conn, options = "")
  result = rawConnectionValue(conn)
  close(conn)

  return(result)
}

#' @export
MLContainer_DrawingRelationships = function() {
  doc = xml2::read_xml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><Root />")

  node = xml2::xml_replace(xml2::xml_root(doc), "Relationships")
  xml2::xml_attr(node, "xmlns") = "http://schemas.openxmlformats.org/package/2006/relationships"

  node %>% xml2::xml_add_child("Relationship") %T>%
    xml2::xml_set_attr("Id", "rId1") %T>%
    xml2::xml_set_attr("Type", "http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme") %T>%
    xml2::xml_set_attr("Target", "../theme/theme1.xml")

  conn = rawConnection(raw(0), "r+")
  xml2::write_xml(doc, conn, options = "")
  result = rawConnectionValue(conn)
  close(conn)

  return(result)
}

#' @export
MLContainer_Drawing = function(canvas) {
  if (!is.environment(canvas)) {
    stop("Invalid canvas object")
  }

  doc = xml2::read_xml("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
      <a:graphic xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">
      <a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/lockedCanvas\">
      <lc:lockedCanvas xmlns:lc=\"http://schemas.openxmlformats.org/drawingml/2006/lockedCanvas\">
      </lc:lockedCanvas>
      </a:graphicData>
      </a:graphic>")

  node = xml2::xml_find_first(doc, ".//lc:lockedCanvas")

  ML_group(node, id = 0, name = "canvas", x = canvas$x, y = canvas$y,
           cx = canvas$cx, cy = canvas$cy)

  for (obj in canvas$objects) {
    if (obj$type == "rect") do.call(ML_rect, append(list(node = node), obj))
  }

  conn = rawConnection(raw(0), "r+")
  xml2::write_xml(doc, conn, options = "")
  result = rawConnectionValue(conn)
  close(conn)

  return(result)
}

#' @export
DrawingML_SendToClipboard = function(canvas) {
  arc = archive_new()
  archive_add(arc, "[Content_Types].xml", MLContainer_Content_Types())
  archive_add(arc, "_rels/.rels", MLContainer_Relationships())
  archive_add(arc, "clipboard/drawings/_rels/drawing1.xml.rels", MLContainer_DrawingRelationships())
  archive_add(arc, "clipboard/theme/theme1.xml", MLContainer_Theme1())
  archive_add(arc, "clipboard/drawings/drawing1.xml", MLContainer_Drawing(canvas))

  ZipAndSendToClipboard(arc)
}

