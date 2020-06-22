#include <Rcpp.h>
#include <iostream>
#include <vector>
#include <fstream>
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

void ZipAndSendToClipboard(const std::vector<std::pair<std::string, std::string>>& container) {
  miniz_cpp::zip_file zip;

  for (const auto& [arc_name, arc_contents] : container)
    zip.writestr(arc_name, arc_contents);

  std::vector<uint8_t> output;
  zip.save(output);

  std::ofstream fzip("/Users/michael/clip3/test_clip.zip", std::ios::out | std::ios::binary);
  fzip.write((char *)output.data(), output.size());
  fzip.close();

  SendToClipboard(output);
}
