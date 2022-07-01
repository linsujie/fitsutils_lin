#include <iostream>
#include "TFile.h"
#include "TH2D.h"

#include "docopt.h"
#include "fitsio.h"

using namespace std;

vector<double> read_th2(TH2D *h, bool reverse) {
  vector<double> result;
  result.reserve(h->GetNbinsX() * h->GetNbinsY());

  for (int iy = 1; iy <= h->GetNbinsX(); iy++)
    for (int ix = 1; ix <= h->GetNbinsX(); ix++)
      result.push_back(h->GetBinContent(reverse ? h->GetNbinsX() - ix + 1 : ix, iy));

  return result;
}

static const char USAGE[] =
R"(Convert the root TH2D to fits.

    Usage:
      ./th2_to_fits [options] <fname> <hname> <fitsname>

    Options:
      -h --help         Show this help.
      -r --reverse      Reverse the Ra direction [default: false].
)";
int main(int argc, const char *argv[]) {
  std::map<std::string, docopt::value> args = docopt::docopt(USAGE,  { argv + 1, argv + argc }, true);
  auto carg = [&](const string& key) -> const char* { return args.at(key).asString().c_str(); };

  TFile *file = TFile::Open(carg("<fname>"));
  TH2D *h = (TH2D*)file->Get(carg("<hname>"));
  h->Print();

  cout << "Writing to " << carg("<fitsname>") << endl;
  int status = 0;
  int bitpix = DOUBLE_IMG;
  long naxis = 2;
  long naxes[2] = { h->GetNbinsX(), h->GetNbinsY() };

  vector<double> array = read_th2(h, args.at("--reverse").asBool());

  fitsfile *fitsfile;

  remove(carg("<fitsname>"));
  if (fits_create_file(&fitsfile, carg("<fitsname>"), &status) != 0) {
    cerr << "Error:: cannot create the output fitsfile " << args.at("<fitsname>") << endl;
    exit(1);
  }

  fits_create_img(fitsfile, bitpix, naxis, naxes, &status);
  fits_write_img(fitsfile, TDOUBLE, 1, naxes[0] * naxes[1], &(array[0]), &status);

  char type1[8] = "Ra";
  char type2[8] = "Dec";
  fits_update_key(fitsfile, TSTRING, "CTYPE1", type1, "", &status);
  fits_update_key(fitsfile, TSTRING, "CTYPE2", type2, "", &status);

  long pix1 = 1;
  double val1 = h->GetXaxis()->GetBinCenter(1);
  double det1 = h->GetXaxis()->GetBinWidth(1);
  if (args.at("--reverse").asBool()) {
    val1 = h->GetXaxis()->GetBinCenter(h->GetNbinsX());
    det1 *= -1;
  }
  fits_update_key(fitsfile, TLONG, "CRPIX1", &pix1, "Ref pixel of Ra", &status);
  fits_update_key(fitsfile, TDOUBLE, "CRVAL1", &val1, "Ref value of Ra", &status);
  fits_update_key(fitsfile, TDOUBLE, "CDELT1", &det1, "delta Ra", &status);

  long pix2 = 1;
  double val2 = h->GetYaxis()->GetBinCenter(1);
  double det2 = h->GetYaxis()->GetBinWidth(1);
  fits_update_key(fitsfile, TLONG, "CRPIX2", &pix2, "Ref pixel of Dec", &status);
  fits_update_key(fitsfile, TDOUBLE, "CRVAL2", &val2, "Ref value of Dec", &status);
  fits_update_key(fitsfile, TDOUBLE, "CDELT2", &det2, "delta Dec", &status);

  fits_close_file(fitsfile, &status);
  return 0;
}
