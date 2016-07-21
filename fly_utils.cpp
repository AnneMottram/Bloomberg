#include<ctime>
#include<numeric>
#include<vector>
#include<iostream>
#include<algorithm>
typedef std::vector<std::vector<double> > DoubleMatrix;

void DatePlusDays(struct tm* date, int days)
{
	const time_t ONE_DAY = 24 * 60 * 60;

	// Seconds since start of epoch
	time_t date_seconds = mktime(date) + (days * ONE_DAY);

	// Update caller's date
	// Use localtime because mktime converts to UTC so may change date
	localtime_s(date,&date_seconds);
}

std::string get_today(){
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	strftime(buffer, 80, "%Y%m%d", &timeinfo);
	std::string today(buffer);
	return today;
}

std::string prevDate(int days){
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	DatePlusDays(&timeinfo, -1*days);
	strftime(buffer, 80, "%Y%m%d", &timeinfo);
	std::string yesterday(buffer);
	return yesterday;
}

double average(std::vector<double>v){
	if (v.size() == 0){
		return 0; //0 value if no fly data
	}
	return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double standard_deviation(std::vector<double>v){
	if (v.size() == 0){
		return 0; //0 value if no fly data
	}
	double sum = std::accumulate(v.begin(), v.end(), 0.0);
	double mean = sum / v.size();
	double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
	return std::sqrt(sq_sum / v.size() - mean * mean);
}




void averageYields(DoubleMatrix m, double* arr, int cols){
	int rows = m.size();
	for (int i = 0; i < cols; i++){
		double total = 0;
		for (int j = 0; j < rows; j++){
			total += m[j][i];
		}
		arr[i] = total / rows;
	}
}

/*Inputs: matrix of lower bond fly data, vector of subject bond data , matrix of upper bond fly data, number of data points(days)
Outputs: array [average of historical flies, standard deviation, standard score, today's fly, % change from yday] 
*/

void flyCalc(double* fly_results, DoubleMatrix lower, std::vector<double> subject, DoubleMatrix upper, int length){
	double* lowerYields;
	double* upperYields;	
	std::vector<double> flies;
	lowerYields = new double[length];
	upperYields = new double[length];

	if (lower.size() == 0 || upper.size() == 0){
		for (int i = 0; i < length; i++){
			flies.push_back(0);
		}
	}
	else{
		averageYields(lower, lowerYields, length);
		averageYields(upper, upperYields, length);
	}

	for (int i = 0; i < length; i++){
		flies.push_back(subject[i] + 0.5*upperYields[i] - 0.5*lowerYields[i]);
		//std::cout << flies[i] << " ";
	}
	fly_results[0] = average(flies);
	fly_results[1] = standard_deviation(flies);
	if (fly_results[1] == 0){
		fly_results[2] = 0; //0 value if no fly data
		fly_results[3] = 0;
		fly_results[4] = 0;
	}

	else{
		fly_results[2] = flies[length - 1] - fly_results[0] / fly_results[1];
		fly_results[3] = flies[length - 1];
		fly_results[4] = (flies[length - 1] / flies[length - 2]) - 1;
		fly_results[5] = *std::min_element(flies.begin(), flies.end());
		fly_results[6] = *std::max_element(flies.begin(), flies.end());
		//add percentiles
	}

}