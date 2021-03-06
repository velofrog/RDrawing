// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// DrawingDevice
void DrawingDevice(double width, double height, double pointsize, std::string font);
RcppExport SEXP _RDrawing_DrawingDevice(SEXP widthSEXP, SEXP heightSEXP, SEXP pointsizeSEXP, SEXP fontSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< double >::type width(widthSEXP);
    Rcpp::traits::input_parameter< double >::type height(heightSEXP);
    Rcpp::traits::input_parameter< double >::type pointsize(pointsizeSEXP);
    Rcpp::traits::input_parameter< std::string >::type font(fontSEXP);
    DrawingDevice(width, height, pointsize, font);
    return R_NilValue;
END_RCPP
}
// ZipAndSendToClipboard
void ZipAndSendToClipboard(Rcpp::Environment archive);
RcppExport SEXP _RDrawing_ZipAndSendToClipboard(SEXP archiveSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< Rcpp::Environment >::type archive(archiveSEXP);
    ZipAndSendToClipboard(archive);
    return R_NilValue;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_RDrawing_DrawingDevice", (DL_FUNC) &_RDrawing_DrawingDevice, 4},
    {"_RDrawing_ZipAndSendToClipboard", (DL_FUNC) &_RDrawing_ZipAndSendToClipboard, 1},
    {NULL, NULL, 0}
};

RcppExport void R_init_RDrawing(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
