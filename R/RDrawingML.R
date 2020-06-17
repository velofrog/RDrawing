#' @useDynLib RDrawingML, .registration = TRUE
#' @import Rcpp
NULL

#' Forward-pipe operator
#'
#' Pipes an object forward into a function or call expression
#'
#' @importFrom magrittr %>%
#' @name %>%
#' @rdname pipe
#' @export
NULL

#' Tee operator
#'
#' Pipes an object forward into a function or call expression and return the original value
#'
#' @importFrom magrittr %T>%
#' @name %T>%
#' @rdname tee
#' @export
NULL

#' @export
pt_emu = function(points) return(floor(points * 12700))
#' @export
cm_emu = function(cm) return(floor(cm * 360000))
#' @export
in_emu = function(inches) return(floor(inches * 914400))

#' @export
ML_linetype = function(lty) {
  if ((lty == 0) | (lty == "blank")) return("blank")
  if ((lty == 1) | (lty == "solid")) return("solid")
  if ((lty == 2) | (lty == "dashed") | (lty == "dash")) return("dash")
  if ((lty == 3) | (lty == "dotted")) return("sysDot")
  if ((lty == 4) | (lty == "dotdash")) return("dashDot")
  if ((lty == 5) | (lty == "longdash")) return("lgDash")
  if ((lty == 6) | (lty == "twodash")) return("lgDashDot")

  return("solid")
}

#' @export
ML_colour = function(colour) {
  tryCatch({
    rgb_comp = col2rgb(colour, alpha = TRUE)
    return(sprintf("%02X%02X%02X", rgb_comp[["red", 1]], rgb_comp[["green", 1]], rgb_comp[["blue", 1]]))
  }, error = function(e)"000000")
}

#' @export
ML_alpha = function(colour) {
  tryCatch({
    rgb_comp = col2rgb(colour, alpha = TRUE)
    format(floor(rgb[["alpha", 1]] * 100000 / 255), scientific = FALSE)
  }, error = function(e)"100000")
}

ML_group = function(node, id, name = "", x, y, cx, cy) {
  node %>%
    xml2::xml_add_child("a:nvGrpSpPr") %T>%
    xml2::xml_add_child("a:cNvPr", id = id, name = name) %T>%
    xml2::xml_add_child("a:cNvGrpSpPr")

  node %>%
    xml2::xml_add_child("a:grpSpPr") %>%
    xml2::xml_add_child("a:xfrm") %T>%
    xml2::xml_add_child("a:off", x = x, y = y) %T>%
    xml2::xml_add_child("a:ext", cx = cx, cy = cy) %T>%
    xml2::xml_add_child("a:ChOff", x = x, y = y) %T>%
    xml2::xml_add_child("a:ChExt", cx = cx, cy = cy)

  invisible(node)
}

ML_rect = function(node, id, name = "", x = 0, y = 0, cx, cy, flipH = 0, flipV = 0,
                   colour = "black", fill = "transparent", lwd = pt_emu(0.25),
                   lty = "solid", text = NULL, text.colour = "black", ...) {
  shape = node %>% xml2::xml_add_child("a:sp")

  shape %>%
    xml2::xml_add_child("a:nvSpPr") %T>%
    xml2::xml_add_child("a:cNvPr", id = id, name = name) %T>%
    xml2::xml_add_child("a:cNvSpPr")

  shape_pr = shape %>% xml2::xml_add_child("a:spPr")

  if ((flipH == 0) & (flipV == 0)) {
    shape_pr %>%
      xml2::xml_add_child("a:xfrm") %T>%
      xml2::xml_add_child("a:off", x = x, y = y) %T>%
      xml2::xml_add_child("a:ext", cx = cx, cy = cy)
  } else {
    shape_pr %>%
      xml2::xml_add_child("a:xfrm", flipH = flipH, flipV = flipV) %T>%
      xml2::xml_add_child("a:off", x = x, y = y) %T>%
      xml2::xml_add_child("a:ext", cx = cx, cy = cy)
  }

  shape_pr %>% xml2::xml_add_child("a:prstGeom", prst = "rect")

  lty = ML_linetype(lty)
  if (lty == "blank") {
    lty = "solid"
    lwd = 0
  }

  shape_pr %>% xml2::xml_add_child("a:solidFill") %>%
    xml2::xml_add_child("a:srgbClr", val = ML_colour(fill)) %>%
    xml2::xml_add_child("a:alpha", val = ML_alpha(fill))

  shape_pr %>% xml2::xml_add_child("a:ln", w = lwd) %>%
    xml2::xml_add_child("a:solidFill") %>%
    xml2::xml_add_child("a:srgbClr", val = ML_colour(colour)) %T>%
    xml2::xml_add_child("a:alpha", val = ML_alpha(colour)) %>% xml2::xml_parent() %>% xml2::xml_parent() %>%
    xml2::xml_add_child("a:prstDash", val = lty)

  if (!is.null(text)) {
    shape_txt = shape %>% xml2::xml_add_child("a:txSp")

    shape_txt %>% xml2::xml_add_child("a:txBody") %T>%
      xml2::xml_add_child("a:bodyPr", rtlCol = "0", anchor = "ctr") %>%
      xml2::xml_add_child("a:p") %T>%
      xml2::xml_add_child("a:pPr", algn = "ctr") %>%
      xml2::xml_add_child("a:r") %>%
      xml2::xml_add_child("a:rPr") %>%
      xml2::xml_add_child("a:solidFill") %>%
      xml2::xml_add_child("a:srgbClr", val = ML_colour(text.colour)) %T>%
      xml2::xml_add_child("a:alpha", val = ML_alpha(text.colour)) %>%
      xml2::xml_parent() %>% xml2::xml_parent() %>% xml2::xml_parent() %>%
      xml2::xml_add_child("a:t", text)

    shape_txt %>% xml2::xml_add_child("a:useSpRect")
  }

  invisible(node)
}


#' @export
canvas_new = function() {
  canvas = new.env()
  canvas$objects = list()
  canvas$last_id = 0;
  canvas$x = 0;
  canvas$y = 0;
  canvas$cx = 0;
  canvas$cy = 0;
  return(canvas)
}

canvas_id = function(canvas, id) {
  if (missing(id)) id = canvas$last_id + 1
  if (id <= canvas$last_id) id = canvas$last_id + 1
  if (canvas$last_id < id) canvas$last_id = id

  return(id)
}

canvas_add = function(canvas, object) {
  canvas$objects[[length(canvas$objects) + 1]] = object
  if (object$x < canvas$x) canvas$x = object$x
  if (object$y < canvas$y) canvas$y = object$y
  if ((object$x + object$cx) > canvas$cx) canvas$cx = object$x + object$cx
  if ((object$y + object$cy) > canvas$cy) canvas$cy = object$y + object$cy
}

#' @export
canvas_rect = function(canvas, id, name = "", x = 0, y = 0, cx = cm_emu(1), cy = cm_emu(1),
                   flipH = 0, flipV = 0, col = "black", fill = "transparent",
                   lwd = pt_emu(0.25), lty = "solid", text = NULL, text.col = "black") {

  id = canvas_id(canvas, id)
  canvas_add(canvas, list(type = "rect", id = id, name = name, x = x, y = y, cx = cx, cy = cy,
                      flipH = flipH, flipV = flipV, colour = col, fill = fill,
                      lwd = lwd, lty = lty, text = text, text.colour = text.col))
}

.onUnload = function(libpath) {
  library.dynam.unload("RDrawingML", libpath)
}
