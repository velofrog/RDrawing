#' @export
archive_new = function() {
  archive = new.env()
  archive$arc_objects = list()
  archive$arc_contents = list()

  return(archive)
}

#' @export
archive_add = function(archive, name, content) {
  if (missing(content)) return(archive)
  if (is.null(content)) return(archive)
  if (!is.raw(content)) content = charToRaw(as.character(content))

  archive$arc_objects[[length(archive$arc_objects) + 1]] = name
  archive$arc_contents[[length(archive$arc_contents) + 1]] = content

  return(archive)
}

