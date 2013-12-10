#include <string>
#include <iostream>
#include <sstream>
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
	vector<string> test;
	vector<double> clang;
	vector<double> without;
	vector<double> with;
	int seating_count = 0;
	while (getline(cin, line)) {
		string testname = tokenize(line)[1];
		if (testname[testname.size() - 1] == ':')
			testname = testname.substr(0, testname.size() - 1);
		if (testname == "seating") {
			++seating_count;
			if (seating_count <= 6 || seating_count > 8) { // ignore other seating tests
				getline(cin, line);
				getline(cin, line);
				getline(cin, line);
				continue;
			}
		}
		test.push_back("\"" + testname + "\"");
		getline(cin, line);
		clang.push_back(toDouble(tokenize(line)[2].substr(1)));
		getline(cin, line);
		without.push_back(toDouble(tokenize(line)[3].substr(1)));
		getline(cin, line);
		with.push_back(toDouble(tokenize(line)[3].substr(1)));
	}

	// normalize with respect to clang
	for (int i = 0; i < (int)without.size(); ++i) {
		without[i] /= clang[i];
		with[i] /= clang[i];
	}

	cout << toStr(test) << endl;
	cout << toStr(without) << endl;
	cout << toStr(with) << endl;
	return 0;
}