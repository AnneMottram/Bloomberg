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
	localtime_s(date, &date_seconds);
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
	DatePlusDays(&timeinfo, -1 * days);
	strftime(buffer, 80, "%Y%m%d", &timeinfo);
	std::string yesterday(buffer);
	return yesterday;
}

bool in_range(std::vector<double>v){
	
	if ((*std::min_element(v.begin(), v.end()) > -10000) && (*std::max_element(v.begin(), v.end()) < 10000)){
		return true;
	}
	
	return false;
}

double average(std::vector<double>v){
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

double percentile(double val, std::vector<double>v){
	std::sort(v.begin(), v.end());
	double position = std::find(v.begin(), v.end(), val) - v.begin();
	std::cout << "position " << position << "\n";
	std::cout << "size " << v.size() << "\n";
	return round(100 * (position - 1) / v.size());
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

void upperAndLowerBonds(bool cluster, int subjectBond, DoubleMatrix data, long* maturities, int date_tolerance, int lower_date, int upper_date, DoubleMatrix& upperBonds, DoubleMatrix& lowerBonds){
	int lowerBound, upperBound;
	if (cluster == true){
		lowerBound = lower_date;
		upperBound = upper_date;
	}
	else{
		lowerBound = lower_date - date_tolerance;
		upperBound = upper_date + date_tolerance;
	}
	int current_min_lower = 1000000;
	int current_min_upper = 1000000;
	std::vector <double> subject = data.at(subjectBond);
	for (int j = 0; j < data.size(); j++){
		if (j == subjectBond){
			continue;
		}
		int diff = maturities[subjectBond] - maturities[j];
		if (diff >= lowerBound && diff <= upperBound && in_range(data[j])){
			if (cluster){
				lowerBonds.push_back(data[j]);
			}
			else if (diff < current_min_lower){
				current_min_lower = diff;
				lowerBonds.clear();
				lowerBonds.push_back(data[j]);
			}
		}
		else if (diff*-1 >= lowerBound && diff*-1 <= upperBound && in_range(data[j])){
			if (cluster){
				upperBonds.push_back(data[j]);
			}
			else if (diff*-1 < current_min_upper){
				current_min_upper = diff;
				upperBonds.clear();
				upperBonds.push_back(data[j]);
			}
		}
	}
}

/* Input: destination vector for time series, matrix of lower bond data, vector of subject bond data, matrix of upper bond data, length of time series
Output: vector of time series values
*/
bool flyTimeSeries(std::vector<double>& time_series, DoubleMatrix lower, std::vector<double> subject, DoubleMatrix upper, int length){
	double* lowerYields;
	double* upperYields;
	lowerYields = new double[length];
	upperYields = new double[length];

	if (lower.size() == 0 || upper.size() == 0 ||!(in_range(subject))){
		for (int i = 0; i < 8; i++){
			time_series.push_back(0);
		}
		return false;
	}
	else{
		averageYields(lower, lowerYields, length);
		averageYields(upper, upperYields, length);
		for (int i = 0; i < length; i++){
			time_series.push_back(subject[i] - 0.5*upperYields[i] - 0.5*lowerYields[i]);
		}
		return true;
	}
}

/*Input: destination vector for fly metrics and vector of fly time series
Output: fly metrics: current fly, change, average, standard deviation, standard score, min,max,percentile*/
void flyMetrics(double* fly_results, std::vector<double>& time_series){
	int length = time_series.size();
	fly_results[0] = time_series.at(length - 1);
	fly_results[1] = (time_series.at(length - 1) - time_series.at(length - 2));
	fly_results[2] = (average(time_series));
	fly_results[3] = (standard_deviation(time_series));
	fly_results[4] = ((time_series.at(length - 1) - average(time_series)) / standard_deviation(time_series));
	fly_results[5] = (*std::min_element(time_series.begin(), time_series.end()));
	fly_results[6] = (*std::max_element(time_series.begin(), time_series.end()));
	fly_results[7] = (percentile(time_series.at(length - 1), time_series));
	
}


