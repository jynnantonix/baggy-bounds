#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
using namespace std;

double toDouble(string s) {
	stringstream ss(s);
	double res;
	ss >> res;
	return res;
}

vector<string> tokenize(string line) {
	stringstream ss(line);
	vector<string> tokens;
	string tmp;
	while (ss >> tmp) {
		tokens.push_back(tmp);
	}
	return tokens;
}

string toStr(vector<string> str) {
	stringstream ss;
	ss << "[";
	for (int i = 0; i < (int)str.size(); ++i) {
		ss << str[i] << ", ";
	}
	ss << "]";
	return ss.str();
}

string toStr(vector<double> str) {
	stringstream ss;
	ss << "[";
	for (int i = 0; i < (int)str.size(); ++i) {
		ss << str[i] << ", ";
	}
	ss << "]";
	return ss.str();
}

int main() {
	string line;
	ifstream noopt("benchoutput.txt"), opt("benchoutput-opt.txt");
	vector<string> test;
	vector<double> clang;
	vector<double> without;
	vector<double> with;
	vector<double> afteropt;
	int seating_count = 0;
	while (getline(noopt, line)) {
		string testname = tokenize(line)[1];
		if (testname[testname.size() - 1] == ':')
			testname = testname.substr(0, testname.size() - 1);
		getline(opt, line);
		if (testname == "seating") {
			++seating_count;
			if (seating_count <= 6 || seating_count > 7) { // ignore other seating tests
				for (int i = 0; i < 3; ++i) {
					getline(noopt, line);
					getline(opt, line);
				}
				continue;
			}
		}
		test.push_back("\"" + testname + "\"");
		getline(noopt, line);
		clang.push_back(toDouble(tokenize(line)[2].substr(1)));
		getline(noopt, line);
		without.push_back(toDouble(tokenize(line)[3].substr(1)));
		getline(noopt, line);
		with.push_back(toDouble(tokenize(line)[3].substr(1)));
		getline(opt, line);
		getline(opt, line);
		getline(opt, line);
		afteropt.push_back(toDouble(tokenize(line)[3].substr(1)));
	}

	// normalize with respect to clang
	for (int i = 0; i < (int)without.size(); ++i) {
		without[i] /= clang[i];
		with[i] /= clang[i];
		afteropt[i] /= clang[i];
	}

	cout << toStr(test) << endl;
	cout << toStr(without) << endl;
	cout << toStr(with) << endl;
	cout << toStr(afteropt) << endl;
	opt.close();
	noopt.close();
	return 0;
}
