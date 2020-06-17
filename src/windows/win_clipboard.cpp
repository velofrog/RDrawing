#if defined __WIN32 || defined __WIN32__
#define STRICT_R_HEADERS
#include <Rcpp.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

uint32_t CF_DRAWINGML = 0;

uint32_t DrawingML_ClipboardFormat() {
  if (CF_DRAWINGML == 0) CF_DRAWINGML = RegisterClipboardFormatA("Art::GVML ClipFormat");
  return CF_DRAWINGML;
}

void SendToClipboard(std::vector<uint8_t> &data) {
  HGLOBAL hglbData;

  if (data.size() == 0) return;

  hglbData = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, data.size());
  if (hglbData == NULL) {
    Rcpp::Rcerr << "Error: Cannot allocate memory (GlobalAlloc failed)\n";
    return;
  }

  uint8_t *ptr = (uint8_t *)GlobalLock(hglbData);
  if (ptr == NULL) {
    Rcpp::Rcerr << "Error: Cannot get pointer to memory object (GlobalLock failed)\n";
    return;
  }

  std::memcpy(ptr, data.data(), data.size());

  GlobalUnlock(hglbData);

  if (OpenClipboard(NULL)) {
    if (EmptyClipboard()) {
      if (SetClipboardData(DrawingML_ClipboardFormat(), hglbData) == NULL) GlobalFree(hglbData);
      CloseClipboard();
    } else {
      Rcpp::Rcerr << "Error: Cannot clear windows clipboard\n";
      CloseClipboard();
      GlobalFree(hglbData);
    }
  } else {
    Rcpp::Rcerr << "Error: Cannot open windows clipboard\n";
    GlobalFree(hglbData);
  }
}

#endif
