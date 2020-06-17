#include <Rcpp.h>
#import <Foundation/Foundation.h>
#import <AppKit/NSPasteboard.h>
#include <vector>

bool DrawingML_UTI(std::string &uti) {
  CFStringRef dyn_uti;
  CFStringRef tag = CFSTR("com.microsoft.Art--GVML-ClipFormat");
  bool result = false;

  dyn_uti = UTTypeCreatePreferredIdentifierForTag(kUTTagClassNSPboardType, tag, kUTTypeContent);
  if (dyn_uti) {
    const char *c_str = CFStringGetCStringPtr(dyn_uti, kCFStringEncodingUTF8);
    if (c_str != NULL) {
      uti.assign(c_str);
      result = true;
    }

    CFRelease(dyn_uti);
  }

  return result;
}

void SendToClipboard(std::vector<uint8_t> &data) {
  std::string mldrawing_uti;

  if (DrawingML_UTI(mldrawing_uti)) {
    @autoreleasepool {
      NSPasteboard *pbpaste = [NSPasteboard generalPasteboard];
      NSString *nsmldrawing_uti = [NSString stringWithUTF8String:mldrawing_uti.c_str()];
      [pbpaste declareTypes: [NSArray arrayWithObject:nsmldrawing_uti] owner:nil];
      NSData *pbdata = [NSData dataWithBytesNoCopy:data.data() length:data.size() freeWhenDone:false];
      [pbpaste clearContents];
      [pbpaste setData:pbdata forType:nsmldrawing_uti];
    }
  }
}
