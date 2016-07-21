typedef std::vector<std::vector<double> > DoubleMatrix;
void DatePlusDays(struct tm* date, int days);
double average(std::vector<double>vec);
double standard_deviation(std::vector<double>v);
std::string get_today();
std::string prevDate(int days);
void flyCalc(double* fly_results, DoubleMatrix lower, std::vector<double> subject, DoubleMatrix upper, int length);
void averageYields(DoubleMatrix m, double* arr, int cols);
