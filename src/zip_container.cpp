#include <Rcpp.h>
#include <iostream>
#include "clipboard.h"
#include "zip_file.hpp"

// [[Rcpp::export]]
void ZipAndSendToClipboard(Rcpp::Environment archive) {
  Rcpp::List archive_objects = archive["arc_objects"];
  Rcpp::List archive_contents = archive["arc_contents"];

  if (archive_objects.length() == 0) {
    Rcpp::Rcerr << "No objects in archive\n";
    return;
  }

  if (archive_objects.length() != archive_contents.length()) {
    Rcpp::Rcerr << "Mismatch between objects and contents in archive\n";
    return;
  }

  miniz_cpp::zip_file zip;

  for (int idx=0; idx<archive_objects.length(); idx++) {
    Rcpp::String arc_name = archive_objects[idx];
    Rcpp::RawVector data = archive_contents[idx];
    std::string arc_data(data.begin(), data.end());
    zip.writestr(arc_name, arc_data);
  }

  std::vector<uint8_t> output;
  zip.save(output);

  SendToClipboard(output);
}
