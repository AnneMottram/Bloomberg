#include <blpapi_session.h>
#include <time.h>
#include <vector>
#include <ctime>
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
	void runRefData(DoubleMatrix& refData, DoubleMatrix& yesterdayData, std::vector<std::string>bonds){
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
		getTodayData(session,refData,bonds);
		getYesterdayData(session, yesterdayData,bonds);
	}

	void getTodayData(Session& session, DoubleMatrix& data, std::vector<std::string>bonds){
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

	void getYesterdayData(Session& session, DoubleMatrix& data, std::vector<std::string>bonds){
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
					i = parse_historical_response(message, data,false);
				}
			}
			if (event.eventType() == Event::RESPONSE){
				break;
			}
		}

	}
	

	int runHistData(DoubleMatrix& data, std::vector<std::string>bonds,int history){
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
		for (int i = 0; i < bonds.size(); i++){
			const char* ISIN = bonds.at(i).c_str();
			std::cout << ISIN << std::endl;
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
					i = parse_historical_response(message, data,true);
				}
				std::cout << std::endl;
			}
			if (event.eventType() == Event::RESPONSE){
				break;
			}
		}
	return i;

	}

	int parse_historical_response(Message message, DoubleMatrix& data, bool fly){
		Element ReferenceDataResponse = message.asElement();
		ReferenceDataResponse.print(std::cout);
		if (ReferenceDataResponse.hasElement("responseError")){
			std::cout << "Response Error";
			return 0;
		}
		Element securityData = message.getElement("securityData");
		Element fieldDataArray = securityData.getElement("fieldData");
		int numItems = fieldDataArray.numValues();
		std::vector<double>newRow;
		for (int i = 0; i < numItems; i++){
			Element fieldData = fieldDataArray.getValueAsElement(i);
			newRow.push_back(fieldData.getElementAsFloat64("YLD_YTM_MID")*100);
			if (fly == false){
				newRow.push_back(fieldData.getElementAsFloat64("ASSET_SWAP_SPD_MID"));
				newRow.push_back(fieldData.getElementAsFloat64("BB_Z_SPREAD_TO_OIS_DISC_MID"));
			}
		}
		data.push_back(newRow);
		return numItems;
	}

	void parse_ref_response(Message message, DoubleMatrix& data){
		Element ReferenceDataResponse = message.asElement();
		ReferenceDataResponse.print(std::cout);
		if (ReferenceDataResponse.hasElement("responseError")){
			return;
		}
		Element securityDataArray = message.getElement("securityData");
		int numItems = securityDataArray.numValues();
		std::vector<double>newRow;
		for (int i = 0; i < numItems; i++){
			Element securityData = securityDataArray.getValueAsElement(i);
			Element fieldData = securityData.getElement("fieldData");
			Datetime  maturity = fieldData.getElementAsDatetime("MATURITY");
			newRow.push_back(maturity.year());
			newRow.push_back(maturity.month());
			newRow.push_back(maturity.day());
			Datetime  issue_dt = fieldData.getElementAsDatetime("ISSUE_DT");
			newRow.push_back(maturity.year() - issue_dt.year());
			//newRow.push_back(issue_dt.month());
			//newRow.push_back(issue_dt.day());
			newRow.push_back(fieldData.getElementAsFloat64("DUR_ADJ_MID"));
			newRow.push_back(fieldData.getElementAsFloat64("LAST_PRICE"));
			newRow.push_back(fieldData.getElementAsFloat64("PX_LAST_EOD"));
			newRow.push_back(fieldData.getElementAsFloat64("YLD_YTM_MID")*100);
			newRow.push_back(fieldData.getElementAsFloat64("ASSET_SWAP_SPD_MID"));
			newRow.push_back(fieldData.getElementAsFloat64("OAS_SPREAD_MID"));
			newRow.push_back(fieldData.getElementAsFloat64("BB_Z_SPREAD_TO_OIS_DISC_MID"));
		}
		data.push_back(newRow);

	}
};

