#include <blpapi_session.h>
#include<time.h>
#include<vector>
#include <windows.h>
#include<ctime>
#include "Header.h"

using namespace BloombergLP;
using namespace blpapi;
typedef std::vector<std::vector<double> > DoubleMatrix;
const int ISIN_LENGTH = 12;

class SimpleRefData{
private:
	const char* APIREFDATA_SVC;
	std::string d_host;
	int d_port;
	std::string today;
	std::string prior_workday;

public:
	SimpleRefData(int prior_day_diff) :APIREFDATA_SVC("//blp/refdata"){
		today = get_today();
		prior_workday = prevDate(prior_day_diff);
	}
	void runRefData(double** refData, double** yesterdayData, std::vector<std::string>bonds){
		d_host = "localhost";
		d_port = 8194;
		SessionOptions sessionOptions;
		sessionOptions.setServerHost(d_host.c_str());
		sessionOptions.setServerPort(d_port);
		Session session(sessionOptions);
		if (!session.start()){
			std::cout << "Failed to start session" << std::endl;
			return;
		}
		if (!session.openService(APIREFDATA_SVC)){
			std::cout << "Failed to open ref data service" << std::endl;
			return;
		}
		getTodayData(session, refData, bonds);
		getYesterdayData(session, yesterdayData, bonds);
	}

	void getTodayData(Session& session,double** data, std::vector<std::string>bonds){
		Service refDataService = session.getService(APIREFDATA_SVC);
		Request request = refDataService.createRequest("ReferenceDataRequest");
		request.getElement("fields").appendValue("MATURITY");
		request.getElement("fields").appendValue("ISSUE_DT");
		request.getElement("fields").appendValue("DUR_ADJ_MID");
		request.getElement("fields").appendValue("LAST_PRICE");
		request.getElement("fields").appendValue("PX_LAST_EOD");
		request.getElement("fields").appendValue("YLD_YTM_MID");
		request.getElement("fields").appendValue("ASSET_SWAP_SPD_MID");
		request.getElement("fields").appendValue("OAS_SPREAD_MID");
		request.getElement("fields").appendValue("BB_Z_SPREAD_TO_OIS_DISC_MID");
		for (int i = 0; i < bonds.size(); i++){
			const char* ISIN = bonds.at(i).c_str();
			request.getElement("securities").appendValue(ISIN);
		}
		session.sendRequest(request);
		while (true){	
			Event event = session.nextEvent();
			MessageIterator msgIter(event);
			while (msgIter.next()){
				Message message = msgIter.message();
				if (message.messageType() == "ReferenceDataResponse"){
					parse_ref_response(message, data);
				}
			}
			if (event.eventType() == Event::RESPONSE){
				break;
			}
		}
	}

	void getYesterdayData(Session& session, double** data, std::vector<std::string>bonds){
		Service refDataService = session.getService(APIREFDATA_SVC);
		Request request_yest = refDataService.createRequest("HistoricalDataRequest");
		for (int i = 0; i < bonds.size(); i++){
			const char* ISIN = bonds.at(i).c_str();
			std::cout << ISIN << std::endl;
			request_yest.getElement("securities").appendValue(ISIN);
		}
		request_yest.getElement("fields").appendValue("YLD_YTM_MID");
		request_yest.getElement("fields").appendValue("ASSET_SWAP_SPD_MID");
		request_yest.getElement("fields").appendValue("BB_Z_SPREAD_TO_OIS_DISC_MID");
		request_yest.set("periodicitySelection", "DAILY");
		request_yest.set("startDate", prior_workday.c_str());
		request_yest.set("endDate", prior_workday.c_str());
		session.sendRequest(request_yest);
		int i = 0;
		while (true){
			Event event = session.nextEvent();
			MessageIterator msgIter(event);
			while (msgIter.next()){
				Message message = msgIter.message();
				if (message.messageType() == "HistoricalDataResponse"){
					i = parse_historical_response(message, data, false);
				}
			}
			if (event.eventType() == Event::RESPONSE){
				break;
			}
		}
	}


