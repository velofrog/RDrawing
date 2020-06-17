#if defined __WIN32 || defined __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Rcpp.h>
#include <vector>

void SendToClipboard(std::vector<uint8_t> &data) {
  HGLOBAL handle;

  if (data.size() == 0) return;

  handle = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, data.size());
  if (handle == NULL) {
    Rcpp:Rcerr << "Error: Cannot allocate memory (GlobalAlloc failed)\n";
    return;
  }

  uint8_t *ptr = (uint8_t *)GlobalLock(handle);
  if (ptr == NULL) {
    Rcpp::Rcerr << "Error: Cannot get pointer to memory object (GlobalLock failed)\n";
    return;
  }

  std::memcpy(ptr, data.data(), data.size());

  GlobalUnlock(handle);

  if (OpenClipboard(NULL)) {
    if (EmptyClipboard(NULL)) {
      CloseClipboard();
    } else {
      Rcpp::Rcerr << "Error: Cannot clear windows clipboard\n";
      CloseClipboard();
    }
  } else {
    Rcpp::Rcerr << "Error: Cannot open windows clipboard\n";
  }

  GlobalFree(handle);
}

#endif