void calculate_flies(bool cluster, int numBonds, int days_data, DoubleMatrix data, long* maturities, long date_tolerance, long lower_date, long upper_date, double* fly_results){
	int lowerBound, upperBound;
	if (cluster == true){
		lowerBound = lower_date;
		upperBound = upper_date;
	}
	else{
		lowerBound = lower_date - date_tolerance;
		upperBound = upper_date + date_tolerance;
	}
	int current_min_lower = INFINITY;
	int current_min_upper = INFINITY;
	for (int i = 0; i < numBonds; i++){
		DoubleMatrix lowerFly;
		DoubleMatrix upperFly;
		std::vector <double> subject = data.at(i);
		for (int j = 0; j < numBonds; j++){
			if (i == j){
				continue;
			}
			int diff = maturities[i] - maturities[j];
			if (diff >= lowerBound && diff <= upperBound){
				if (cluster)
					lowerFly.push_back(data[j]);
				else if (diff < current_min_lower){
					current_min_lower = diff;
					lowerFly.clear;
					lowerFly.push_back(data[j]);
				}
			}
			else if (diff*-1 >= lowerBound && diff*-1 <= upperBound){
				if (cluster)	
					upperFly.push_back(data[j]);
				else if (diff*-1 < current_min_upper){
					current_min_upper = diff;
					upperFly.clear;
					upperFly.push_back(data[j]);
				}
			}
		}
		//std::cout << lowerFly.size()<<" ";
		//std::cout << upperFly.size() << std::endl;
		flyCalc(fly_results, lowerFly, subject, upperFly, days_data);

		for (int i = 0; i < 5; i++){
			std::cout << fly_results[i] << " ";
		}
		std::cout << std::endl;
	}

}

int main(int argc, char **argv){
	// pull all the data first for all the bonds into a history X numbonds array
	// do the maturity check
	/// if its in 
	std::cout << "Example";
	SimpleRefData example(1);
	DoubleMatrix refData;
	DoubleMatrix yesterdayData;


	/*
	double dataArray1[13];
	DoubleMatrix data;
	*/
	std::string bond1 = "/isin/IT0004923998";
	std::string bond2 = "/isin/IT0005162828";
	std::string bond3 = "/isin/IT0005083057";
	std::vector <std::string> vec;
	vec.push_back(bond1);
	vec.push_back(bond2);
	vec.push_back(bond3);
	example.runRefData(refData, yesterdayData, vec);
	std::cout << "data size "<< refData.size() << std::endl;
	std::cout << "data size " << refData[0].size() << std::endl;
	for (int i = 0; i < yesterdayData.size(); i++){
		for (int j = 0; j < yesterdayData[i].size(); j++){
			std::cout << yesterdayData.at(i).at(j) <<" ";
		}
		std::cout << "\n";
	}

	for (int i = 0; i < refData.size(); i++)
	{
		double dataLocal[13];
		for (int j = 0; j < 8; j++){
			dataLocal[j] = refData.at(i).at(j);	
		}
		dataLocal[8] = (dataLocal[7] - yesterdayData.at(i).at(0))*100;
		dataLocal[9] = refData.at(i).at(8);
		dataLocal[10] = dataLocal[9] - yesterdayData.at(i).at(1);
		dataLocal[11] = refData.at(i).at(9);
		dataLocal[12] = dataLocal[11] - yesterdayData.at(i).at(2);

		for (int k = 0; k < 13; k++){
			std::cout << dataLocal[k]<<" ";
		}		
		std::cout << std::endl;
	}
	
	DoubleMatrix flydata;
	int history = 10;
	int days_data = example.runHistData(flydata, vec, history);
	long maturities [] = {53752, 53571, 52841};
	
	for (int i = 0; i < flydata.size(); i++)
	{
		for (int j = 0; j < flydata[i].size(); j++)
		{
			std::cout << flydata.at(i).at(j) <<" ";
		}
		std::cout << std::endl;
	}
	

	
	int numBonds = 3;
	double fly_results[5];
	calculate_flies(numBonds, days_data, flydata, maturities, 0, 1, 1,fly_results);
	for (int i = 0; i < 5; i++){
		std::cout << fly_results[i] << " ";
	}

	return 0;
}