	int runHistData(double** data, std::vector<std::string>bonds, int history){
		d_host = "localhost";
		d_port = 8194;
		SessionOptions sessionOptions;
		sessionOptions.setServerHost(d_host.c_str());
		sessionOptions.setServerPort(d_port);
		Session session(sessionOptions);
		if (!session.start()){
			std::cout << "Failed to start session" << std::endl;
			return 0;
		}
		if (!session.openService(APIREFDATA_SVC)){
			std::cout << "Failed to open ref data service" << std::endl;
			return 0;
		}
		Service refDataService = session.getService(APIREFDATA_SVC);
		Request request_hist = refDataService.createRequest("HistoricalDataRequest");
		std::wstring stemp2 = std::to_wstring(bonds.size());
		LPCWSTR sw2 = stemp2.c_str();
		OutputDebugString(L"number bonds ");
		OutputDebugString(sw2);


		for (int i = 0; i < bonds.size(); i++){
			const char* ISIN = bonds.at(i).c_str(); 
			request_hist.getElement("securities").appendValue(ISIN);

		}

		request_hist.getElement("fields").appendValue("YLD_YTM_MID");
		request_hist.set("periodicitySelection", "DAILY");
		std::string endDate = today;
		std::string startDate = prevDate(history);

		request_hist.set("startDate", startDate.c_str());
		request_hist.set("endDate", endDate.c_str());

		session.sendRequest(request_hist);
		int i = 0;

		while (true){
			Event event = session.nextEvent();
			MessageIterator msgIter(event);
			while (msgIter.next()){
				Message message = msgIter.message();
				if (message.messageType() == "HistoricalDataResponse"){
					i = parse_historical_response(message, data, true);
				}
				std::cout << std::endl;
			}
			if (event.eventType() == Event::RESPONSE){
				break;
			}
		}

		return i;
		OutputDebugString(L"i HERE ");

		std::wstring stemp = std::to_wstring(i);
		LPCWSTR sw = stemp.c_str();
		OutputDebugString(sw);
	}

	int parse_historical_response(Message message, double** data, bool fly){
		Element ReferenceDataResponse = message.asElement();
		ReferenceDataResponse.print(std::cout);
		if (ReferenceDataResponse.hasElement("responseError")){
			std::cout << "Response Error";
			OutputDebugString(L"ERRRRR");
			return 0;
		}

		Element securityData = message.getElement("securityData");
		Element fieldDataArray = securityData.getElement("fieldData");
		int numItems = fieldDataArray.numValues();
		int sequenceNum;
		for (int i = 0; i < numItems; i++){
			Element fieldData = fieldDataArray.getValueAsElement(i);
			sequenceNum = securityData.getElementAsInt32("sequenceNumber");
			if (fly == false){
				data[sequenceNum][i] = (fieldData.getElementAsFloat64("YLD_YTM_MID"));
				data[sequenceNum][i + 1] = (fieldData.getElementAsFloat64("ASSET_SWAP_SPD_MID"));
				data[sequenceNum][i + 2] = (fieldData.getElementAsFloat64("BB_Z_SPREAD_TO_OIS_DISC_MID"));
			}
			else {
				data[sequenceNum][i] = (fieldData.getElementAsFloat64("YLD_YTM_MID") * 100);
			}
		}
		return numItems;
	}

	void parse_ref_response(Message message, double** data){
		Element ReferenceDataResponse = message.asElement();
		ReferenceDataResponse.print(std::cout);
		if (ReferenceDataResponse.hasElement("responseError")){
			OutputDebugString(L"ERROR");
			return;
		}
		Element securityDataArray = message.getElement("securityData");
		int numItems = securityDataArray.numValues();
		double array[11];
		int sequenceNum;
		for (int i = 0; i < numItems; i++){
			Element securityData = securityDataArray.getValueAsElement(i);
			sequenceNum = securityData.getElementAsInt32("sequenceNumber");
			Element fieldData = securityData.getElement("fieldData");
			Datetime  maturity = fieldData.getElementAsDatetime("MATURITY"); 
			array[0] = (maturity.year()); //0
			array[1] = (maturity.month()); //1
			array[2] = (maturity.day()); //2
			Datetime  issue_dt = fieldData.getElementAsDatetime("ISSUE_DT"); 
			array[3] = (maturity.year() - issue_dt.year()); //3
			//newRow.push_back(issue_dt.month());
			//newRow.push_back(issue_dt.day());
			array[4] = (fieldData.getElementAsFloat64("DUR_ADJ_MID")); //4
			array[5] = (fieldData.getElementAsFloat64("LAST_PRICE")); //5
			array[6] = (fieldData.getElementAsFloat64("PX_LAST_EOD")); //6
			array[7] = (fieldData.getElementAsFloat64("YLD_YTM_MID")); //7
			array[8] = (fieldData.getElementAsFloat64("ASSET_SWAP_SPD_MID")); //8
			array[9] = (fieldData.getElementAsFloat64("BB_Z_SPREAD_TO_OIS_DISC_MID")); //9
			array[10] = (fieldData.getElementAsFloat64("BB_Z_SPREAD_TO_OIS_DISC_MID")); //10
		}
		for (int j = 0; j < 11; j++){
			data[sequenceNum][j] = array[j];
		}
	}
};

