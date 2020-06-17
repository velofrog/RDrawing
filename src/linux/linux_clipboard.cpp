#include <Rcpp.h>
#include <vector>

void SendToClipboard(std::vector<uint8_t> &data) {
  Rcpp::Rcerr << "Sending to clipboard not supported on this platform\n";
}