void getFlyMetrics(double* results, bool cluster, int subjectBond, DoubleMatrix data, long* maturities, int date_tolerance, int lower_date, int upper_date, int length){
	DoubleMatrix lowerBonds;
	DoubleMatrix upperBonds;
	std::vector<double>timeSeries;
	upperAndLowerBonds(cluster, subjectBond, data, maturities, date_tolerance, lower_date, upper_date, upperBonds, lowerBonds);
	bool dataExists = flyTimeSeries(timeSeries, lowerBonds, data.at(subjectBond), upperBonds, length);
	if (dataExists){
		flyMetrics(results, timeSeries);
	}
	else{
		for (int i = 0; i < 8; i++){
			results[i] = 0;
		}
	}
}

void chartData(std::vector<double>& results, bool cluster, int subjectBond, DoubleMatrix data, long* maturities, int date_tolerance, int lower_date, int upper_date, int length){
	DoubleMatrix lowerBonds;
	DoubleMatrix upperBonds;
	std::vector<double>timeSeries;
	upperAndLowerBonds(cluster, subjectBond, data, maturities, date_tolerance, lower_date, upper_date, upperBonds, lowerBonds);
	flyTimeSeries(results, lowerBonds, data.at(subjectBond), upperBonds, length);
}



long _stdcall getRefData(double* arr, long nlength, unsigned char bonds[], long numbonds,int prior_day_diff) {
	SimpleRefData bbg(prior_day_diff);

	double** refData;
	refData = new double *[numbonds+1];
	for (int i = 0; i <=numbonds; i++)
		refData[i] = new double[10];

	double** yesterdayData;
	yesterdayData = new double*[numbonds+1];
	for (int i = 0; i <= numbonds; i++){
		yesterdayData[i] = new double[3];
	}

	std::vector <std::string> bond_vec;

	for (int b = 0; b <= numbonds; b++){
		std::string prefix = "/isin/";
		std::string suffix;
		suffix.clear();
		for (int j = 0; j < ISIN_LENGTH; j++){
			char a = bonds[b + j*(numbonds + 1)];
			suffix += a;
		}
		
		std::wstring stemp = std::wstring(suffix.begin(), suffix.end());
		LPCWSTR sw = stemp.c_str();
		OutputDebugString(sw);
		OutputDebugString(L"\n");
		
		bond_vec.push_back((prefix + suffix).c_str());
	}
	bbg.runRefData(refData, yesterdayData, bond_vec);

	for (int i = 0; i <= numbonds; i++){
		double dataLocal[13];
		for (int j = 0; j < 8; j++){
			dataLocal[j] = refData[i][j];
		}
		dataLocal[6] = refData[i][5] - refData[i][6]; //Change in last price
		dataLocal[8] = (dataLocal[7] - yesterdayData[i][0])*100; //Change in yield
		dataLocal[9] = refData[i][8];
		dataLocal[10] = dataLocal[9] - yesterdayData[i][1]; //Change in ASW
		dataLocal[11] = refData[i][9];
		dataLocal[12] = dataLocal[11] - yesterdayData[i][2]; //Change in OAS
		//std::wstring stemp = std::to_wstring(refData.at(i).at(6));
		//std::wstring stemp2 = std::to_wstring(refData.at(i).at(5));

	
		for (int k = 0; k < 13; k++){
			arr[i + k*(numbonds + 1)] = dataLocal[k];
		}
		
	}

		/*
		std::wstring stemp = std::wstring(suffix.begin(), suffix.end());
		LPCWSTR sw = stemp.c_str();
		OutputDebugString(sw);
		OutputDebugString(L"\n");
		ref_data.run(dataLocal, (prefix + suffix).c_str());
		for (int j = 0; j < nlength; j++){
			//arr[b+j*(numbonds+1)] = j;
			arr[b+j*(numbonds + 1)] = dataLocal[j];
		}
		*/
	delete[] refData;
	delete[] yesterdayData;
	return 0;
}
/*Inputs: Destinaion Excel grid, array of ISINS, number of bonds, array of maturities, date tolerance for flies,  */
long _stdcall getFlyData(double* arr, unsigned char bonds[], int numbonds, long* maturities, int microWidth, int dateTolerance, int clusterLower, int clusterUpper, int history){
	SimpleRefData bbg(1);
	DoubleMatrix yieldData;
	DoubleMatrix micro;
	DoubleMatrix cluster;
	std::vector <std::string> bond_vec;
	std::wstring stemp2 = std::to_wstring(numbonds);
	LPCWSTR sw2 = stemp2.c_str();
	OutputDebugString(L"numbonds ");

	OutputDebugString(sw2);

	for (int b = 0; b <= numbonds; b++){
		std::string prefix = "/isin/";
		std::string suffix;
		suffix.clear();
		for (int j = 0; j < ISIN_LENGTH; j++){
			char a = bonds[b + j*(numbonds + 1)];
			suffix += a;
		}
		std::wstring stemp = std::wstring(suffix.begin(), suffix.end());
		LPCWSTR sw = stemp.c_str();
		OutputDebugString(sw);
		OutputDebugString(L"\n");

		bond_vec.push_back((prefix + suffix).c_str());
	}

	double** yieldArray;
	yieldArray = new double*[numbonds+1];
	for (int i = 0; i <= numbonds; i++){
		yieldArray[i] = new double[history];
	}

	int days_data = bbg.runHistData(yieldArray, bond_vec, history);
	std::wstring stemp = std::to_wstring(days_data);
	LPCWSTR sw = stemp.c_str();
	OutputDebugString(sw);

	yieldData.resize(numbonds+1);
	for (int i = 0; i <= numbonds; i++){
		yieldData[i].resize(days_data);
	}
	for (int i = 0; i <= numbonds; i++){
		for (int j = 0; j < days_data; j++){
			yieldData.at(i).at(j) = yieldArray[i][j];
		}
	}

	DoubleMatrix microMatrix;
	DoubleMatrix clusterMatrix;
	for (int i = 0; i < numbonds; i++){
		double microLocal[8];
		double clusterLocal[8];
		getFlyMetrics(microLocal, false, i, yieldData, maturities, dateTolerance, microWidth, microWidth, days_data);
		getFlyMetrics(clusterLocal, true, i, yieldData, maturities, 0, clusterLower, clusterUpper, days_data);
		double flyLocal[16];
		for (int j = 0; j < 8; j++){
			flyLocal[j] = microLocal[j];
			flyLocal[8 + j] = clusterLocal[j];
		}

		for (int k = 0; k < 16; k++){
			arr[i + k*(numbonds + 1)] = flyLocal[k];
		}
	}
	delete[] yieldArray;
	return 0;
}

long _stdcall getChartData(int bond_id, bool cluster, double* arr, unsigned char bonds[], int numbonds, long* maturities, int microWidth, int dateTolerance, int clusterLower, int clusterUpper, int history){
	SimpleRefData bbg(1);
	DoubleMatrix yieldData;
	std::vector <std::string> bond_vec;
	for (int b = 0; b < numbonds; b++){
		std::string prefix = "/isin/";
		std::string suffix;
		suffix.clear();
		for (int j = 0; j < ISIN_LENGTH; j++){
			char a = bonds[b + j*(numbonds + 1)];
			suffix += a;
		}
		bond_vec.push_back((prefix + suffix).c_str());
	}

	double** yieldArray;
	yieldArray = new double*[numbonds+1];
	for (int i = 0; i <= numbonds; i++){
		yieldArray[i] = new double[history];
	}

	int days_data = bbg.runHistData(yieldArray, bond_vec, history);

	yieldData.resize(numbonds);
	for (int i = 0; i <= numbonds; i++){
		yieldData[i].resize(days_data);
	}
	for (int i = 0; i <= numbonds; i++){
		for (int j = 0; j < days_data; j++){
			yieldData.at(i).at(j) = yieldArray[i][j];
		}
	}
	
	std::vector<double> dataLocal;
	if (cluster){
		chartData(dataLocal, cluster, bond_id, yieldData, maturities, 0, clusterLower, clusterUpper, days_data);
	}
	else{
		chartData(dataLocal, cluster, bond_id, yieldData, maturities, dateTolerance, microWidth, microWidth, days_data);
	}
	
	for (int i = 0; i < days_data; i++){
		arr[i] = dataLocal.at(i);
	}
	return days_data;
}